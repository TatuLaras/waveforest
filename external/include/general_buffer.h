#ifndef _FILE_BUFFER
#define _FILE_BUFFER

// A general auto-growing buffer that just holds bytes.

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t data_allocated;
    size_t data_size;
} GeneralBuffer;

GeneralBuffer genbuf_init(void);
void genbuf_free(GeneralBuffer *buf);

// Appends `data_size` bytes from `data` to the buffer, returns the start
// address of that newly appended data.
void *genbuf_append(GeneralBuffer *buf, void *data, size_t data_size);
// Allocates `size` zero-initialized bytes.
void *genbuf_allocate(GeneralBuffer *buf, size_t size);

#endif
