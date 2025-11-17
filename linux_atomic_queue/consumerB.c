#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <math.h>
#include <sched.h>
#include <time.h>
#include "queue.h"
#include "shared.h"

/* Forward declarations for queues (defined in main.c). */
extern Queue* queueB;

/*
 * IsPrime: Check whether a number is prime using trial division.
 *
 * A prime number is only divisible by 1 and itself. We check divisibility
 * up to the square root of the number for efficiency.
 *
 * Parameters:
 *   number: The number to check (must be positive)
 *
 * Returns: 1 if prime, 0 if not prime.
 * Note that numbers less than 2 are not prime.
 * See https://en.wikipedia.org/wiki/Prime_number
 */
static int IsPrime(int32_t number)
{
    if (number < 2) {
        return 0;
    }

    if (number == 2) {
        return 1;
    }

    if (number % 2 == 0) {
        return 0;
    }

    int32_t limit = (int32_t)sqrt((double)number) + 1;

    for (int32_t i = 3; i <= limit; i += 2) {
        if (number % i == 0) {
            return 0;
        }
    }

    return 1;
}

/*
 * ConsumerBThread: Process odd numbers and check if they are prime.
 *
 * Consumer B dequeues numbers from its queue, checks if each is prime,
 * and enqueues a ConsumerBMessage to the result queue. It continues until
 * the producer is finished and its queue is empty.
 *
 * Parameters:
 *   arg: Unused (NULL)
 *
 * Returns: NULL (thread exit code)
 */
void* ConsumerBThread(void* arg)
{
    (void)arg;  /* Mark parameter as intentionally unused. */

    while (1) {
        /* Try to dequeue a number pointer from consumer B's queue. */
        void* vp = NULL;
        if (QueueDequeue(queueB, &vp)) {
            int32_t number = *(int32_t*)vp;
            free(vp);

            int is_prime = IsPrime(number);

            /* Create and enqueue a typed message for the main thread. */
            ConsumerBMessage* msg = malloc(sizeof(ConsumerBMessage));
            if (!msg) {
                continue;
            }
            msg->number = number;
            msg->isPrime = is_prime;
            /* Capture the time this message was created/sent. */
            clock_gettime(CLOCK_REALTIME, &msg->sendTime);
            QueueEnqueue(resultQueueB, msg);

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
