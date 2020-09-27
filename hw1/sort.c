#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHUNK_SIZE 1024
#define CHUNK_PATH "./chunk/"

static void print_usage()
{
    printf(
        "Usage: sort [options]\n"
        "Option:\n"
        "   -i     Specify the input filename\n"
        "   -o     Specify the output filename\n");
    exit(-1);
}

static inline int cmp(const void *a, const void *b)
{
    return *(int32_t *) a > *(int32_t *) b;
}

#if 0
void *chunk_worker(void *arg) {}
#endif

int main(int argc, char *argv[])
{
    if (argc < 4)
        print_usage();

    char in[100] = "input.txt", out[100] = "output.txt";

    /* parsing the arguments */
    char opt;
    while ((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch (opt) {
        case 'i':
            strncpy(in, optarg, 100);
            break;
        case 'o':
            strncpy(out, optarg, 100);
            break;
        default:
            print_usage();
        }
    }

    FILE *fin = fopen(in, "r");

#if 0
    /* counting the number of lines */
    char dump[20];
    size_t lines = 0;
    while (fgets(dump, 20, fin) != NULL)
        lines++;
#endif

    /* devide the data into chunks */
    int32_t idx = 0;
    size_t chunk_cnt = 0;
    char chunk_file[50];

    int32_t elements[CHUNK_SIZE];

    while (fscanf(fin, "%d", &elements[idx]) != EOF) {
        if (++idx >= CHUNK_SIZE) {
            /* apply sort on current array */
            qsort(elements, CHUNK_SIZE, sizeof(int), cmp);

            /* write the data in binary format */
            snprintf(chunk_file, 50, CHUNK_PATH "data%lu", chunk_cnt);
            int fd = open(chunk_file, O_WRONLY | O_CREAT, 777);
            if (fd == -1) {
                printf("[ERR] Fail to open file\n");
                exit(-1);
            }
            write(fd, elements, sizeof(elements));
            close(fd);

            chunk_cnt++;
            idx = 0;
        }
    }

    if (!idx) {
        snprintf(chunk_file, 50, CHUNK_PATH "data%lu", chunk_cnt);
        int fd = open(chunk_file, O_WRONLY | O_CREAT, 777);
        if (fd == -1) {
            printf("[ERR] Fail to open file\n");
            exit(-1);
        }

        write(fd, elements, sizeof(elements));
        close(fd);
        chunk_cnt++;
    }

    return 0;
}
