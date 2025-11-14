/*
A multithreaded C program that:
- Reads a text file specified on the command line
- Distributes lines to three per-thread queues in round-robin order
- Thread 1 counts words, Thread 2 counts characters, Thread 3 counts vowels (1/3 of lines each)
- Prints totals for each at the end

Compile:
  gcc -std=gnu11 -pthread -o line_counters main.c
Run:
  ./line_counters input.txt
*/

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> // E:\Users\User\vcpkg\packages\pthreads_x64-windows\include
#include <ctype.h>
#include <errno.h>

/* Simple linked queue holding char* lines (NULL line is sentinel) */
typedef struct node {
	// To make it generic, we could have void* data.
    char* line;
    struct node* next;
} node_t;

// Function main() adds nodes to the queue, worker_thread() removes them.
// As a result, we need to ensure thread safety with mutexes and condition variables.
typedef struct {
    node_t* head;
    node_t* tail;
    pthread_mutex_t mutex;
	// A pthread condition variable is roughly similar to a Windows event object.
    pthread_cond_t cond_input_is_available;
} queue_t;

// In C we don't have constructors/destructors, so we need init/destroy functions.
// We also use static functions to limit their scope to this file and avoid name clashes.
static void queue_init(queue_t* q) {
    q->head = q->tail = NULL;
    // Default attributes are fine.
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_input_is_available, NULL);
}

static void queue_destroy(queue_t* q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_input_is_available);
}

/* Enqueue a line (can be NULL as end-of-stream sentinel) */
static int queue_enqueue(queue_t* q, char* line) {
	// Allocate new node and initialize it
    node_t* n = malloc(sizeof(node_t));
    if (!n) return -1;
    n->line = line;
    n->next = NULL;

	// Thread-safe enqueue operation (add to tail)
    pthread_mutex_lock(&q->mutex);
    if (q->tail) {
        q->tail->next = n;
        q->tail = n;
    }
    else {
        q->head = q->tail = n;
    }
	pthread_cond_signal(&q->cond_input_is_available); // Notify consumer thread
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

/* Dequeue a line; blocks until an item is available.
   Caller takes ownership of returned char* (may be NULL sentinel). */
static char* queue_dequeue(queue_t* q) {
	// Wait for an item to be available
    pthread_mutex_lock(&q->mutex);
    while (q->head == NULL) {
		// pthread_cond_wait must always be in a loop to avoid spurious wakeups
        pthread_cond_wait(&q->cond_input_is_available, &q->mutex);
    }
	// Remove from head, while keeping the queue consistent
    node_t* n = q->head;
    q->head = n->next;
    if (q->head == NULL) q->tail = NULL;
    pthread_mutex_unlock(&q->mutex);
	// We got the node, extract the line and free the node
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
        if (*s != '\n') ++chars; /* exclude newline from character count */
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
	// Mode could be a function pointer, but this is simpler
    int mode; /* 1 = words, 2 = chars, 3 = vowels */
} worker_arg_t;

static void* worker_thread(void* arg) {
    worker_arg_t* w = (worker_arg_t*)arg;
    unsigned long long total = 0ULL;

    while (1) {
        char* line = queue_dequeue(w->queue);
        if (line == NULL) break; /* sentinel => no more data */

        size_t c = 0;
        if (w->mode == 1) c = count_words(line);
        else if (w->mode == 2) c = count_chars(line);
        else if (w->mode == 3) c = count_vowels(line);

        total += c;
        free(line);
    }

    unsigned long long* ret = malloc(sizeof(unsigned long long));
    if (!ret) pthread_exit(NULL);
    *ret = total;
    return ret;
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

    queue_t queues[3];
    for (int i = 0; i < 3; ++i) queue_init(&queues[i]);

    pthread_t threads[3];
    worker_arg_t args[3];
    for (int i = 0; i < 3; ++i) {
        args[i].queue = &queues[i];
		// Instead of mode, could use pointers to functions, but this is simpler
		args[i].mode = i + 1; // 1=words, 2=chars, 3=vowels
        if (pthread_create(&threads[i], NULL, worker_thread, &args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            fclose(f);
            return EXIT_FAILURE;
        }
    }

    /* Read lines and distribute round-robin. Use getline (POSIX). */
    char* linebuf = NULL;
    size_t cap = 0;
    ssize_t nread;
    int idx = 0;
    // Manually added: count lines. See the C++ single line comment style?
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
		// linebuf will be freed by worker threads
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
        void* res;
        pthread_join(threads[i], &res);
        if (res) {
            totals[i] = *(unsigned long long*)res;
            free(res);
        }
        else {
            totals[i] = 0;
        }
        queue_destroy(&queues[i]);
    }

    printf("Total words   : %llu\n", totals[0]);
    printf("Total chars   : %llu\n", totals[1]);
    printf("Total vowels  : %llu\n", totals[2]);
	printf("Total lines   : %zu\n", line_count);

    return EXIT_SUCCESS;
}