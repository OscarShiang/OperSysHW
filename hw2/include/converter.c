#include <stdio.h>
#include <string.h>

#include "converter.h"

extern int worker_num;
extern sem_t idle_thd;

/* find the index of idle thread */
int thd_sched(convert_args thds[])
{
    int index = -1, task, completed;
    bool work;
    for (int i = 0; i < worker_num; i++) {
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
        // printf("[worker] change to idle state\n");

        sem_post(&idle_thd);
        sem_wait(&args->task);

        // printf("[worker] start to work\n");

        if (args->exit) {
            // printf("[worker] exit\n");
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
            // printf("[worker] tok: %s\n", tok);
            len += sprintf(args->out + len, "\t\t\"col_%d\":%s", data_cnt, tok);

            int flag = (data_cnt++ == 20);
            strcat(args->out, &",\n"[flag]);
            len += 2 - flag;

            tok = strtok_r(NULL, "|", &last);
        }
        strcat(args->out, "\t}");

        // printf("[worker] completed: %s\n", args->out);

        // printf("[worker] task completed\n");
        sem_post(&args->completed);
    }
}
