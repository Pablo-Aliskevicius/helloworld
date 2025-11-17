#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "shared.h"

/* Forward declarations for queues (defined in main.c). */
extern Queue* queueA;
extern Queue* queueB;

/*
 * ProducerThread: Read integers from file and distribute to consumer queues.
 *
 * The producer thread reads numbers from a file. Even numbers are sent to
 * consumer A's queue, odd numbers to consumer B's queue. After reading all
 * numbers, it atomically updates the shared state and signals consumers to exit.
 *
 * Parameters:
 *   arg: Pointer to filename string (const char*)
 *
 * Returns: NULL (thread exit code)
 */
void* ProducerThread(void* arg)
{
    const char* filename = (const char*)arg;
    FILE* file = fopen(filename, "r");

    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    int32_t number;
    int32_t count = 0;

    /* Read integers from file and distribute based on even/odd. */
    while (fscanf(file, "%d", &number) == 1) {
        int32_t* p = malloc(sizeof(int32_t));
        if (!p) {
            break;
        }
        *p = number;

        if (number % 2 == 0) {
            /* Even number: send to consumer A. */
            QueueEnqueue(queueA, p);
        }
        else {
            /* Odd number: send to consumer B. */
            QueueEnqueue(queueB, p);
        }

        /* Increment global in-flight counter to indicate work outstanding. */
        extern _Atomic(int32_t) inFlightCount;
        atomic_fetch_add_explicit(&inFlightCount, 1, memory_order_acq_rel);
        count++;
        struct timespec ts = { 0, 10000 }; /* 100 microseconds */
        nanosleep(&ts, NULL);
    }

    fclose(file);

    /*
     * Signal completion using atomic operations.
     * Store the total count of numbers read.
     */
    atomic_store(&producerState.totalCount, count);

    /*
     * Set the finished flag to signal main thread.
     * Use memory_order_release to ensure all prior stores are visible.
     */
    atomic_store_explicit(&producerState.producerFinished, 1, memory_order_release);

    /*
     * Signal consumer threads to exit by setting the global exit flag.
     * Use memory_order_release to ensure visibility.
     */
    atomic_store_explicit(&shouldExit, 1, memory_order_release);

    return NULL;
}
