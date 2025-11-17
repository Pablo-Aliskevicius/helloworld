#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <stdatomic.h>

/* Lock-free queue node structure for holding integer data. */
typedef struct QueueNode
{
    void* value;
    _Atomic(struct QueueNode*) next;
} QueueNode;

/* Lock-free queue structure using atomic operations for synchronization. */
typedef struct
{
    _Atomic(QueueNode*) head;
    _Atomic(QueueNode*) tail;
} Queue;

/* Initialize a lock-free queue. */
Queue* QueueCreate(void);

/* Enqueue a value into the queue atomically. */
void QueueEnqueue(Queue* queue, void* value);

/* Dequeue a value from the queue atomically. Returns 1 if successful, 0 if empty. */
int QueueDequeue(Queue* queue, void** out_value);

/* Check if queue is empty. */
int QueueIsEmpty(Queue* queue);

/* Free the queue and all nodes. */
void QueueDestroy(Queue* queue);

#endif /* QUEUE_H */
