#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHUNK_SIZE 512
#define FILENAME_SIZE 100
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
    return *(int32_t *) a - *(int32_t *) b;
}

/* return the index of the minimum value of the array */
static int min_idx(int *arr, size_t len)
{
    int index = 0;
    int value = arr[0];

    for (int i = 0; i < len; i++) {
        if (arr[i] < value) {
            value = arr[i];
            index = i;
        }
    }
    return index;
}

static bool is_finish(bool *arr, size_t len)
{
    bool flag = true;
    for (int i = 0; i < len; i++)
        flag &= arr[i];
    return flag;
}

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

    /* devide the data into chunks */
    size_t idx = 0;
    size_t chunk_cnt = 0;
    char chunk_file[FILENAME_SIZE];

    int elements[CHUNK_SIZE];

    size_t data_cnt = 0;

    while (fscanf(fin, "%d", &elements[idx]) != EOF) {
        idx++;
        if (idx >= CHUNK_SIZE) {
            /* apply sort on current array */
            qsort(elements, CHUNK_SIZE, sizeof(int), cmp);

            /* write the data back */
            snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                     chunk_cnt++);
            FILE *tmp = fopen(chunk_file, "w");
            for (int i = 0; i < CHUNK_SIZE; i++) {
                fprintf(tmp, "%d\n", elements[i]);
                data_cnt++;
            }
            fclose(tmp);
            idx = 0;
        }
    }
    if (idx) {
        qsort(elements, idx, sizeof(int), cmp);

        snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu", chunk_cnt++);
        FILE *tmp = fopen(chunk_file, "w");
        for (int i = 0; i < idx; i++) {
            fprintf(tmp, "%d\n", elements[i]);
            data_cnt++;
        }
        fclose(tmp);
    }

    /* merge the chunks into a single file */
    size_t *chunks = malloc(sizeof(size_t) * chunk_cnt);

    /* initialize every file descriptor */
    for (size_t i = 0; i < chunk_cnt; i++)
        chunks[i] = i;

    size_t backup = chunk_cnt;
    int pending_idx = 0;

    size_t out_cnt = 0;

    while (chunk_cnt > 1) {
        size_t i;
        for (i = 0; i < chunk_cnt - 1; i += 2) {
            snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                     chunks[i]);
            FILE *fa = fopen(chunk_file, "r");

            snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                     chunks[i + 1]);
            FILE *fb = fopen(chunk_file, "r");

            int a, b;
            int ret_a = fscanf(fa, "%d", &a) != EOF;
            int ret_b = fscanf(fb, "%d", &b) != EOF;

            FILE *tmp = fopen("tmp", "w");

            /* merge the files */
            while (ret_a || ret_b) {
                if (!ret_a) {
                    fprintf(tmp, "%d\n", b);
                    ret_b = fscanf(fb, "%d", &b) != EOF;
                } else if (!ret_b) {
                    fprintf(tmp, "%d\n", a);
                    ret_a = fscanf(fa, "%d", &a) != EOF;
                } else if (a < b) {
                    fprintf(tmp, "%d\n", a);
                    ret_a = fscanf(fa, "%d", &a) != EOF;
                } else {
                    fprintf(tmp, "%d\n", b);
                    ret_b = fscanf(fb, "%d", &b) != EOF;
                }
            }

            fclose(fa);
            fclose(fb);
            fclose(tmp);

            /* remove the original files */
            remove(chunk_file);
            snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                     chunks[i]);
            remove(chunk_file);

            /* rename the merged file */
            rename("tmp", chunk_file);

            chunks[pending_idx++] = chunks[i];
        }

        /* save the redundant file */
        if (i < chunk_cnt)
            chunks[pending_idx++] = chunks[i];

        chunk_cnt = pending_idx;
        pending_idx = 0;
    }

    free(chunks);

    rename(CHUNK_PATH"data0", out);

        return 0;
}
