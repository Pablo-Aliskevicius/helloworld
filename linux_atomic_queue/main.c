#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "queue.h"
#include "shared.h"

#include <inttypes.h>

/* Global queues for inter-thread communication. */
Queue* queueA = NULL;
Queue* queueB = NULL;

/* Global result queues for consumer outputs. */
Queue* resultQueueA = NULL;
Queue* resultQueueB = NULL;

/* Global producer state. */
ProducerState producerState = {
    .totalCount = 0,
    .producerFinished = 0
};

/* Global flag to signal consumers to exit. */
_Atomic(int) shouldExit = 0;

/* Global in-flight counter definition. */
_Atomic(int32_t) inFlightCount = 0;

/* Forward declarations. */
extern void* ProducerThread(void* arg);
extern void* ConsumerAThread(void* arg);
extern void* ConsumerBThread(void* arg);

/*
 * TimestampCmp: Compare two struct timespec values.
 * Returns: -1 if a < b, 0 if a == b, 1 if a > b
 */
static int TimestampCmp(struct timespec a, struct timespec b)
{
    if (a.tv_sec != b.tv_sec) {
        return a.tv_sec < b.tv_sec ? -1 : 1;
    }
    if (a.tv_nsec != b.tv_nsec) {
        return a.tv_nsec < b.tv_nsec ? -1 : 1;
    }
    return 0;
}

/*
 * FormatTimestamp: Format a timespec as YYYY-MM-DD HH:MM:SS.mmm
 */
static void FormatTimestamp(struct timespec ts, char* buf, size_t bufSize)
{
    time_t sec = (time_t)ts.tv_sec;
    struct tm tm;
    localtime_r(&sec, &tm);

    size_t len = strftime(buf, bufSize, "%Y-%m-%d %H:%M:%S", &tm);
    if (len >= bufSize)
        return;

    int microsecs = (int)(ts.tv_nsec / 1000);
    snprintf(buf + len, bufSize - len, ".%06d", microsecs);
}

/*
 * PrintResultsLive: Print messages from both consumers in the order they were sent.
 *
 * Uses a merge algorithm: keep track of the next message from each queue and
 * always print whichever has the earlienanosleepst timestamp. This ensures results appear
 * in the order they were generated, not the order they happen to be dequeued.
 */
