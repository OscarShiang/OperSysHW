#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "include/random.h"

#define FILENAME "input.csv"

#define MAX_SPAWN ((size_t) 1 << 30)
#define WORKER_NUM 4
#define WORKING_DIR "/tmp/spawner_data%d"

void *spawn_worker(void *arg)
{
    int worker_num = *(int *) arg;

    int num[25];
    uint64_t content = 0;

    char filename[25];
    snprintf(filename, 25, WORKING_DIR, worker_num);

    FILE *fp = fopen(filename, "w");

    while (1) {
        randombytes((uint8_t *) &num, sizeof(num));
        int i;
        for (i = 0; i < 19; i++)
            content += fprintf(fp, "%d|", num[i]);
        content += fprintf(fp, "%d\n", num[i]);

        if (content >= MAX_SPAWN / WORKER_NUM) {
            fclose(fp);
            return NULL;
        }
    }
}

int main(void)
{
    mkdir("./tmp", S_IRUSR | S_IWUSR);

    pthread_t thr[WORKER_NUM - 1];
    int num[WORKER_NUM];
    for (int i = 0; i < WORKER_NUM - 1; i++) {
        num[i] = i;
        pthread_create(&thr[i], 0, &spawn_worker, &num[i]);
    }

    num[WORKER_NUM - 1] = WORKER_NUM - 1;
    spawn_worker((void *) &num[WORKER_NUM - 1]);

    for (int i = 0; i < WORKER_NUM - 1; i++)
        pthread_join(thr[i], NULL);

    int fd = creat(FILENAME, S_IRUSR | S_IWUSR);

    char filename[25], buf[4096];
    for (int i = 0; i < WORKER_NUM; i++) {
        snprintf(filename, 25, WORKING_DIR, i);
        int tmp = open(filename, O_RDONLY), ret;

        while ((ret = read(tmp, buf, 4096)))
            write(fd, buf, ret);
        close(tmp);
        remove(filename);
    }

    rmdir("./tmp");
    close(fd);

    printf("Spawning completed\n");

    return 0;
}
