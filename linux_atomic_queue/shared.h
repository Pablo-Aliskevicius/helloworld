#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <stdatomic.h>
#include <time.h>
#include "queue.h"

/* Shared state between main thread and producer thread. */
typedef struct
{
    _Atomic(int32_t) totalCount;    /* Total numbers read from file */
    _Atomic(int) producerFinished;  /* Flag: producer has finished */
} ProducerState;

/* Atomic counter tracking numbers enqueued but not yet processed by consumers. */
extern _Atomic(int32_t) inFlightCount;

/* Message from consumer A containing a number, its square, and send timestamp. */
typedef struct ConsumerAMessage
{
    int32_t number;
    int64_t square;
    struct timespec sendTime;  /* Time when message was created. */
} ConsumerAMessage;

/* Message from consumer B containing a number, prime check, and send timestamp. */
typedef struct ConsumerBMessage
{
    int32_t number;
    int isPrime;
    struct timespec sendTime;  /* Time when message was created. */
} ConsumerBMessage;

/* Shared result queue for consumer A messages. */
extern Queue* resultQueueA;

/* Shared result queue for consumer B messages. */
extern Queue* resultQueueB;

/* Shared producer state. */
extern ProducerState producerState;

/* Flag to signal consumers to exit. */
extern _Atomic(int) shouldExit;

#endif /* SHARED_H */
