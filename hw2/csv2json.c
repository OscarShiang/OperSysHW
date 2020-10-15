#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
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
#define WORKER_NUM 1

/* Variables for main thread */
bool ipt_eof = false;
pthread_mutex_t start = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t finish_sig = PTHREAD_COND_INITIALIZER;

sem_t idle_thd;
sem_t idle_out;

/* Worker thread attributes */
process_buf buf_lines[WORKER_NUM];
line_out buf_out[WORKER_NUM];

pthread_t process_thd[WORKER_NUM];
process_attr thd_attr[WORKER_NUM];

/* find the index of idle thread */
static int thd_sched(process_attr thds[])
{
    int index = -1;
    for (int i = 0; i < WORKER_NUM; i++) {
        if (thds[i].state == IDLE) {
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

    int ret = 0;
    bool begin = true;

    sem_init(&idle_thd, 0, 0);
    sem_init(&idle_out, 0, 0);

    pthread_mutex_init(&start, NULL);
    pthread_mutex_lock(&start);

    /* Initialize the threads */
    for (int i = 0; i < WORKER_NUM; i++) {
#if 0
	pthread_mutex_init(&thd_attr[i].mutex, NULL);
        pthread_cond_init(&thd_attr[i].cond, NULL);
#endif

        sem_init(&thd_attr[i].sem, 0, 0);
        sem_init(&thd_attr[i].finished, 0, 0);
        thd_attr[i].state = INIT;
        thd_attr[i].buf = &buf_lines[i];

        pthread_create(&process_thd[i], 0, process_worker, &thd_attr[i]);
    }

    /* Creating output thread */
    pthread_t output_thd;
    out_attr output_attr = {.buf = buf_out,
                            .num = 0,
                            .mutex = PTHREAD_MUTEX_INITIALIZER,
                            .cond = PTHREAD_COND_INITIALIZER,
                            .state = IDLE};
    sem_init(&output_attr.sem, 0, 0);
    pthread_create(&output_thd, 0, output_worker, &output_attr);

    int data_cnt = 0;
    pthread_mutex_unlock(&start);

    sem_t *working_thd[WORKER_NUM];

    while (1) {
        printf("starting to process the file...\n");

        /* Read lines from file */
        int i;
        for (i = 0; i < WORKER_NUM; i++) {
            sem_wait(&idle_thd);

            int index;
            do {
                index = thd_sched(thd_attr);
            } while (index == -1);

            printf("[schd] %d\n", index);

            ret = fscanf(in, "%s", buf_lines[index].data);
            thd_attr[index].out = &buf_out[i];

            if (ret == EOF) {
                ipt_eof = true;
                break;
            } else {
                /* Inform the thread to do the job */
                sem_post(&thd_attr[index].sem);
                working_thd[i] = &thd_attr[index].finished;
                // pthread_cond_signal(&thd_attr[index].cond);
                printf("[main] loop\n");
                thd_attr[index].state = WORK;
            }

            printf("[%03d] passing the data to worker thread...\n", ++data_cnt);
            printf("[main] [%p] data %s\n\n", buf_lines[index].data,
                   buf_lines[index].data);
        }


        printf("break [%d]\n", i);

        for (int j = 0; j < i; j++)
            sem_wait(working_thd[j]);
        printf("pass\n");

        sem_wait(&idle_out);

        for (int j = 0; j < i; j++)
            buf_out[j].len = thd_attr[j].write_len;

        output_attr.num = i;

        /* Inform the output worker to print out the data */
        // pthread_cond_signal(&output_attr.cond);
        sem_post(&output_attr.sem);

        if (ipt_eof)
            break;
    }

    printf("[main] scanning completed\n");


    for (int i = 0; i < WORKER_NUM; i++)
        sem_post(&thd_attr[i].sem);
    sem_post(&output_attr.sem);

    for (int i = 0; i < WORKER_NUM; i++)
        pthread_join(process_thd[i], NULL);
    pthread_join(output_thd, NULL);



    printf("[main] converting completed\n");

    fclose(in);
    return 0;
}
