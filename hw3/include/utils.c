#include "utils.h"

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
        sprintf(dst, "EMPTY");
    } else {
        // bucket exists
        uint64_t offset = key & BUCKET_MOD;
        lseek(fd, KEY_SIZE * offset, SEEK_SET);
        read(fd, dst, KEY_SIZE);
        if (!dst[0])
            sprintf(dst, "EMPTY");
        close(fd);
    }
}
