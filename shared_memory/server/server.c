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
#include <signal.h>
#include <pthread.h>
#include <errno.h>

/* Global variables */
static int shm_fd = -1;
static SharedMemory *shared_mem = NULL;
static mqd_t global_queue = -1;
static volatile sig_atomic_t shutdown_requested = 0;

/* Function declarations */
static void cleanup(void);
static void signal_handler(int sig);
static void handle_client_connect(pid_t client_pid);
static void handle_client_disconnect(pid_t client_pid);
static void broadcast_message(pid_t sender_pid, uint32_t offset, uint32_t length);
static int find_client_index(pid_t pid);
static void send_message_to_client(pid_t pid, MessageType type, uint32_t offset, uint32_t length);
static void process_global_queue(void);

/* Cleanup resources on exit */
static void cleanup(void)
{
    if (shared_mem != NULL) {
        munmap(shared_mem, sizeof(SharedMemory));
    }
    if (shm_fd >= 0) {
        close(shm_fd);
        shm_unlink(SHARED_MEMORY_NAME);
    }
    if (global_queue >= 0) {
        mq_close(global_queue);
        mq_unlink(GLOBAL_QUEUE_NAME);
    }
}

/* Signal handler for graceful shutdown */
static void signal_handler(int sig)
{
    (void)sig;
    shutdown_requested = 1;
}

/* Find client index in connected clients array */
static int find_client_index(pid_t pid)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared_mem->clients[i].allocated && shared_mem->clients[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

/* Send message to a specific client */
static void send_message_to_client(pid_t pid, MessageType type, uint32_t offset, uint32_t length)
{
    mqd_t client_queue;
    QueueMessage msg;
    char queue_name[64];

    snprintf(queue_name, sizeof(queue_name), "/chat_queue_%d", pid);
    client_queue = mq_open(queue_name, O_WRONLY | O_NONBLOCK);
    if (client_queue < 0) {
        print_timestamp();
        printf("Cannot open queue to send a message to the client.\n");
        return;
    }

    msg.mtype = type;
    msg.client_pid = 0;
    msg.offset = offset;
    msg.length = length;
    memset(msg.data, 0, sizeof(msg.data));

    print_timestamp();
    printf("Sending message of length %d to client %d.\n", length, pid);

    mq_send(client_queue, (const char *)&msg, sizeof(msg), 0);
    mq_close(client_queue);
}

/* Handle new client connection */
static void handle_client_connect(pid_t client_pid)
{
    if (shared_mem->client_count >= MAX_CLIENTS) {
        print_timestamp();
        printf("Server refused a client connection, too many clients already connected.\n");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!shared_mem->clients[i].allocated) {
            shared_mem->clients[i].pid = client_pid;
            shared_mem->clients[i].allocated = 1;
            shared_mem->client_count++;

            print_timestamp();
            printf("Client connect + client ID %d\n", client_pid);
            return;
        }
    }
}

/* Handle client disconnection */
static void handle_client_disconnect(pid_t client_pid)
{
    int idx = find_client_index(client_pid);
    if (idx >= 0) {
        shared_mem->clients[idx].allocated = 0;
        shared_mem->client_count--;

        print_timestamp();
        printf("Client disconnected with client ID %d\n", client_pid);
    }
}

/* Broadcast message to all other connected clients */
static void broadcast_message(pid_t sender_pid, uint32_t offset, uint32_t length)
{
    print_timestamp();
    printf("Broadcasting message\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared_mem->clients[i].allocated && shared_mem->clients[i].pid != sender_pid) {
            send_message_to_client(shared_mem->clients[i].pid, MSG_BROADCAST, offset, length);
        }
    }

    print_timestamp();
    printf("Finished broadcasting, all clients acknowledged with the list of clients.\n");
}

/* Process messages from the global queue */
static void process_global_queue(void)
{
    QueueMessage msg;
    unsigned int priority;
    int ret;

    ret = mq_receive(global_queue, (char *)&msg, sizeof(msg), &priority);
    if (ret < 0) {
        if (errno != EAGAIN) {
            perror("mq_receive");
        }
        return;
    }

    switch (msg.mtype) {
        case MSG_CLIENT_CONNECT:
            handle_client_connect(msg.client_pid);
            break;
        case MSG_CLIENT_DISCONNECT:
            handle_client_disconnect(msg.client_pid);
            break;
        case MSG_STRING_AVAILABLE:
            print_timestamp();
            printf("Message received + client ID %d\n", msg.client_pid);
            broadcast_message(msg.client_pid, msg.offset, msg.length);
            break;
        default:
            break;
    }
}

/* Main server loop */
int main(void)
{
    struct mq_attr attr;
    struct sigaction sa;

    /* Register signal handler for Ctrl-C */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    // sa.sa_flags = SA_SIGINFO; // Use with sa.sa_sigaction to get more info
    sigaction(SIGINT, &sa, NULL);

    /* Create or open shared memory */
    shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd < 0) {
        fprintf(stderr, "Another instance already running or failed to create shared memory.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Set size of shared memory */
    if (ftruncate(shm_fd, sizeof(SharedMemory)) < 0) {
        perror("ftruncate");
        cleanup();
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

    /* Initialize shared memory */
    memset(shared_mem, 0, sizeof(SharedMemory));
    shared_mem->initialized = 1;

    print_timestamp();
    printf("Map open\n");

    /* Create global message queue */
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(QueueMessage);
    attr.mq_curmsgs = 0;

    global_queue = mq_open(GLOBAL_QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (global_queue < 0) {
        perror("mq_open");
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Main event loop */
    while (!shutdown_requested) {
        process_global_queue();
        usleep(100000);  /* Sleep 100ms between iterations */
    }

    /* Disconnect all remaining clients */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared_mem->clients[i].allocated) {
            send_message_to_client(shared_mem->clients[i].pid, MSG_DISCONNECT_REQUEST, 0, 0);
        }
    }

    usleep(DISCONNECT_TIMEOUT_MS * 1000);  /* Wait for acknowledgments */

    cleanup();
    return EXIT_SUCCESS;
}
