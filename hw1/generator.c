#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BASE ((size_t) 1 << 28)
// #define BASE 10
#define WORKER_NUM 10

static volatile _Atomic size_t cnt;
static size_t num = BASE;
static pthread_mutex_t mutex;
FILE *fout;

static void randombytes(uint8_t *x, size_t how_much)
{
    ssize_t i;
    static int fd = -1;

    ssize_t xlen = (ssize_t) how_much;
    if (fd == -1) {
        for (;;) {
            fd = open("/dev/urandom", O_RDONLY);
            if (fd != -1)
                break;
            sleep(1);
        }
    }

    while (xlen > 0) {
        if (xlen < 1048576)
            i = xlen;
        else
            i = 1048576;

        i = read(fd, x, (size_t) i);
        if (i < 1) {
            sleep(1);
            continue;
        }

        x += i;
        xlen -= i;
    }
}

void *worker(void *arg)
{
    int32_t a;
    while (1) {
        /* output random number into the file */
        randombytes((uint8_t *) &a, sizeof(int32_t));

        pthread_mutex_lock(&mutex);
        fprintf(fout, "%d\n", a);
        pthread_mutex_unlock(&mutex);

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
        "   -n     size of the file\n"
        "   -o     output name of the file\n");
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
