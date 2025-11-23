#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "../common/shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <ctype.h>
#include <poll.h>

/* Global variables */
static int shm_fd = -1;
static SharedMemory *shared_mem = NULL;
static mqd_t global_queue = -1;
static mqd_t client_queue = -1;
static pid_t client_pid;
static int gt_promted = 0;

/* Function declarations */
static void cleanup(void);
static int connect_to_server(void);
static void disconnect_from_server(void);
static int request_server_offset(uint32_t length, uint32_t *offset);
static void notify_string_available(uint32_t offset, uint32_t length);
static int receive_messages(void);
static int check_for_exit(char input[1024]);
static char* str_lower(char *str);

/* Clean up resources on exit */
static void cleanup(void)
{
    if (client_queue >= 0) {
        mq_close(client_queue);
        char queue_name[64];
        snprintf(queue_name, sizeof(queue_name), "/chat_queue_%d", client_pid);
        mq_unlink(queue_name);
    }
    if (shared_mem != NULL) {
        munmap(shared_mem, sizeof(SharedMemory));
    }
    if (shm_fd >= 0) {
        close(shm_fd);
    }
    if (global_queue >= 0) {
        mq_close(global_queue);
    }
}

/* Convert string to lowercase */
static char* str_lower(char *str)
{
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
    return str;
}

/* Connect to the server */
static int connect_to_server(void)
{
    struct mq_attr attr;
    char queue_name[64];
    QueueMessage msg;

    client_pid = getpid();

    /* Create client message queue */
    snprintf(queue_name, sizeof(queue_name), "/chat_queue_%d", client_pid);
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(QueueMessage);
    attr.mq_curmsgs = 0;

    client_queue = mq_open(queue_name, O_CREAT | O_RDONLY, 0666, &attr);
    if (client_queue < 0) {
        perror("mq_open client queue");
        return -1;
    }

    /* Open global queue */
    global_queue = mq_open(GLOBAL_QUEUE_NAME, O_WRONLY);
    if (global_queue < 0) {
        fprintf(stderr, "Server not running.\n");
        cleanup();
        return -1;
    }

    /* Send connection request to server */
    msg.mtype = MSG_CLIENT_CONNECT;
    msg.client_pid = client_pid;
    msg.offset = 0;
    msg.length = 0;
    memset(msg.data, 0, sizeof(msg.data));

    if (mq_send(global_queue, (const char *)&msg, sizeof(msg), 0) < 0) {
        perror("mq_send connect");
        cleanup();
        return -1;
    }

    usleep(100000);  /* Wait for server to process */
    return 0;
}

/* Disconnect from the server */
static void disconnect_from_server(void)
{
    QueueMessage msg;

    msg.mtype = MSG_CLIENT_DISCONNECT;
    msg.client_pid = client_pid;
    msg.offset = 0;
    msg.length = 0;
    memset(msg.data, 0, sizeof(msg.data));

    if (global_queue >= 0) {
        mq_send(global_queue, (const char *)&msg, sizeof(msg), 0);
    }
}

/* Request memory offset from server for a message */
static int request_server_offset(uint32_t length, uint32_t *offset)
{
    (void)length;
    (void)offset;
    /* In this simplified version, we allocate sequentially */
    // BUG: The offset allocation is not synchronized with the server
    // In a real implementation, the client should send a request to the server
    // and wait for a response with the allocated offset.
    static uint32_t allocated = 0;
    if (allocated + length > SHARED_MEMORY_SIZE) {
        return -1;
    }
    *offset = allocated;
    allocated += length;
    return 0;
}

/* Notify server that string is available */
static void notify_string_available(uint32_t offset, uint32_t length)
{
    QueueMessage msg;

    msg.mtype = MSG_STRING_AVAILABLE;
    msg.client_pid = client_pid;
    msg.offset = offset;
    msg.length = length;
    memset(msg.data, 0, sizeof(msg.data));

    mq_send(global_queue, (const char *)&msg, sizeof(msg), 0);
}

