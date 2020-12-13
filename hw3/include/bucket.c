#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bucket.h"

void bucket_init(bucket_t *a)
{
    a->size = 0;
    memset(a->data, -1, sizeof(element_t) * ELEMENT_COUNT);
}

static int32_t hash(const uint8_t *key, const int32_t max)
{
    int32_t val = 5381;
    for (int i = 0; i < 8; i++) {
        val %= (1 << 24);
        val = ((val << 5) + val) + key[i];
    }
    return ((val % max) < 0) ? -val % max : val % max;
}

int bucket_write(bucket_t *bucket, const uint8_t *key, const char *value)
{
    if (!key || !value)
        return -1;

    int i = hash(key, ELEMENT_COUNT);

    if (bucket->data[i].key == -1)
        bucket->size++;

    bucket->data[i].key = *(int64_t *) key;
    strncpy(bucket->data[i].value, value, VALUE_SIZE);

    // TODO: dump the bucket to disk

    return 0;
}

int bucket_read(bucket_t *bucket, const uint8_t *key, char *buf)
{
    if (!key)
        return -1;

    int64_t key_value = *(int64_t *) key;
    int i = hash(key, ELEMENT_COUNT);

    if (bucket->data[i].key == -1 || bucket->data[i].key != key_value) {
        strcpy(buf, "EMPTY");
    } else {
        strncpy(buf, bucket->data[i].value, 128);
    }
    return 0;
}
