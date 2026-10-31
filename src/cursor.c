#include <corecrt.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "cursor.h"

struct ByteCursor {
    byte *ptr;
    size_t remaining;
};


ByteCursor_t *cursor_init(
    byte *ptr,
    size_t size
) {
    if (ptr == NULL || size == 0)
        return NULL;

    ByteCursor_t *cursor = (ByteCursor_t *)malloc(sizeof(struct ByteCursor));
    if (cursor != NULL) {
        cursor->ptr = ptr;
        cursor->remaining = size;
    }

    return cursor;
}

errno_t cursor_skip(
    ByteCursor_t *cursor,
    size_t size
) {
    if (cursor == NULL || size > cursor->remaining) return EINVAL;


    cursor->ptr += size;
    cursor->remaining -= size;

    return 0;
}

byte *cursor_track(
    ByteCursor_t *cursor
) {
    if (cursor== NULL || cursor->ptr == NULL) return NULL;
    return cursor->ptr;
}

size_t cursor_remaining(
    ByteCursor_t *cursor
) {
    if (cursor == NULL) return 0;
    return cursor->remaining;
}

errno_t cursor_read(
    ByteCursor_t *cursor,
    size_t size,
    uint8_t *dst
) {
    if (cursor == NULL || dst == NULL) {
        return EINVAL;
    }

    if (cursor_remaining(cursor) < size) {
        return ENODATA;
    }

    errno_t err = memcpy_s(dst, size, cursor_track(cursor), size);
    if (err != 0) {
        return err;
    }

    return cursor_skip(cursor, size);
}

errno_t cursor_merge(
    ByteCursor_t *dst,
    ByteCursor_t *src
) {
    if (dst == NULL || src == NULL)
        return EINVAL;

    memcpy(dst, src, sizeof(struct ByteCursor));

    return 0;
}

ByteCursor_t *cursor_fork(
    ByteCursor_t *src
) {
    if (src == NULL) return NULL;

    ByteCursor_t *cursor = (ByteCursor_t *)malloc(sizeof(struct ByteCursor));
    if (cursor != NULL) {
        cursor->ptr = src->ptr;
        cursor->remaining = src->remaining;
    }

    return cursor;
}

void cursor_close(
    ByteCursor_t *cursor
) {
    free(cursor);
}
