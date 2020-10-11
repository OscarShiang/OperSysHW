#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "include/utils.h"

#define IPT "test.csv"
#define OUT "output.json"
#define WORKER_NUM 5

int out_fd;
pthread_mutex_t start;
static pthread_cond_t finish_sig;

/* find the index of idle thread */
static int thd_sched(process_attr thds[])
{
    int index = -1;
    for (int i = 0; i < WORKER_NUM; i++) {
        if (thds[i].finished) {
            index = i;
            break;
        }
    }
    return index;
}

int main(int argc, char *argv[])
{
    /* Open the files */
    FILE *in = fopen(IPT, "r");
    out_fd = open(OUT, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    int ret;
    bool begin = true;

    /* Buffers for threads to use */
    process_buf buf_lines[WORKER_NUM];
    line_out buf_out[WORKER_NUM];

    pthread_mutex_init(&start, NULL);
    pthread_mutex_lock(&start);

    /* Initialize the threads */
    pthread_t process_thd[WORKER_NUM];
    process_attr thd_attr[WORKER_NUM];
    for (int i = 0; i < WORKER_NUM; i++) {
        pthread_mutex_init(&thd_attr[i].mutex, NULL);
        thd_attr[i].finished = true;

        pthread_create(&process_thd[i], 0, process_worker, &thd_attr[i]);
    }

    pthread_t output_thd;
    out_attr output_attr = {
        .buf = buf_out, .num = 0, .mutex = PTHREAD_MUTEX_INITIALIZER};
    pthread_create(&output_thd, 0, output_worker, &output_attr);

    write(out_fd, "[\n", 2);

    while (1) {
        /* Read lines from file */
        for (int i = 0; i < 20, ret != EOF; i++) {
            int index;
            while ((index = thd_sched(thd_attr)) == -1) {
                // TODO: block this thread
            }

            /* Inform the thread to do the job */
            ret = fscanf(in, "%s", buf_lines[index].data);
            if (ret)
                pthread_cond_signal(&thd_attr[i].cond);
        }

        /* Inform the output worker to print out the data */
        pthread_cond_signal(&output_attr.cond);
    }

    write(out_fd, "]\n", 2);

    fclose(in);
    close(out_fd);
    return 0;
}
