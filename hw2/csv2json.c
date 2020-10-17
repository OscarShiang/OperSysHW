#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/converter.h"

#define IPT "tmp.csv"
#define OUT "output.json"

sem_t idle_thd;

int main(int argc, char *argv[])
{
    /* Open the files */
    FILE *in = fopen(IPT, "r");
    FILE *out = fopen(OUT, "w");

    int ret = 0;
    bool begin = true;

    sem_init(&idle_thd, 0, 0);

    pthread_t workers[WORKER_NUM];
    convert_args worker_args[WORKER_NUM];

    for (int i = 0; i < WORKER_NUM; i++) {
        sem_init(&worker_args[i].task, 0, 0);
        sem_init(&worker_args[i].completed, 0, 0);
        worker_args[i].work = false;
        worker_args[i].exit = false;
        pthread_create(&workers[i], 0, convert_worker, &worker_args[i]);
    }

    int schedule[WORKER_NUM];

    fprintf(out, "[");
    while (1) {
        int i;
        for (i = 0; i < WORKER_NUM; i++) {
            sem_wait(&idle_thd);
            // printf("worker idle\n");
            /* Pass the task to idle worker */
            int index;
            do {
                index = thd_sched(worker_args);
            } while (index == -1);

            // printf("[sced] %d\n", index);

            ret = fscanf(in, "%s", worker_args[index].input);
            // printf("[main] pass: %s\n", worker_args[index].input);

            schedule[i] = index;

            if (ret == EOF) {
                break;
            } else {
                worker_args[index].work = true;
                sem_post(&worker_args[index].task);
            }
        }

        // printf("[main] i = %d\n", i);

        /* wait for the worker */
        for (int j = 0; j < i; j++) {
            sem_wait(&worker_args[schedule[j]].completed);
            worker_args[schedule[j]].work = false;
        }

        // printf("print out\n");

        /* Output */
        for (int j = 0; j < i; j++) {
            // printf("[main] print out [%d]: \n%s\n", schedule[j],
            //         worker_args[schedule[j]].out);
            fprintf(out, "%s", &",\n"[begin]);
            fprintf(out, "%s", worker_args[schedule[j]].out);
            begin = false;
        }

        if (ret == EOF)
            break;
    }
    fprintf(out, "\n]");

    // printf("[main] detroy the threads\n");

    for (int i = 0; i < WORKER_NUM; i++) {
        worker_args[i].exit = true;
        sem_post(&worker_args[i].task);
        pthread_join(workers[i], NULL);
    }

    printf("Convertion completed\n");

    fclose(in);
    fclose(out);

    return 0;
}
