#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "include/utils.h"

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
	; /* iterates */
    bool found = (argv[1][iter] == '/');

    strncpy(ipt_name, &argv[1][iter + found], len - iter - found - 6);
    sprintf(tmp_name, ".%s.output", ipt_name);

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
            fprintf(out, "%s\n", value);
        } else if (!strcmp(cmd, "SCAN")) {
            fscanf(input, "%lu %lu", &key1, &key2);

            has_out = true;
            for (uint64_t i = key1; i <= key2; i++) {
                kv_get(i, value);
                fprintf(out, "%s\n", value);
            }
        } else
            assert(0 && "Unknown command");
    }

    // close the files
    fclose(input);
    fclose(out);

    if (has_out)
        rename(tmp_name, tmp_name + 1);
    else
        remove(tmp_name);

    return 0;
}
