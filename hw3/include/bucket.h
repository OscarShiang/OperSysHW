#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define ELEMENT_COUNT 65535
#define VALUE_SIZE 128

typedef struct __element {
    int64_t key;
    char value[VALUE_SIZE];
} element_t;

typedef struct __bucket {
    int size;
    ssize_t num;
    element_t data[ELEMENT_COUNT];
} bucket_t;

void bucket_init(bucket_t *a);
int bucket_write(bucket_t *bucket, const uint8_t *key, const char *value);
int bucket_read(bucket_t *bucket, const uint8_t *key, char *buf);
int bucket_dump(bucket_t *bucket);
