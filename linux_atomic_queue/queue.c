#include <stdlib.h>
#include <stdatomic.h>
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
    Queue* queue = malloc(sizeof(Queue));
    if (!queue) {
        return NULL;
    }

    QueueNode* sentinel = malloc(sizeof(QueueNode));
    if (!sentinel) {
        free(queue);
        return NULL;
    }

    sentinel->value = NULL;
    atomic_store(&sentinel->next, NULL);

    atomic_store(&queue->head, sentinel);
    atomic_store(&queue->tail, sentinel);

    return queue;
}

/*
 * QueueEnqueue: Add a value to the queue using Michael-Scott algorithm.
 *
 * The Michael-Scott algorithm is a lock-free queue implementation that uses
 * only atomic compare-and-swap operations. We atomically swing the tail
 * pointer to the new node.
 *
 * Parameters:
 *   queue: Pointer to the queue
 *   value: Pointer value to enqueue (void*)
 */
void QueueEnqueue(Queue* queue, void* value)
{
    QueueNode* new_node = malloc(sizeof(QueueNode));
    new_node->value = value;
    atomic_store(&new_node->next, NULL);

    while (1) {
        QueueNode* tail = atomic_load(&queue->tail);
        QueueNode* next = atomic_load(&tail->next);

        /* Check that tail is still the same (avoid ABA problem). */
        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                /* Try to link new node at the end. */
                if (atomic_compare_exchange_strong(&tail->next, &next, new_node)) {
                    /* Success: now swing tail pointer to new node. */
                    atomic_compare_exchange_strong(&queue->tail, &tail, new_node);
                    return;
                }
            }
            else {
                /* Tail is lagging behind, help advance it. */
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            }
        }
    }
}

/*
 * QueueDequeue: Remove and return the value at the front of the queue.
 *
 * Uses the Michael-Scott algorithm. The sentinel node ensures there is always
 * at least one node in the queue, simplifying the logic.
 *
 * Parameters:
 *   queue: Pointer to the queue
 *   out_value: Pointer to store the dequeued value
 *
 * Returns: 1 if dequeue was successful, 0 if queue is empty.
 */
int QueueDequeue(Queue* queue, void** out_value)
{
    while (1) {
        QueueNode* head = atomic_load(&queue->head);
        QueueNode* tail = atomic_load(&queue->tail);
        QueueNode* next = atomic_load(&head->next);

        /* Check that head is still the same. */
        if (head == atomic_load(&queue->head)) {
            if (head == tail) {
                if (next == NULL) {
                    /* Queue is empty. */
                    return 0;
                }
                /* Tail is lagging, help advance it. */
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            }
            else {
                /* Read value before CAS to avoid use-after-free. */
                void* value = next->value;

                if (atomic_compare_exchange_strong(&queue->head, &head, next)) {
                    free(head);  /* Free the old sentinel/node. */
                    *out_value = value;
                    return 1;
                }
            }
        }
    }
}

/*
 * QueueIsEmpty: Check if the queue is empty without removing elements.
 *
 * Parameters:
 *   queue: Pointer to the queue
 *
 * Returns: 1 if empty, 0 if not empty.
 */
int QueueIsEmpty(Queue* queue)
{
    QueueNode* head = atomic_load(&queue->head);
    QueueNode* next = atomic_load(&head->next);
    return next == NULL;
}

/*
 * QueueDestroy: Free all allocated memory for the queue.
 *
 * Parameters:
 *   queue: Pointer to the queue to destroy
 */
void QueueDestroy(Queue* queue)
{
    if (!queue) {
        return;
    }

    QueueNode* current = atomic_load(&queue->head);
    while (current) {
        QueueNode* next = atomic_load(&current->next);
        free(current);
        current = next;
    }

    free(queue);
}
