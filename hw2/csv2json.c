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

#define IPT "tmp.csv"
#define OUT "output.json"
#define WORKER_NUM 10

sem_t idle_thd;
sem_t task_completed;

typedef struct {
    sem_t task;
    sem_t completed;
    bool work;
    bool exit;
    char input[500];
    char out[500];
} convert_args;

/* find the index of idle thread */
static int thd_sched(convert_args thds[])
{
    int index = -1, task, completed;
    bool work;
    for (int i = 0; i < WORKER_NUM; i++) {
        sem_getvalue(&thds[i].task, &task);
        sem_getvalue(&thds[i].completed, &completed);
        work = *(&thds[i].work);
        if (!task && !completed && !work) {
            index = i;
            break;
        }
    }
    return index;
}

void *convert_worker(void *arg)
{
    convert_args *args = (convert_args *) arg;

    while (1) {
        /* Wait for the task */
        printf("[worker] change to idle state\n");
        args->work = false;

        sem_post(&idle_thd);
        sem_wait(&args->task);

        printf("[worker] start to work\n");

        if (args->exit) {
            printf("[worker] exit\n");
            return NULL;
        }

        int len = 0, data_cnt = 1;
        args->out[0] = 0;
        strcat(args->out, "\t{\n");

        len += 3;

        char *tok, *last = NULL;

        // printf("[worker] %s\n", args->out);

        tok = strtok_r(args->input, "|", &last);
        while (tok) {
            printf("[worker] tok: %s\n", tok);
            len += sprintf(args->out + len, "\t\t\"col_%d\":%s", data_cnt, tok);

            int flag = (data_cnt++ == 20);
            strcat(args->out, &",\n"[flag]);
            len += 2 - flag;

            tok = strtok_r(NULL, "|", &last);
        }
        strcat(args->out, "\t}");

        printf("[worker] completed: %s\n", args->out);

        printf("[worker] task completed\n");
        sem_post(&args->completed);
    }
}

int main(int argc, char *argv[])
{
    /* Open the files */
    FILE *in = fopen(IPT, "r");
    FILE *out = fopen(OUT, "w");

    int ret = 0;
    bool begin = true;

    sem_init(&idle_thd, 0, 0);
    sem_init(&task_completed, 0, 0);

    pthread_t workers[WORKER_NUM];
    convert_args worker_args[WORKER_NUM];

    for (int i = 0; i < WORKER_NUM; i++) {
        sem_init(&worker_args[i].task, 0, 0);
        sem_init(&worker_args[i].completed, 0, 0);
        worker_args[i].work = false;
        worker_args[i].exit = false;
        pthread_create(&workers[i], 0, convert_worker, &worker_args[i]);
    }

    char *out_buf[WORKER_NUM];
    sem_t *completed[WORKER_NUM];

    fprintf(out, "[");
    while (1) {
        int i;
        for (i = 0; i < WORKER_NUM; i++) {
            sem_wait(&idle_thd);
            printf("worker idle\n");
            /* Pass the task to idle worker */
            int index;
            do {
                index = thd_sched(worker_args);
            } while (index == -1);

            printf("[sced] %d\n", index);

            ret = fscanf(in, "%s", worker_args[index].input);
            printf("[main] pass: %s\n", worker_args[index].input);

            out_buf[i] = worker_args[index].out;
            completed[i] = &worker_args[index].completed;

            if (ret == EOF) {
                break;
            } else {
                worker_args[i].work = true;
                sem_post(&worker_args[index].task);
            }
        }

        printf("[main] i = %d\n", i);

        // FIXME: this cannot prevent the threads asychronous problems
        /* wait for the worker */
        for (int j = 0; j < i; j++)
            sem_wait(completed[j]);

        printf("print out\n");

        /* Output */
        for (int j = 0; j < i; j++) {
            printf("[main] print out [%d]: \n%s\n", j, out_buf[j]);
            fprintf(out, "%s", &",\n"[begin]);
            fprintf(out, "%s", out_buf[j]);
            begin = false;
        }

        if (ret == EOF)
            break;
    }
    fprintf(out, "\n]");

    printf("[main] detroy the threads\n");

    for (int i = 0; i < WORKER_NUM; i++) {
        worker_args[i].exit = true;
        sem_post(&worker_args[i].task);
        pthread_join(workers[i], NULL);
    }

    printf("[MAIN] Completed\n");

    fclose(in);
    fclose(out);

    return 0;
}
