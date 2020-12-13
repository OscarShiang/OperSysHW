#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bucket.h"

static int32_t hash(const uint8_t *key, const int32_t max)
{
    int32_t val = 5381;
    for (int i = 0; i < 8; i++) {
        val %= (1 << 24);
        val = ((val << 5) + val) + key[i];
    }
    return ((val % max) < 0) ? -val % max : val % max;
}

void bucket_init(bucket_t *a)
{
    // open the bucket setting if it exists
    int fd = open("storage/.bucket_meta", O_RDONLY);
    if (fd == -1)
        a->num = 0;
    else {
        read(fd, &a->num, sizeof(size_t));
        assert("The number of the bucket is greater than 0" && a->num > 0);
    }

    a->size = 0;
    memset(a->data, -1, sizeof(element_t) * ELEMENT_COUNT);
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

    // dump the bucket to disk
    if (bucket->size >= ELEMENT_COUNT) {
        bucket_dump(bucket);
        bucket_init(bucket);
    }

    return 0;
}

int bucket_read(bucket_t *bucket, const uint8_t *key, char *buf)
{
    if (!key)
        return -1;

    int64_t key_value = *(int64_t *) key;
    int i = hash(key, ELEMENT_COUNT);

    if (!bucket->num)
        strncpy(buf, bucket->data[i].value, VALUE_SIZE);
    else if (bucket->data[i].key == -1 || bucket->data[i].key != key_value) {
        int fd;
        bool found_data = false;
        char bucket_name[512];
        bucket_t *tmp_bucket = malloc(sizeof(bucket_t));
        for (ssize_t j = bucket->num - 1; j >= 0; j--) {
            sprintf(bucket_name, "storage/%lu.bucket", j);
            fd = open(bucket_name, O_RDONLY);
            assert("File is not existed" && fd != -1);
            read(fd, tmp_bucket, sizeof(bucket_t));
            close(fd);
            if (tmp_bucket->data[i].key == -1 ||
                tmp_bucket->data[i].key != key_value) {
                found_data = true;
                strncpy(buf, tmp_bucket->data[i].value, VALUE_SIZE);
                break;
            }
        }
        if (!found_data)
            strcpy(buf, "EMPTY");
        free(tmp_bucket);
    } else
        strncpy(buf, bucket->data[i].value, VALUE_SIZE);
    return 0;
}

int bucket_dump(bucket_t *bucket)
{
    // dump the bucket data
    char bucket_name[512];
    sprintf(bucket_name, "storage/%lu.bucket", bucket->num++);
    int fd = open(bucket_name, O_CREAT | O_WRONLY, S_IRWXU);

    write(fd, bucket, sizeof(bucket_t));
    close(fd);

    // revise the meta data
    fd = open("storage/.bucket_meta", O_WRONLY | O_CREAT, S_IRWXU);
    assert("Can not open the meta data" && fd != -1);
    write(fd, &bucket->num, sizeof(size_t));
    close(fd);

    return 0;
}
