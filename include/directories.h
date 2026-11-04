#pragma once

#include <stdint.h>
#include <corecrt.h>

#include "cursor.h"
#include "headers.h"

#pragma pack(push, 1)
typedef struct PathTableHeader {
    uint8_t dir_id_length;
    uint8_t extended_attribute_record_length;
    uint32_t location_extent;
    uint16_t dir_number_parent;
} PathTableHeader_t;
#pragma pack(pop)

typedef struct PathTableEntry {
    PathTableHeader_t header;
    char *dir_id;
} PathTableEntry_t;

typedef struct PathTable {
    PathTableEntry_t *entries;
    size_t count;
} PathTable_t;

errno_t getPathTable(
    PathTable_t *path_table,
    ByteCursor_t *cur,
    const VolDescriptor_t *primary
);

void freePathTable(PathTable_t *path_table);
