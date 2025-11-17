#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include "queue.h"

/*
 * QueueCreate: Initialize a new lock-free queue with sentinel node.
 *
 * A sentinel (dummy) node at the head improves performance by eliminating
 * a special case in the dequeue operation. The queue starts with head and
 * tail both pointing to the sentinel node.
 *
 * Returns: Pointer to newly allocated queue, or NULL on allocation failure.
 */
Queue* QueueCreate(void)
{
    Queue *queue = malloc(sizeof(Queue));
    if (!queue) {
        return NULL;
    }

    QueueNode *sentinel = malloc(sizeof(QueueNode));
    if (!sentinel) {
        free(queue);
        return NULL;
    }

    sentinel->value = NULL;
    atomic_store_explicit(&sentinel->next, NULL, memory_order_relaxed);
    atomic_store_explicit(&queue->head, sentinel, memory_order_relaxed);
    atomic_store_explicit(&queue->tail, sentinel, memory_order_release);

    return queue;
}

/*
 * QueueEnqueue: Add a value to the queue using Michael-Scott algorithm.
 *
 * The Michael-Scott algorithm is a lock-free queue that uses atomic
 * compare-and-swap (CAS) operations to safely link nodes. This function:
 *   1. Allocates a new node with the value.
 *   2. Atomically tries to link it at the tail.
 *   3. Helps advance the tail pointer if needed.
 *   4. Retries on CAS failure (spin-loop).
 *
 * Memory ordering:
 *   - Use acquire_release to ensure visibility across threads.
 *   - Use release on tail swing to guarantee other threads see the new node.
 *
 * Parameters:
 *   queue: Pointer to the queue
 *   value: Pointer value to enqueue (void*)
 */
void QueueEnqueue(Queue *queue, void *value)
{
    QueueNode *new_node = malloc(sizeof(QueueNode));
    new_node->value = value;
    atomic_store_explicit(&new_node->next, NULL, memory_order_relaxed);

    while (1) {
        QueueNode *tail = atomic_load_explicit(&queue->tail,
                                                memory_order_acquire);
        QueueNode *next = atomic_load_explicit(&tail->next,
                                               memory_order_acquire);

        /* Re-check tail hasn't changed (avoid ABA). */
        QueueNode *tail_check = atomic_load_explicit(&queue->tail,
                                                      memory_order_acquire);
        if (tail != tail_check) {
            continue;  /* Tail changed, retry. */
        }

        if (next == NULL) {
            /* Try to link new_node at the end of the list. */
            if (atomic_compare_exchange_strong_explicit(
                    &tail->next, &next, new_node,
                    memory_order_release, memory_order_acquire)) {
                /* Success: now swing tail to new_node. */
                atomic_compare_exchange_strong_explicit(
                    &queue->tail, &tail, new_node,
                    memory_order_release, memory_order_acquire);
                return;
            }
            /* CAS failed; tail->next changed, retry. */
        }
        else {
            /* Tail is lagging behind; help advance it. */
            atomic_compare_exchange_strong_explicit(
                &queue->tail, &tail, next,
                memory_order_release, memory_order_acquire);
        }
    }
}

/*
 * QueueDequeue: Remove and return the value at the front of the queue.
 *
 * Uses the Michael-Scott algorithm with safe memory reclamation:
 *   1. Load head and next pointers.
 *   2. Check head hasn't changed (avoid ABA).
 *   3. If queue is empty, return 0.
 *   4. Extract value before CAS (avoid use-after-free).
 *   5. Atomically swing head to next node.
 *   6. Free the old head (was either sentinel or previous node).
 *
 * Memory ordering:
 *   - Use acquire_release to ensure correct visibility.
 *   - Read value BEFORE the CAS to prevent use-after-free.
 *
 * Parameters:
 *   queue: Pointer to the queue
 *   out_value: Pointer to store the dequeued value
 *
 * Returns: 1 if dequeue was successful, 0 if queue is empty.
 */
int QueueDequeue(Queue *queue, void **out_value)
{
    while (1) {
        QueueNode *head = atomic_load_explicit(&queue->head,
                                               memory_order_acquire);
        QueueNode *tail = atomic_load_explicit(&queue->tail,
                                               memory_order_acquire);
        QueueNode *next = atomic_load_explicit(&head->next,
                                               memory_order_acquire);

        /* Re-check head hasn't changed. */
        QueueNode *head_check = atomic_load_explicit(&queue->head,
                                                      memory_order_acquire);
        if (head != head_check) {
            continue;  /* Head changed, retry. */
        }

        if (head == tail) {
            if (next == NULL) {
                /* Queue is empty. */
                return 0;
            }
            /* Tail is lagging behind; help advance it. */
            atomic_compare_exchange_strong_explicit(
                &queue->tail, &tail, next,
                memory_order_release, memory_order_acquire);
        }
        else {
            /* Read value BEFORE CAS (critical to avoid use-after-free). */
            void *value = next->value;

            /* Try to swing head to next node. */
            if (atomic_compare_exchange_strong_explicit(
                    &queue->head, &head, next,
                    memory_order_release, memory_order_acquire)) {
                /* Success: free old head and return value. */
                free(head);
                *out_value = value;
                return 1;
            }
            /* CAS failed; head changed, retry. */
        }
    }
}

/*
 * QueueIsEmpty: Check if the queue is empty without removing elements.
 *
 * Returns: 1 if empty, 0 if not empty.
 */
int QueueIsEmpty(Queue *queue)
{
    QueueNode *head = atomic_load_explicit(&queue->head,
                                           memory_order_acquire);
    QueueNode *next = atomic_load_explicit(&head->next,
                                           memory_order_acquire);
    return next == NULL;
}

/*
 * QueueDestroy: Free all allocated memory for the queue.
 *
 * Parameters:
 *   queue: Pointer to the queue to destroy
 */
void QueueDestroy(Queue *queue)
{
    if (!queue) {
        return;
    }

    QueueNode *current = atomic_load_explicit(&queue->head,
                                              memory_order_relaxed);
    while (current) {
        QueueNode *next = atomic_load_explicit(&current->next,
                                               memory_order_relaxed);
        free(current);
        current = next;
    }

    free(queue);
}
