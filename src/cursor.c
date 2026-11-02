#include <corecrt.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "cursor.h"

typedef struct CursorBuffer {
    const byte *data;
    size_t size;
    size_t references;
} CursorBuffer_t;

struct ByteCursor {
    CursorBuffer_t *buffer;

    size_t start;
    size_t position;
    size_t limit;

    // Transaction information
    size_t transaction_start;
    size_t transaction_limit;

    bool transactional;
};

static errno_t cursor_create_view(
    ByteCursor_t **out,
    CursorBuffer_t *buffer,
    size_t start,
    size_t position,
    size_t limit,
    bool transactional,
    size_t transaction_start,
    size_t transaction_limit
) {
    if (out == NULL || buffer == NULL)
        return EINVAL;

    *out = NULL;

    if (start > position ||
        position > limit ||
        limit > buffer->size)
        return EINVAL;

    ByteCursor_t *cursor = malloc(sizeof(*cursor));

    if (cursor == NULL)
        return ENOMEM;

    cursor->buffer = buffer;

    cursor->start = start;
    cursor->position = position;
    cursor->limit = limit;

    cursor->transaction_start = transaction_start;
    cursor->transaction_limit = transaction_limit;

    cursor->transactional = transactional;

    buffer->references++;

    *out = cursor;

    return 0;
}


errno_t cursor_init(
    ByteCursor_t **out,
    const byte *data,
    size_t size
) {
    if (out == NULL)
        return EINVAL;

    *out = NULL;

    if (data == NULL || size == 0)
        return EINVAL;

    CursorBuffer_t *buffer = malloc(sizeof(*buffer));

    if (buffer == NULL)
        return ENOMEM;

    buffer->data = data;
    buffer->size = size;
    buffer->references = 0;

    errno_t err = cursor_create_view(
        out,
        buffer,
        0,
        0,
        size,
        false,
        0,
        0
    );

    if (err != 0)
        free(buffer);

    return err;
}

void cursor_close(
    ByteCursor_t *cursor
) {
    if (cursor == NULL)
        return;

    CursorBuffer_t *buffer = cursor->buffer;

    free(cursor);

    if (--buffer->references == 0)
        free(buffer);
}


errno_t cursor_fork(
    ByteCursor_t **out,
    const ByteCursor_t *src
) {
    if (out == NULL)
        return EINVAL;

    *out = NULL;

    if (src == NULL)
        return EINVAL;

    return cursor_create_view(
        out,
        src->buffer,
        src->start,
        src->position,
        src->limit,
        true,
        src->position,
        src->limit
    );
}

errno_t cursor_slice(
    ByteCursor_t **out,
    const ByteCursor_t *src,
    size_t size
) {
    if (out == NULL)
        return EINVAL;

    *out = NULL;

    if (src == NULL)
        return EINVAL;

    if (size > src->limit - src->position)
        return ENODATA;

    return cursor_create_view(
        out,
        src->buffer,
        src->position,
        src->position,
        src->position + size,
        false,
        0,
        0
    );
}

errno_t cursor_take(
    ByteCursor_t **out,
    ByteCursor_t *src,
    size_t size
) {
    if (out == NULL)
        return EINVAL;

    *out = NULL;

    if (src == NULL)
        return EINVAL;

    if (size > src->limit - src->position)
        return ENODATA;

    errno_t err = cursor_slice(out, src, size);

    if (err != 0)
        return err;

    src->position += size;

    return 0;
}


errno_t cursor_read(
    ByteCursor_t *cursor,
    void *dst,
    size_t dst_capacity,
    size_t size
) {
    if (cursor == NULL)
        return EINVAL;

    if (size == 0)
        return 0;

    if (dst == NULL)
        return EINVAL;

    if (size > dst_capacity)
        return ERANGE;

    if (size > cursor->limit - cursor->position)
        return ENODATA;

    memmove(
        dst,
        cursor->buffer->data + cursor->position,
        size
    );

    cursor->position += size;

    return 0;
}

errno_t cursor_skip(
    ByteCursor_t *cursor,
    size_t size
) {
    if (cursor == NULL)
        return EINVAL;

    if (size > cursor->limit - cursor->position)
        return ENODATA;

    cursor->position += size;

    return 0;
}

errno_t cursor_commit(
    ByteCursor_t *dst,
    const ByteCursor_t *src
) {
    if (dst == NULL || src == NULL)
        return EINVAL;

    // Must belong to the same buffer.
    if (dst->buffer != src->buffer)
        return EINVAL;

    // Source must be a transaction fork.
    if (!src->transactional)
        return EINVAL;

    // Destination must not have changed since fork.
    if (dst->position != src->transaction_start)
        return EINVAL;

    // Both cursors must represent the same region.
    if (dst->start != src->start ||
        dst->limit != src->transaction_limit ||
        src->limit != src->transaction_limit)
        return EINVAL;

    // Validate the resulting position.
    if (src->position < src->transaction_start ||
        src->position > src->transaction_limit)
        return EINVAL;

    dst->position = src->position;

    return 0;
}

const byte *cursor_track(
    const ByteCursor_t *cursor
) {
    if (cursor == NULL)
        return NULL;

    return cursor->buffer->data + cursor->position;
}

size_t cursor_remaining(
    const ByteCursor_t *cursor
) {
    if (cursor == NULL)
        return 0;

    return cursor->limit - cursor->position;
}

size_t cursor_offset(
    const ByteCursor_t *cursor
) {
    if (cursor == NULL)
        return 0;

    return cursor->position - cursor->start;
}

size_t cursor_size(
    const ByteCursor_t *cursor
) {
    if (cursor == NULL)
        return 0;

    return cursor->limit - cursor->start;
}