static void PrintResultsLive(void)
{
    int printedFinished = 0;
    ConsumerAMessage* msgA = NULL;
    ConsumerBMessage* msgB = NULL;
    int hasA = 0, hasB = 0;

    for (;;) {
        int didWork = 0;

        /* Try to fetch next message from A if we don't have one. */
        if (!hasA) {
            void* vp = NULL;
            if (QueueDequeue(resultQueueA, &vp)) {
                msgA = (ConsumerAMessage*)vp;
                if (msgA) {
                    hasA = 1;
                }
                didWork = 1;
            }
        }

        /* Try to fetch next message from B if we don't have one. */
        if (!hasB) {
            void* vp = NULL;
            if (QueueDequeue(resultQueueB, &vp)) {
                msgB = (ConsumerBMessage*)vp;
                if (msgB) {
                    hasB = 1;
                }
                didWork = 1;
            }
        }

        /* Merge: print whichever message has the earlier timestamp. */
        if (hasA && hasB) {
            if (TimestampCmp(msgA->sendTime, msgB->sendTime) <= 0) {
                char tsbuf[64];
                FormatTimestamp(msgA->sendTime, tsbuf, sizeof(tsbuf));
                printf("[%s] %d x %d = %" PRId64 "\n", tsbuf, msgA->number, msgA->number, msgA->square);
                free(msgA);
                msgA = NULL;
                hasA = 0;
            }
            else {
                char tsbuf[64];
                FormatTimestamp(msgB->sendTime, tsbuf, sizeof(tsbuf));
                if (msgB->isPrime) {
                    printf("[%s] %d is prime\n", tsbuf, msgB->number);
                }
                else {
                    printf("[%s] %d is not prime\n", tsbuf, msgB->number);
                }
                free(msgB);
                msgB = NULL;
                hasB = 0;
            }
        }
        else if (hasA) {
            char tsbuf[64];
            FormatTimestamp(msgA->sendTime, tsbuf, sizeof(tsbuf));
            printf("[%s] %d x %d = %" PRId64 "\n", tsbuf, msgA->number, msgA->number, msgA->square);
            free(msgA);
            msgA = NULL;
            hasA = 0;
        }
        else if (hasB) {
            char tsbuf[64];
            FormatTimestamp(msgB->sendTime, tsbuf, sizeof(tsbuf));
            if (msgB->isPrime) {
                printf("[%s] %d is prime\n", tsbuf, msgB->number);
            }
            else {
                printf("[%s] %d is not prime\n", tsbuf, msgB->number);
            }
            free(msgB);
            msgB = NULL;
            hasB = 0;
        }

        /* Print producer finished message after all messages are printed. */
        if (!printedFinished && atomic_load_explicit(&producerState.producerFinished, memory_order_acquire)
            && atomic_load_explicit(&inFlightCount, memory_order_acquire) == 0
            && !hasA && !hasB
            && QueueIsEmpty(resultQueueA) && QueueIsEmpty(resultQueueB)) {
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            char tsbuf[64];
            FormatTimestamp(now, tsbuf, sizeof(tsbuf));
            int32_t total_count = atomic_load(&producerState.totalCount);
            printf("[%s] Finished reading the file, %d numbers read\n", tsbuf, total_count);
            printedFinished = 1;
        }

        /* Termination: all sources done and no pending messages and finished printed. */
        if (printedFinished
            && atomic_load_explicit(&producerState.producerFinished, memory_order_acquire)
            && atomic_load_explicit(&inFlightCount, memory_order_acquire) == 0
            && !hasA && !hasB
            && QueueIsEmpty(resultQueueA) && QueueIsEmpty(resultQueueB)) {
            break;
        }

        if (!didWork) {
            struct timespec ts = { 0, 1000000 };
            nanosleep(&ts, NULL);
        }
    }
}

/*
 * Main: Initialize threads, coordinate execution, and display results.
 *
 * Creates and joins three worker threads (producer, consumer A, consumer B).
 * The main thread waits for the producer to finish, then processes results
 * from both consumers using atomic operations for synchronization.
 *
 * Parameters:
 *   argc: Argument count
 *   argv: Command line arguments (argv[1] should be the input filename)
 *
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Initialize the queues. */
    queueA = QueueCreate();
    queueB = QueueCreate();
    resultQueueA = QueueCreate();
    resultQueueB = QueueCreate();

    if (!queueA || !queueB || !resultQueueA || !resultQueueB) {
        fprintf(stderr, "Error: Failed to create queues\n");
        return EXIT_FAILURE;
    }

    /* Create thread IDs. */
    pthread_t producer_tid, consumerA_tid, consumerB_tid;

    /* Create producer thread. */
    if (pthread_create(&producer_tid, NULL, ProducerThread, argv[1]) != 0) {
        perror("Error creating producer thread");
        return EXIT_FAILURE;
    }

    /* Create consumer A thread. */
    if (pthread_create(&consumerA_tid, NULL, ConsumerAThread, NULL) != 0) {
        perror("Error creating consumer A thread");
        return EXIT_FAILURE;
    }

    /* Create consumer B thread. */
    if (pthread_create(&consumerB_tid, NULL, ConsumerBThread, NULL) != 0) {
        perror("Error creating consumer B thread");
        return EXIT_FAILURE;
    }

    /* Print results live as they arrive and wait for completion condition. */
    PrintResultsLive();

    /* Wait for consumer and producer threads to finish cleanly. */
    pthread_join(consumerA_tid, NULL);
    pthread_join(consumerB_tid, NULL);
    pthread_join(producer_tid, NULL);

    /* Clean up allocated resources. */
    QueueDestroy(queueA);
    QueueDestroy(queueB);
    QueueDestroy(resultQueueA);
    QueueDestroy(resultQueueB);

    return EXIT_SUCCESS;
}
