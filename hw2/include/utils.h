#ifndef __UTILS_H__
#define __UTILS_H__

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define LINE_BUF 500

enum { INIT, IDLE, WORK, EXIT };

/* Entity of processing lines */
typedef struct __process_buf {
    char data[LINE_BUF];
    bool finished;
} process_buf;

/* Attributes for processing data on threads */
typedef struct __process_attr {
    /* Thread attributes */
    pthread_t id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    /* Buffers */
    process_buf *buf;
    struct __line_out *out;

    int state;
    int write_len;
    sem_t sem;
    sem_t finished;
} process_attr;

/* Output buffer entity */
typedef struct __line_out {
    char data[LINE_BUF];
    int len;
    bool eof;
} line_out;

typedef struct __out_attr {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct __line_out *buf;
    int state;
    int num;
    sem_t sem;
} out_attr;

/* Subroutines for threads */
void *process_worker(void *arg);
void *output_worker(void *arg);

#endif
