#ifndef __UTILS_H__
#define __UTILS_H__

#include <pthread.h>
#include <stdbool.h>

#define LINE_BUF 500

/* Entity of processing lines */
typedef struct __process_buf {
    char data[500];
    bool finished;
} process_buf;

/* Attributes for processing data on threads */
typedef struct __process_attr {
    /* Thread attributes */
    pthread_t id;
    pthread_mutex_t mutex;
    pthread_cond_t con;

    /* Buffers */
    process_buf *buf;
    struct __line_out *out;

    bool finished;
} process_attr;

/* Output buffer entity */
typedef struct __line_out {
    char data[500];
    int len;
} line_out;

typedef struct __out_attr {
    pthread_mutex_t mutex;
    pthread_cond_t con;
    struct __line_out *buf;
    int num;
} out_attr;

/* Subroutines for threads */
void *process_worker(void *arg);
void *output_worker(void *arg);

#endif
