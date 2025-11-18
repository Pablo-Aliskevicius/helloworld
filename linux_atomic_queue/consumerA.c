#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include "queue.h"
#include "shared.h"

/* Forward declarations for queues (defined in main.c). */
extern Queue* queueA;

/*
 * SquareNumber: Calculate the square of a 32-bit integer.
 *
 * Parameters:
 *   number: The number to square
 *
 * Returns: The square as a 64-bit integer to avoid overflow.
 */
static int64_t SquareNumber(int32_t number)
{
    return (int64_t)number * number;
}

/*
 * ConsumerAThread: Process even numbers and calculate their squares.
 *
 * Consumer A dequeues numbers from its queue, calculates the square of each,
 * and enqueues a ConsumerAMessage to the result queue. It continues until
 * the producer is finished and its queue is empty.
 *
 * Parameters:
 *   arg: Unused (NULL)
 *
 * Returns: NULL (thread exit code)
 */
void* ConsumerAThread(void* arg)
{
    (void)arg;  /* Mark parameter as intentionally unused. */

    while (1) {
        /* Try to dequeue a number pointer from consumer A's queue. */
        void* vp_number = NULL;
        if (QueueDequeue(queueA, &vp_number)) {
            int32_t number = *(int32_t*)vp_number;
            free(vp_number);

            int64_t square = SquareNumber(number);

            /* Create and enqueue a typed message for the main thread. */
            ConsumerAMessage* msg = malloc(sizeof(ConsumerAMessage));
            if (!msg) {
                continue;
            }
            msg->number = number;
            msg->square = square;
            /* Capture the time this message was created/sent. */
            clock_gettime(CLOCK_REALTIME, &msg->sendTime);
            QueueEnqueue(resultQueueA, msg);

            /* Decrement in-flight count to mark this work item processed. */
            extern _Atomic(int32_t) inFlightCount;
            atomic_fetch_sub_explicit(&inFlightCount, 1, memory_order_acq_rel);
        }
        else {
            /*
             * Queue is empty. Check if producer is finished.
             * Use memory_order_acquire to see all producer writes.
             */
            if (atomic_load_explicit(&producerState.producerFinished,
                                     memory_order_acquire)) {
                /* Producer finished and queue is empty, exit. */
                break;
            }

            /* Yield to reduce busy-waiting. */
            sched_yield();
        }
    }

    return NULL;
}