/* Receive and display broadcast messages */
// Return 0 to break, 1 to continue waiting for messages
static int receive_messages(void)
{
    QueueMessage msg;
    unsigned int priority;
    int ret;
    // Check for messages
    struct mq_attr attr;
    mq_getattr(client_queue, &attr);
    if (attr.mq_curmsgs == 0) {
        return 1;
    }

    // If there are messages, process them
    ret = mq_receive(client_queue, (char *)&msg, sizeof(msg), &priority);
    if (ret < 0) {
        if (errno != EAGAIN) {
            perror("mq_receive");
        }
        return 0;
    }

    if (msg.mtype == MSG_BROADCAST) {
        if (msg.offset < SHARED_MEMORY_SIZE && msg.length > 0 &&
            msg.offset + msg.length <= SHARED_MEMORY_SIZE) {
            // printf("\b \b");
            // fflush(stdout);
            printf("\b\b< %.*s\n", (int)msg.length, &shared_mem->data[msg.offset]);

        }
    } else if (msg.mtype == MSG_DISCONNECT_REQUEST) {
        printf("Server is shutting down.\n");
    }
    gt_promted = 0;
    return msg.mtype == MSG_DISCONNECT_REQUEST ? 0 : 1;
}

/* Get the file descriptor for the message queue */
static int get_mq_fd(void)
{
    return (int)client_queue;
}

/* Main client loop */
int main(void)
{
    uint32_t offset;
    char input[MAX_MESSAGE_SIZE];
    uint32_t length;

    /* Open shared memory */
    shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        fprintf(stderr, "Server not running.\n");
        exit(EXIT_FAILURE);
    }

    /* Map shared memory */
    shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE,
                      MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Connect to server */
    if (connect_to_server() < 0) {
        fprintf(stderr, "Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    client_pid = getpid();
    printf("Process ID: %d\n", client_pid);
    printf("Enter message (type 'bye' or 'exit' to quit):\n");
    gt_promted = printf("> ");
    fflush(stdout);

    /* Main interaction loop using poll */
    while (1) {
        struct pollfd fds[2];
        int poll_ret;

        /* Set up poll file descriptors: stdin and message queue */
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        fds[1].fd = get_mq_fd();
        fds[1].events = POLLIN;
        fds[1].revents = 0;

        /* Wait for input on either stdin or message queue */
        poll_ret = poll(fds, 2, 100);
        if (poll_ret < 0) {
            perror("poll");
            break;
        }

        /* Process incoming message queue events */
        if (fds[1].revents & POLLIN) {
            if (!receive_messages())
                break;  /* Server requested disconnect */
        }

        /* Process stdin events */
        if (fds[0].revents & POLLIN) {
            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;
            }

            /* Remove trailing newline */
            length = strlen(input);
            if (length > 0 && input[length - 1] == '\n') {
                input[length - 1] = '\0';
                length--;
            }

            if (length == 0) {
                gt_promted = printf("> ");
                fflush(stdout);
                continue;
            }

            if (check_for_exit(input)) {
                break;
            }

            /* Request offset from server */
            if (request_server_offset(length, &offset) < 0) {
                fprintf(stderr,
                    "No more space in shared memory.\n");
                gt_promted = printf("> ");
                fflush(stdout);
                continue;
            }

            /* Write message to shared memory */
            if (offset + length <= SHARED_MEMORY_SIZE) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
                strncpy(&shared_mem->data[offset], input, length);
#pragma GCC diagnostic pop
                shared_mem->data[offset + length] = '\0';
            }

            /* Notify server */
            notify_string_available(offset, length);
            gt_promted = printf("> ");
            fflush(stdout);
        } else if (poll_ret == 0 && !gt_promted) {
            /* Timeout occurred, no activity - keep prompt visible */
            gt_promted = printf("> ");
            fflush(stdout);
        }
    }

    /* Clean disconnect */
    disconnect_from_server();
    usleep(100000);

    cleanup();
    return EXIT_SUCCESS;
}

/// @brief Returns -1 if exit command is detected, 0 otherwise
/// @param input 
/// @return 
int check_for_exit(char input[1024])
{
    /* Check for exit commands */
    char input_lower[MAX_MESSAGE_SIZE];
    strcpy(input_lower, input);
    str_lower(input_lower);

    if (strcmp(input_lower, "bye") == 0 ||
        strcmp(input_lower, "exit") == 0)
    {
        {
            return -1;
        };
    }
    return 0;
}
