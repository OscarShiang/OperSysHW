#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUCKET_SIZE (1 << 28) /* 256 MiB */
#define BUCKET_MOD ((uint64_t) 0xfffffff)

#define KEY_SIZE 128 /* at most 128 characters */

typedef struct bucket {
    char keys[BUCKET_SIZE][KEY_SIZE];
} bucket_t;

void kv_put(uint64_t key, char *value);
void kv_get(uint64_t key, char *dst);
