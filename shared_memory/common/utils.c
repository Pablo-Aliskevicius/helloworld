#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* Print timestamp with microsecond precision */
void print_timestamp(void)
{
    struct timespec ts;
    struct tm *tm_info;
    char buffer[64];

    clock_gettime(CLOCK_REALTIME, &ts);
    tm_info = localtime(&ts.tv_sec);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s.%06ld] ", buffer, ts.tv_nsec / 1000);
}

/* Generate queue name for a specific client PID */
char* get_queue_name(pid_t pid)
{
    static char queue_name[64];
    snprintf(queue_name, sizeof(queue_name), "/chat_queue_%d", pid);
    return queue_name;
}
