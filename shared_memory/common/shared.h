#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <time.h>
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/types.h>

/* IPC Constants */
#define SHARED_MEMORY_NAME "/shared_memory_chat"
#define GLOBAL_QUEUE_NAME "/71dcbdfc-8b5c-45b6-93cf-5e961df6f4f4-listener"
#define MAX_CLIENTS 5
#define MAX_MESSAGE_SIZE 1024
#define SHARED_MEMORY_SIZE (MAX_CLIENTS * MAX_MESSAGE_SIZE * 2)

/* Special messages */
#define DISCONNECT_MESSAGE "6b540d1b-cd12-4bd9-bdfd-64cbcf1ed258"
#define DISCONNECT_ACK_MESSAGE "disconnect-ack"
#define DISCONNECT_TIMEOUT_MS 100

/* Message types for queue communication */
typedef enum {
    MSG_CLIENT_CONNECT = 1,
    MSG_CLIENT_DISCONNECT = 2,
    MSG_STRING_AVAILABLE = 3,
    MSG_STRING_ACK = 4,
    MSG_BROADCAST = 5,
    MSG_DISCONNECT_REQUEST = 6,
    MSG_DISCONNECT_ACK = 7
} MessageType;

/* Message structure for POSIX message queues */
typedef struct {
    long mtype;           /* message type */
    pid_t client_pid;     /* client process ID */
    uint32_t offset;      /* offset in shared memory */
    uint32_t length;      /* length of message */
    char data[256];       /* additional data */
} QueueMessage;

/* Client state structure in shared memory */
typedef struct {
    pid_t pid;            /* client process ID */
    uint32_t allocated;   /* 1 if slot is allocated, 0 otherwise */
} ClientSlot;

/* Shared memory layout */
typedef struct {
    uint32_t initialized; /* 1 if initialized, 0 otherwise */
    uint32_t client_count; /* number of connected clients */
    ClientSlot clients[MAX_CLIENTS]; /* client slots */
    char data[SHARED_MEMORY_SIZE]; /* shared data area */
} SharedMemory;

/* Utility function declarations */
void print_timestamp(void);
char* get_queue_name(pid_t pid);

#endif /* SHARED_H */
