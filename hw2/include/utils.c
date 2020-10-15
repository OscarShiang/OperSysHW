#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

#define DELIM "|"

#define OUT "output.json"

extern pthread_mutex_t start;
extern sem_t idle_thd;
extern sem_t idle_out;
extern bool ipt_eof;

static char postfix[] = ",\n";

void *process_worker(void *arg)
{
    process_attr *attr = arg;

    /* Wait for the initial input */
    printf("change state to idle\n");

    while (1) {
        attr->state = IDLE;
        printf("change to idle\n");

        sem_post(&idle_thd);
#if 0
        pthread_mutex_lock(&attr->mutex);
        pthread_cond_wait(&attr->cond, &attr->mutex);
	pthread_mutex_unlock(&attr->mutex);
#endif

        sem_wait(&attr->sem);

        printf("[worker] term state: %d\n", ipt_eof);
        if (ipt_eof)
            break;

        printf("processing...\n");

        /* Fetching new buffers */
        char *ipt = attr->buf->data;
        char *out = attr->out->data;

        out[0] = 0;
        int len = 0;

        printf("[%p] data recv: %s\n", ipt, ipt);

        int i = 1;
        char buf[50] = {0};
        char *last = NULL;
        char *tok = strtok_r(ipt, DELIM, &last);

        strcat(out, "\t{\n");
        while (tok) {
            len += sprintf(buf, "\t\t\"col_%d\":%s", i, tok);
            strcat(out, buf);
            printf("[worker] %s\n", tok);

            int flag = i++ == 20;
            len += (2 - flag);
            strcat(out, &postfix[flag]);

#if 0
            if (i > 20)
                break;
#endif

            tok = strtok_r(NULL, DELIM, &last);
        }
        strcat(out, "\t}");

        len += 5;

        /* Update the string length */
        // attr->out->len = len;

        attr->write_len = len;
        sem_post(&attr->finished);
        printf("Completed\n");
    }
    printf("[WORK] exiting\n");

    return NULL;
}

void *output_worker(void *arg)
{
    /* Store the attributes */
    out_attr *attr = arg;

    int fd = open(OUT, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    bool begin = true;

    write(fd, "[\n", 2);

    printf("[OUT] waiting\n");

    while (1) {
        attr->state = IDLE;

        sem_post(&idle_out);
#if 0
        pthread_mutex_lock(&attr->mutex);
        pthread_cond_wait(&attr->cond, &attr->mutex);
        pthread_mutex_unlock(&attr->mutex);
#endif
        printf("[OUT] waiting\n");
        sem_wait(&attr->sem);

        if (ipt_eof)
            break;

        printf("[OUT] writing data\n");
        line_out *lines = attr->buf;

        printf("[OUT] data size: %d\n", attr->num);

        for (int i = 0; i < attr->num; i++) {
            if (!begin)
                write(fd, postfix, 2);
            begin = false;
            printf("[OUT] string size: %d\n", lines[i].len);
            write(fd, lines[i].data, lines[i].len);
        }
        printf("[OUT] finish writing\n");
    }

    write(fd, "\n]\n", 2);

    printf("[OUT] completed\n");
    close(fd);
    return NULL;
}
