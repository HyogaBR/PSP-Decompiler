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

typedef struct DirectoryFlags {
    bool is_hidden          : 1;
    bool is_directory       : 1;
    bool is_associated      : 1;
    bool has_format_info    : 1;
    bool has_permissions    : 1;
    bool reserved_5         : 1;
    bool reserved_6         : 1;
    bool is_continuation    : 1;
} DirectoryFlags_t;

typedef struct DirectoryDate {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t gmt_offset;
} DirectoryDate_t;

#define BOTH_ENDIAN(type, name) \
    type name;                  \
    type __##name##_msb

typedef struct DirectoryRecordHeader {
    uint8_t dir_record_length;
    uint8_t extended_attribute_record_length;
    BOTH_ENDIAN(uint32_t, location_extent);
    BOTH_ENDIAN(uint32_t, data_length);
    DirectoryDate_t datetime;
    DirectoryFlags_t flags;
    uint8_t file_unit_size;
    uint8_t interleave_gap_size;
    BOTH_ENDIAN(uint16_t, v_sequence_number);
    uint8_t file_id_length;
} DirectoryRecordHeader_t;

#undef BOTH_ENDIAN
#pragma pack(pop)

typedef struct PathTableEntry {
    PathTableHeader_t header;
    char *dir_id;
} PathTableEntry_t;

typedef struct PathTable {
    PathTableEntry_t *entries;
    size_t count;
} PathTable_t;

typedef struct DirectoryRecordEntry {
    DirectoryRecordHeader_t header;
    char *file_id;
    uint8_t *system_use;

    uint32_t parent_extent;
} DirectoryRecordEntry_t;

typedef struct DirectoryRecord {
    DirectoryRecordEntry_t *entries;
    size_t count;
} DirectoryRecord_t;

errno_t getPathTable(
    PathTable_t *path_table,
    ByteCursor_t *cur,
    const VolDescriptor_t *primary
);

void freePathTable(PathTable_t *path_table);

errno_t getDirectoryRecords(
    DirectoryRecord_t *directory_records,
    ByteCursor_t *cur,
    const VolDescriptor_t *primary
);

void freeDirectoryRecords(DirectoryRecord_t *directory_records);
