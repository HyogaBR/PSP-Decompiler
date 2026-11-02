#pragma once

#include <corecrt.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t byte;

typedef struct ByteCursor ByteCursor_t;

// Lifecycle
errno_t cursor_init(
    ByteCursor_t **out,
    const byte *data,
    size_t size
);

void cursor_close(ByteCursor_t *cursor);

// Cursor creation
errno_t cursor_fork(
    ByteCursor_t **out,
    const ByteCursor_t *src
);

errno_t cursor_slice(
    ByteCursor_t **out,
    const ByteCursor_t *src,
    size_t size
);

errno_t cursor_take(
    ByteCursor_t **out,
    ByteCursor_t *src,
    size_t size
);

// Reading
errno_t cursor_read(
    ByteCursor_t *cursor,
    void *dst,
    size_t dst_capacity,
    size_t size
);

errno_t cursor_skip(
    ByteCursor_t *cursor,
    size_t size
);

// Transaction
errno_t cursor_commit(
    ByteCursor_t *dst,
    const ByteCursor_t *src
);

// Information
const byte *cursor_track(
    const ByteCursor_t *cursor
);

size_t cursor_remaining(
    const ByteCursor_t *cursor
);

size_t cursor_offset(
    const ByteCursor_t *cursor
);

size_t cursor_size(
    const ByteCursor_t *cursor
);
