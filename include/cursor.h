#pragma once

#include <corecrt.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t byte;
typedef struct ByteCursor ByteCursor_t;

ByteCursor_t *cursor_init(byte *ptr, size_t size);
errno_t cursor_skip(ByteCursor_t *cursor, size_t size);
byte *cursor_track(ByteCursor_t *cursor);
size_t cursor_remaining(ByteCursor_t *cursor);
errno_t cursor_read(ByteCursor_t *cursor, size_t size, byte *dst);
errno_t cursor_merge(ByteCursor_t *dst, ByteCursor_t *src);
ByteCursor_t *cursor_fork(ByteCursor_t *src);
void cursor_close(ByteCursor_t *cursor);
