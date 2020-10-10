#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

#define DELIM "|"
#define POSTFIX ",\n"

extern pthread_mutex_t start;
extern int out_fd;

void *process_worker(void *arg)
{
    process_attr *attr = arg;

    /* Wait for the initial input */
    pthread_mutex_lock(&start);
    pthread_mutex_unlock(&start);

    while (1) {
        pthread_mutex_lock(&attr->mutex);
        pthread_cond_wait(&attr->con, &attr->mutex);
        pthread_mutex_unlock(&attr->mutex);
        if (attr->finished)
            break;

        char *ipt = attr->buf->data;
        char *out = attr->out->data;

        int i = 1;
        char buf[50];
        char *last = NULL;
        char *tok = strtok_r(ipt, DELIM, &last);

        strcat(out, "\t{\n");
        while (tok) {
            sprintf(buf, "\t\t\"col_%d\":%s", i, tok);
            strcat(out, buf);
            int flag = i++ == 20;
            strcat(out, &POSTFIX[flag]);

            tok = strtok_r(NULL, DELIM, &last);
        }
        strcat(out, "\t}");
    }

    return NULL;
}

void *output_worker(void *arg)
{
    out_attr *attr = arg;

    pthread_mutex_lock(&start);
    pthread_mutex_unlock(&start);

    while (1) {
        pthread_cond_wait(&attr->con, &attr->mutex);

        line_out *lines = attr->buf;

        for (int i = 0; i < attr->num; i++)
            write(out_fd, lines[i].data, lines[i].len);
    }
}
