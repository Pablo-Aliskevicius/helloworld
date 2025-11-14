#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// POSIX message queue names
#define MQ_A  "/ipc_consumerA"
#define MQ_B  "/ipc_consumerB"

volatile sig_atomic_t producer_ready = 0;

static void sigusr1_handler(int sig) {
    (void)sig;
    producer_ready = 1; 
}

int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

_Noreturn void producer_process(void) {
    mqd_t mq_a, mq_b;
    // producer does not need to set attributes here (parent created queues)

    mq_a = mq_open(MQ_A, O_WRONLY);
    if (mq_a == (mqd_t)-1) {
        perror("producer: mq_open A");
        exit(EXIT_FAILURE);
    }
    mq_b = mq_open(MQ_B, O_WRONLY);
    if (mq_b == (mqd_t)-1) {
        perror("producer: mq_open B");
        mq_close(mq_a);
        exit(EXIT_FAILURE);
    }

    // Notify parent that producer is ready (so it can fork consumers)
    if (kill(getppid(), SIGUSR1) == -1) {
        perror("producer: kill parent");
        // continue nonetheless
    }

    // Produce numbers 1..100
    for (int32_t i = 1; i <= 100; ++i) {
        if (mq_send(mq_a, (const char *)&i, sizeof(i), 0) == -1) {
            perror("producer: mq_send A");
        }
        if (mq_send(mq_b, (const char *)&i, sizeof(i), 0) == -1) {
            perror("producer: mq_send B");
        }
        // small sleep to avoid flooding (and make output readable)
        usleep(10000);
    }

    // Send termination marker (-1)
    int32_t term = -1;
    if (mq_send(mq_a, (const char *)&term, sizeof(term), 0) == -1) {
        perror("producer: mq_send term A");
    }
    if (mq_send(mq_b, (const char *)&term, sizeof(term), 0) == -1) {
        perror("producer: mq_send term B");
    }

    mq_close(mq_a);
    mq_close(mq_b);
    exit(EXIT_SUCCESS);
}

_Noreturn void consumer_a_process(void) {
    mqd_t mq;
    mq = mq_open(MQ_A, O_RDONLY);
    if (mq == (mqd_t)-1) {
        perror("consumer A: mq_open");
        exit(EXIT_FAILURE);
    }
    int32_t val;
    ssize_t r;
    while (1) {
        r = mq_receive(mq, (char *)&val, sizeof(val), NULL);
        if (r == -1) {
            if (errno == EINTR) continue;
            perror("consumer A: mq_receive");
            break;
        }
        if (val == -1) break;
        printf("[A: pid %d] received %d → square = %d\n", getpid(), val, val * val);
        fflush(stdout);
    }
    mq_close(mq);
    exit(EXIT_SUCCESS);
}

_Noreturn void consumer_b_process(void) {
    mqd_t mq;
    mq = mq_open(MQ_B, O_RDONLY);
    if (mq == (mqd_t)-1) {
        perror("consumer B: mq_open");
        exit(EXIT_FAILURE);
    }
    int32_t val;
    ssize_t r;
    while (1) {
        r = mq_receive(mq, (char *)&val, sizeof(val), NULL);
        if (r == -1) {
            if (errno == EINTR) continue;
            perror("consumer B: mq_receive");
            break;
        }
        if (val == -1) break;
        printf("[B: pid %d] received %d → prime = %s\n", getpid(), val, is_prime(val) ? "yes" : "no");
        fflush(stdout);
    }
    mq_close(mq);
    exit(EXIT_SUCCESS);
}

int main(void) {
    pid_t pid_producer, pid_a, pid_b;
    struct mq_attr attr;

    // Install signal handler for SIGUSR1
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // Create message queues (parent creates so attributes are set)
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int32_t);
    attr.mq_curmsgs = 0;

    mqd_t q;
    q = mq_open(MQ_A, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (q == (mqd_t)-1) {
        if (errno == EEXIST) {
            mq_unlink(MQ_A);
            q = mq_open(MQ_A, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
            if (q == (mqd_t)-1) {
                perror("mq_open A create");
                return EXIT_FAILURE;
            }
        } else {
            perror("mq_open A");
            return EXIT_FAILURE;
        }
    }
    mq_close(q);

    q = mq_open(MQ_B, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (q == (mqd_t)-1) {
        if (errno == EEXIST) {
            mq_unlink(MQ_B);
            q = mq_open(MQ_B, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
            if (q == (mqd_t)-1) {
                perror("mq_open B create");
                mq_unlink(MQ_A);
                return EXIT_FAILURE;
            }
        } else {
            perror("mq_open B");
            // Main difference between C and C++: you must clean up unrelated resources
            mq_unlink(MQ_A);
            return EXIT_FAILURE;
        }
    }
    mq_close(q);

    // Fork producer first
    pid_producer = fork();
    if (pid_producer == -1) {
        perror("fork producer");
        mq_unlink(MQ_A);
        mq_unlink(MQ_B);
        return EXIT_FAILURE;
    }
    if (pid_producer == 0) {
        // child -> producer
        producer_process();
    }

    // Wait for producer to signal readiness
    while (!producer_ready) {
        pause();
    }

    // Fork consumers after producer signaled ready
    pid_a = fork();
    if (pid_a == -1) {
        perror("fork consumer A");
        kill(pid_producer, SIGTERM);
        mq_unlink(MQ_A);
        mq_unlink(MQ_B);
        return EXIT_FAILURE;
    }
    if (pid_a == 0) {
        consumer_a_process();
    }

    pid_b = fork();
    if (pid_b == -1) {
        perror("fork consumer B");
        kill(pid_producer, SIGTERM);
        kill(pid_a, SIGTERM);
        mq_unlink(MQ_A);
        mq_unlink(MQ_B);
        return EXIT_FAILURE;
    }
    if (pid_b == 0) {
        consumer_b_process();
    }

    // Parent: wait for children
    int status;
    waitpid(pid_producer, &status, 0);
    waitpid(pid_a, &status, 0);
    waitpid(pid_b, &status, 0);

    // Cleanup
    if (mq_unlink(MQ_A) == -1) {
        perror("mq_unlink A");
    }
    if (mq_unlink(MQ_B) == -1) {
        perror("mq_unlink B");
    }

    return EXIT_SUCCESS;
}
