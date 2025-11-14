/*
C11 threads version of the line-processing program.

Build (Linux, gcc, release):
   gcc -std=gnu11 -Wall -Wextra -O2 -pthread -o line_counters main_using_threads.c

Remove the -O2 optimization and add -g for debugging. Otherwise won't stop at breakpoints.
   gcc -std=gnu11 -Wall -Wextra -g -pthread -o line_counters main_using_threads.c

Run:
  ./line_counters input.txt
*/
#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

/* linked queue node */
typedef struct node {// match return type of getline.
    char* line;
    struct node* next;
} node_t;

/* thread-safe FIFO queue */
typedef struct {
    node_t* head;
    node_t* tail;
    mtx_t mutex;
    cnd_t cond;
} queue_t;

static void queue_init(queue_t* q) {
    q->head = q->tail = NULL;
    mtx_init(&q->mutex, (int) mtx_plain); // from a nameless enum
    cnd_init(&q->cond);
}

/* Note: queue_destroy assumes queue is drained (no outstanding nodes) */
static void queue_destroy(queue_t* q) {
    mtx_destroy(&q->mutex);
    cnd_destroy(&q->cond);
}
// match return type of getline.
static int queue_enqueue(queue_t* q, char* line) {
    node_t* n = malloc(sizeof(node_t));
    if (!n) return -1;
    n->line = line;
    n->next = NULL;

    mtx_lock(&q->mutex);
    if (q->tail) {
        q->tail->next = n;
        q->tail = n;
    }
    else {
        q->head = q->tail = n;
    }
    cnd_signal(&q->cond);
    mtx_unlock(&q->mutex);
    return 0;
}

/* Blocks until an item is available. Returns ownership of returned char* (may be NULL sentinel). */
static char* queue_dequeue(queue_t* q) {
    mtx_lock(&q->mutex);
    while (q->head == NULL) {
        cnd_wait(&q->cond, &q->mutex);
    }
    node_t* n = q->head;
    q->head = n->next;
    if (q->head == NULL) 
        q->tail = NULL;
    mtx_unlock(&q->mutex);

    char* line = n->line;
    free(n);
    return line;
}

/* Counting helpers */
static size_t count_words(const char* s) {
    size_t words = 0;
    int in_word = 0;
    for (; *s; ++s) {
        if (!isspace((unsigned char)*s)) {
            if (!in_word) { in_word = 1; ++words; }
        }
        else {
            in_word = 0;
        }
    }
    return words;
}

static size_t count_chars(const char* s) {
    size_t chars = 0;
    for (; *s; ++s) {
        if (*s != '\n') ++chars;
    }
    return chars;
}

static size_t count_vowels(const char* s) {
    size_t v = 0;
    for (; *s; ++s) {
        char c = tolower((unsigned char)*s);
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') ++v;
    }
    return v;
}

/* Thread argument */
typedef struct {
    queue_t* queue;
    int mode; /* 1 = words, 2 = chars, 3 = vowels */
    unsigned long long total;
} worker_arg_t;

/* Thread entry (C11 thrd_start_t) */
static int worker(void* arg) {
    worker_arg_t* w = (worker_arg_t*)arg;
    unsigned long long total = 0ULL;
    printf("Thread mode %d running in process %d\n", w->mode, (int) getpid());
    for (;;) {
        char* line = queue_dequeue(w->queue);
        if (line == NULL) break; /* sentinel => no more data */

        size_t c = 0;
        if (w->mode == 1) c = count_words(line);
        else if (w->mode == 2) c = count_chars(line);
        else if (w->mode == 3) c = count_vowels(line);

        total += c;
        free(line);
    }

    w->total = total;
    printf("Thread mode %d running in process %d returning total %lld\n", w->mode, (int) getpid(), w->total);
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) {
        fprintf(stderr, "Failed to open '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    printf("%s running with pid=%d on file %s\n", argv[0], (int) getpid(), argv[1]);
    queue_t queues[3];
    for (int i = 0; i < 3; ++i) queue_init(&queues[i]);

    thrd_t threads[3];
    worker_arg_t args[3];
    for (int i = 0; i < 3; ++i) {
        args[i].queue = &queues[i];
        args[i].mode = i + 1; /* 1=words,2=chars,3=vowels */
        args[i].total = 0;
        if (thrd_create(&threads[i], worker, &args[i]) != thrd_success) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            fclose(f);
            return EXIT_FAILURE;
        }
    }

    /* Read lines and distribute round-robin using getline (POSIX) */
    char* linebuf = NULL;
    size_t cap = 0;
    ssize_t nread; 
    int idx = 0;
    size_t line_count = 0;

    while ((nread = getline(&linebuf, &cap, f)) != -1) {
        char* copy = strdup(linebuf);
        if (!copy) {
            fprintf(stderr, "Out of memory\n");
            free(linebuf);
            fclose(f);
            return EXIT_FAILURE;
        }
        if (queue_enqueue(&queues[idx], copy) != 0) {
            fprintf(stderr, "Failed to enqueue line\n");
            free(copy);
            free(linebuf);
            fclose(f);
            return EXIT_FAILURE;
        }
        idx = (idx + 1) % 3;
        ++line_count;
    }
    free(linebuf);
    fclose(f);

    /* Send sentinel (NULL) to each queue to signal completion */
    for (int i = 0; i < 3; ++i) {
        queue_enqueue(&queues[i], NULL);
    }

    /* Join threads and collect totals */
    unsigned long long totals[3] = { 0,0,0 };
    for (int i = 0; i < 3; ++i) {
        thrd_join(threads[i], NULL);
        totals[i] = args[i].total;
        queue_destroy(&queues[i]);
    }

    printf("One third words   : %llu\n", totals[0]);
    printf("One third chars   : %llu\n", totals[1]);
    printf("One third vowels  : %llu\n", totals[2]);
    printf("Total lines   : %zu\n", line_count);

    return EXIT_SUCCESS;
}