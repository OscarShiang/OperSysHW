#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUCKET_SIZE (1 << 28) /* 256 MiB */
#define BUCKET_MOD ((uint64_t) 0xfffffff)

#define KEY_SIZE 128 /* 128 + null terminated */

typedef struct bucket {
    char keys[BUCKET_SIZE][KEY_SIZE];
} bucket_t;

/* put value into storage */
void kv_put(uint64_t key, char *value)
{
    char bucket_name[512];

    sprintf(bucket_name, "storage/%lu.bucket", key >> 28);
    int fd = open(bucket_name, O_CREAT | O_WRONLY, S_IRWXU);
    assert(fd != -1);

    ftruncate(fd, (uint64_t) KEY_SIZE * BUCKET_SIZE);

    uint64_t offset = key & BUCKET_MOD;
    lseek(fd, KEY_SIZE * offset, SEEK_SET);
    write(fd, value, KEY_SIZE);

    close(fd);
}

/* get value from the storage */
void kv_get(uint64_t key, char *dst)
{
    char bucket_name[512];

    sprintf(bucket_name, "storage/%lu.bucket", key >> 28);
    int fd = open(bucket_name, O_RDONLY);
    if (fd == -1) {
        // bucket not found
        sprintf(dst, "EMPTY\n");
    } else {
        // bucket exists
        uint64_t offset = key & BUCKET_MOD;
        lseek(fd, KEY_SIZE * offset, SEEK_SET);
        read(fd, dst, KEY_SIZE);
        if (!dst[0])
            sprintf(dst, "EMPTY\n");
        close(fd);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: ./kv_store [INPUT_FILE]\n");
        exit(-1);
    }

    char ipt_name[256] = {0};
    char tmp_name[256] = {0};

    // find the filename
    int len = strlen(argv[1]);
    int iter = len;
    while (--iter && argv[1][iter] != '/')
	;/* iterates */
    bool found = (argv[1][iter] == '/');

    strncpy(ipt_name, &argv[1][iter + found], len - iter - found - 6);
    sprintf(tmp_name, "%s.output", ipt_name);

    mkdir("storage", 511);

    bool has_out = false;
    FILE *input = fopen(argv[1], "r");
    FILE *out = fopen(tmp_name, "w");

    char cmd[5] = {0};
    uint64_t key1, key2;
    char value[KEY_SIZE];

    while (fscanf(input, "%s", cmd) != EOF) {
        if (!strcmp(cmd, "PUT")) {
            fscanf(input, "%lu %s", &key1, value);
            kv_put(key1, value);
        } else if (!strcmp(cmd, "GET")) {
            fscanf(input, "%lu", &key1);

            has_out = true;
            kv_get(key1, value);
            fprintf(out, "%s", value);
        } else if (!strcmp(cmd, "SCAN")) {
            fscanf(input, "%lu %lu", &key1, &key2);

            has_out = true;
            for (uint64_t i = key1; i <= key2; i++) {
                kv_get(i, value);
                fprintf(out, "%s", value);
            }
        } else
            assert(0 && "Unknown command");
    }

    // close the files
    fclose(input);
    fclose(out);

    if (has_out) {
        char out_name[512];
        sprintf(out_name, "%s.output", ipt_name);
        rename(tmp_name, out_name);
    } else
        remove(tmp_name);

    return 0;
}
