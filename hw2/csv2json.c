#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "include/converter.h"

#define IPT "input.csv"
#define OUT "output.json"

sem_t idle_thd;

int worker_num = 1;

int main(int argc, char *argv[])
{
    /* Parse the worker number */
    if (argc > 1)
        worker_num = atoi(argv[1]);

    /* Open the files */
    FILE *in = fopen(IPT, "r");
#ifndef SYS_WRITE
    FILE *out = fopen(OUT, "w");
#else
    int out = creat(OUT, S_IRUSR | S_IWUSR);
#endif

    int ret = 0;
    bool begin = true;

    sem_init(&idle_thd, 0, 0);

    pthread_t *workers = malloc(sizeof(pthread_t) * worker_num);
    convert_args *worker_args = malloc(sizeof(convert_args) * worker_num);

    for (int i = 0; i < worker_num; i++) {
        sem_init(&worker_args[i].task, 0, 0);
        sem_init(&worker_args[i].completed, 0, 0);
        worker_args[i].work = false;
        worker_args[i].exit = false;
        pthread_create(&workers[i], 0, convert_worker, &worker_args[i]);
    }

    int *schedule = malloc(sizeof(int) * worker_num);

    /* Define the time measurement variables */
    clock_t overall_start, overall_end;
    clock_t input_start, input_end, input_sum = 0;
    clock_t output_start, output_end, output_sum = 0;
    clock_t process_start, process_end, process_sum = 0;

    overall_start = clock();

#ifndef SYS_WRITE
    fprintf(out, "[");
#else
    write(out, "[", 1);
#endif
    while (1) {
        input_start = clock();

        int i;
        for (i = 0; i < worker_num; i++) {
            sem_wait(&idle_thd);
            // printf("worker idle\n");
            /* Pass the task to idle worker */
            int index;
            do {
                index = thd_sched(worker_args);
            } while (index == -1);

            // printf("[sced] %d\n", index);

            if (!i)
                process_start = clock();

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

        /* Analyze the time cost */
        input_end = clock();
        input_sum += input_end - input_start;

        process_end = clock();
        process_sum += process_end - process_start;

        // printf("print out\n");

        /* Output */
        output_start = clock();
        for (int j = 0; j < i; j++) {
            // printf("[main] print out [%d]: \n%s\n", schedule[j],
            //         worker_args[schedule[j]].out);
#ifndef SYS_WRITE
            fprintf(out, "%s", &",\n"[begin]);
            fprintf(out, "%s", worker_args[schedule[j]].out);
#else
            write(out, &",\n"[begin], 2 - begin);
            write(out, worker_args[schedule[j]].out,
                  worker_args[schedule[j]].size);
#endif
            begin = false;
        }
        output_end = clock();
        output_sum += output_end - output_start;

        if (ret == EOF)
            break;
    }
#ifndef SYS_WRITE
    fprintf(out, "\n]\n");
#else
    write(out, "\n]\n", 3);
#endif

    // printf("[main] detroy the threads\n");

    for (int i = 0; i < worker_num; i++) {
        worker_args[i].exit = true;
        sem_post(&worker_args[i].task);
        pthread_join(workers[i], NULL);
    }

    overall_end = clock();

    free(workers);
    free(worker_args);
    free(schedule);

    fclose(in);
#ifndef SYS_WRITE
    fclose(out);
#else
    close(out);
#endif

    /* Print out time consumption of each stage */
    printf("[Scan] time cost:\t%lf\n", ((double) input_sum) / CLOCKS_PER_SEC);
    printf("[Process] time cost:\t%lf\n",
           ((double) process_sum) / CLOCKS_PER_SEC);
    printf("[Output] time cost:\t%lf\n",
           ((double) output_sum) / CLOCKS_PER_SEC);
    printf("[Overall] time cost:\t%lf\n",
           ((double) overall_end - overall_start) / CLOCKS_PER_SEC);

    printf("\nConvertion completed\n");

    return 0;
}
