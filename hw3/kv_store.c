#include <assert.h>
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
#define BUCKET_MOD 0xfffffffUL

#define KEY_SIZE 129 /* 128 + null terminated */

typedef struct bucket {
    char keys[BUCKET_SIZE][KEY_SIZE];
} bucket_t;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: ./kv_store [INPUT_FILE]\n");
        exit(-1);
    }

    char ipt_name[256] = {0};
    strncpy(ipt_name, argv[1], sizeof(ipt_name));
    char tmp_name[256] = {0};

    // remove the extension name
    for (int i = 0; i < strlen(ipt_name); i++) {
        if (ipt_name[i] == '.') {
            ipt_name[i] = '\0';
            break;
        }
    }
    sprintf(tmp_name, ".%s.output", ipt_name);

    mkdir("store", 511);

    bool has_out = false;
    FILE *input = fopen(argv[1], "r");
    FILE *out = fopen(tmp_name, "w");

    char cmd[5] = {0};
    uint64_t key1, key2;
    char value[129];
    while (fscanf(input, "%s", cmd) != EOF) {
        if (!strcmp(cmd, "PUT")) {
            fscanf(input, "%lu %s", &key1, value);
            // printf("[%llu] -> %s\n", key1, value);

            /* do store */
            char bucket_name[512];
            sprintf(bucket_name, "store/%lu.bucket", key1 >> 28);
            int fd = open(bucket_name, O_CREAT | O_WRONLY);
            assert(fd != -1 || "Fail to open the file");

            ftruncate(fd, (uint64_t) 129 * BUCKET_SIZE);
            uint64_t offset = key1 & BUCKET_MOD;
            lseek(fd, 129 * offset, SEEK_SET);
            write(fd, value, 129);

            close(fd);
        } else if (!strcmp(cmd, "GET")) {
            fscanf(input, "%lu", &key1);
            // printf("get [%llu]\n", key1);

            char bucket_name[512];
            sprintf(bucket_name, "store/%lu.bucket", key1 >> 28);
            int fd = open(bucket_name, O_RDONLY);
            if (fd == -1) {
                // bucket not found
                fprintf(out, "EMPTY\n");
            } else {
                // bucket exists
                uint64_t offset = key1 & BUCKET_MOD;
                lseek(fd, 129 * offset, SEEK_SET);
                read(fd, value, 129);
                fprintf(out, "%s\n", value);
                close(fd);
            }
        } else if (!strcmp(cmd, "SCAN")) {
            fscanf(input, "%lu %lu", &key1, &key2);

            // basically use assert to check if the scan across different pages
            assert(key1 >> 28 == key2 >> 28 || "Cross the page");

            uint64_t count = key2 & BUCKET_MOD - key1 & BUCKET_MOD;

            char bucket_name[512];
            sprintf(bucket_name, "store/%lu.bucket", key1 >> 28);
            int fd = open(bucket_name, O_RDONLY);
            if (fd == -1) {
                for (uint64_t i = 0; i < count; i++)
                    fprintf(out, "EMPTY\n");
            } else {
                lseek(fd, (uint64_t) 129 * key1 & BUCKET_MOD, SEEK_SET);
                for (uint64_t i = 0; i < count; i++) {
                    read(fd, value, 129);
                    fprintf(out, "%s\n", value);
                    lseek(fd, 129, SEEK_CUR);
                }
                close(fd);
            }
        } else {
            assert(0 && "Unknown command");
        }
    }

    if (has_out) {
        char out_name[512];
        sprintf(out_name, "%s.output", ipt_name);
        rename(tmp_name, out_name);
    }

    // close the files
    fclose(input);
    fclose(out);

    return 0;
}
