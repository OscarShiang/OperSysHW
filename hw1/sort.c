#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CHUNK_SIZE (1 << 19)
#define MERGE_NUM 5
#define FILENAME_SIZE 100
#define CHUNK_PATH "./chunk/"

#define min(a, b) ((a < b) ? (a) : (b))

typedef struct {
    int value;
    int ret;
    FILE *fp;
} merge_item;

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

static bool is_finish(merge_item items[], size_t len)
{
    bool finish = true;
    for (int i = 0; i < len; i++) {
        finish &= (items[i].ret == EOF);
    }
    return finish;
}

static size_t find_index(merge_item items[], size_t len)
{
    ssize_t index = -1;
    int value = items[index].value;

    for (int i = 0; i < len; i++) {
        if (items[i].fp == NULL || items[i].ret == EOF)
            continue;
        if (value > items[i].value || index == -1) {
            value = items[i].value;
            index = i;
        }
    }
    return index;
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

    /* measure the time of spliting */
    clock_t time_start, time_end;

    time_start = clock();
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
    time_end = clock();

    printf("split time: %lf\n",
           ((double) time_end - time_start) / CLOCKS_PER_SEC);

    /* merge the chunks into a single file */
    size_t *chunks = malloc(sizeof(size_t) * chunk_cnt);

    for (size_t i = 0; i < chunk_cnt; i++)
        chunks[i] = i;

    size_t backup = chunk_cnt;
    int pending_idx = 0;

    size_t out_cnt = 0;

    time_start = clock();
    while (chunk_cnt > 1) {
        size_t i;
        for (i = 0; i < min(chunk_cnt, (ssize_t) chunk_cnt - 1);
             i += MERGE_NUM) {
            merge_item items[MERGE_NUM];
            int sort_num = 0;

            for (int j = 0; j < MERGE_NUM; j++) {
                snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                         chunks[i + j]);
                items[j].fp = fopen(chunk_file, "r");
                if (items[j].fp) {
                    sort_num++;
                    items[j].ret = fscanf(items[j].fp, "%d", &items[j].value);
                } else
                    items[j].ret = EOF;
            }

            FILE *tmp = fopen("tmp", "w");

            /* merge the files */
            while (!is_finish(items, sort_num)) {
                size_t idx = find_index(items, sort_num);
                fprintf(tmp, "%d\n", items[idx].value);
                if (items[idx].fp)
                    items[idx].ret =
                        fscanf(items[idx].fp, "%d", &items[idx].value);
                else
                    items[idx].ret = EOF;
            }

            for (int j = 0; j < sort_num; j++) {
                fclose(items[j].fp);

                /* remove the original files */
                snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                         chunks[i + j]);
                remove(chunk_file);
            }
            fclose(tmp);

            /* rename the merged file */
            snprintf(chunk_file, FILENAME_SIZE, CHUNK_PATH "data%lu",
                     chunks[i]);
            rename("tmp", chunk_file);

            chunks[pending_idx++] = chunks[i];
        }

        /* save the redundant file */
        while (i < chunk_cnt)
            chunks[pending_idx++] = chunks[i++];

        chunk_cnt = pending_idx;
        pending_idx = 0;
    }
    rename(CHUNK_PATH "data0", out);

    time_end = clock();

    printf("merge time: %lf\n",
           ((double) time_end - time_start) / CLOCKS_PER_SEC);

    free(chunks);


    return 0;
}
