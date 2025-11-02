#include "general_buffer.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STARTING_SIZE 16
#define GROWTH_FACTOR 2

GeneralBuffer genbuf_init(void) {
    return (GeneralBuffer){
        .data = malloc(STARTING_SIZE),
        .data_allocated = STARTING_SIZE,
        .data_size = 0,
    };
}

static inline void *allocate(GeneralBuffer *buf, size_t size) {
    int need_to_grow = 0;
    while (buf->data_size + size >= buf->data_allocated) {
        buf->data_allocated *= GROWTH_FACTOR;
        need_to_grow = 1;
    }
    if (need_to_grow)
        buf->data = realloc(buf->data, buf->data_allocated);

    void *start_address = buf->data + buf->data_size;
    buf->data_size += size;
    return start_address;
}

void *genbuf_append(GeneralBuffer *buf, void *data, size_t data_size) {
    void *address = allocate(buf, data_size);
    memcpy(address, data, data_size);
    return address;
}

void *genbuf_allocate(GeneralBuffer *buf, size_t size) {
    void *address = allocate(buf, size);
    memset(address, 0, size);
    return address;
}

void genbuf_free(GeneralBuffer *buf) {
    if (buf->data) {
        free(buf->data);
    }
}
