#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

/* Worker structure */
typedef struct {
    sem_t task;
    sem_t completed;
    bool work;
    bool exit;
    char input[500];
    char out[500];
} convert_args;

/* Utilities for convert worker */
int thd_sched(convert_args thds[]);
void *convert_worker(void *arg);

#endif
