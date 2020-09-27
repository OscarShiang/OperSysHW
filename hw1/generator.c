#include <locale.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// #define BASE ((size_t) 1 << 28)
#define BASE 10
#define WORKER_NUM 3

static volatile _Atomic size_t cnt;
static size_t num = BASE;
static pthread_mutex_t mutex;
FILE *fout;

void *worker(void *arg)
{
    while (1) {
        /* output random number into the file */
        // pthread_mutex_lock(&mutex);
        fprintf(fout, "%d\n", rand());
        // pthread_mutex_unlock(&mutex);

        size_t m = atomic_fetch_add(&cnt, 1);
        printf("generating test data... (%lu / %lu)\r", m, num);
        if (m >= num)
            return NULL;
    }
}

static void print_usage()
{
    printf(
        "Usage: generator [options]\n"
        "Options:\n"
        "   -n        size of the file\n"
        "   -o        output name of the file\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        print_usage();

    /* set up locale */
    setlocale(LC_ALL, "en_US.UTF-8");

    /* parsing the arguments */
    char opt, filename[100] = "input.txt";
    while ((opt = getopt(argc, argv, "n:o:")) != -1) {
        switch (opt) {
        case 'n':
            num *= atoi(optarg);
            break;
        case 'o':
            strncpy(filename, optarg, 100);
            break;
        default:
            print_usage();
        }
    }

    /* open the file */
    fout = fopen(filename, "w");

    /* initialize mutex */
    pthread_mutex_init(&mutex, NULL);
    srand(time(NULL));
    atomic_store(&cnt, 0);

    pthread_t thr;

    for (int i = 0; i < WORKER_NUM - 1; i++)
        pthread_create(&thr, 0, &worker, 0);

    worker(0);

    fclose(fout);
    return 0;
}
