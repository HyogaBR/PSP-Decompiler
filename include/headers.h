#pragma once

#include <corecrt.h>
#include <stddef.h>
#include <stdint.h>

#include "cursor.h"

#define SECTOR_LEN 2048

#define ISO_TO_C_STRING(size) ((size) + 1)

#define ISO_DESCRIPTOR_HEADER_SIZE 7
#define ISO_DESCRIPTOR_DATA_SIZE  (SECTOR_LEN - ISO_DESCRIPTOR_HEADER_SIZE)

typedef enum DescriptorType {
    DESCRIPTOR_BOOT_RECORD       = 0,
    DESCRIPTOR_PRIMARY           = 1,
    DESCRIPTOR_SUPPLEMENTARY     = 2,
    DESCRIPTOR_VOLUME_PARTITION  = 3,
    DESCRIPTOR_SET_TERMINATOR    = 255
} DescriptorType_t;

/*
 * ISO 9660 binary structures.
 *
 * These structures represent the bytes exactly as they appear
 * in the ISO file. Do not add string terminators or modify fields.
 */

#pragma pack(push, 1)

typedef struct DescriptorDateTimeISO {
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char hundredsec[2];
    uint8_t timezone;
} DescriptorDateTimeISO_t;

typedef struct DescriptorHeaderISO {
    uint8_t type;
    char id[5];
    uint8_t version;
} DescriptorHeaderISO_t;

typedef struct BootRecordDescriptorISO {
    char system_id[32];
    char id[32];
    uint8_t system_use[1977];
} BootRecordDescriptorISO_t;

/*
 * ISO 9660 stores some numeric fields twice:
 *
 * - First representation: little-endian
 * - Second representation: big-endian
 *
 * BOTH_ENDIAN preserves both representations.
 */
#define BOTH_ENDIAN(type, name) \
    type name;                  \
    type __##name##_msb

typedef struct PrimaryDescriptorISO {
    uint8_t _unused1;

    char system_id[32];
    char v_id[32];

    uint64_t _unused2;

    BOTH_ENDIAN(uint32_t, v_space_size);

    uint8_t _unused3[32];

    BOTH_ENDIAN(uint16_t, v_set_size);
    BOTH_ENDIAN(uint16_t, v_sequence_number);
    BOTH_ENDIAN(uint16_t, logical_block_size);
    BOTH_ENDIAN(uint32_t, path_table_size);

    uint32_t location_typeL_path_table;
    uint32_t location_opt_typeL_path_table;

    uint32_t location_typeM_path_table;
    uint32_t location_opt_typeM_path_table;

    uint8_t direntry_root[34];

    char v_set_id[128];
    char publisher_id[128];
    char data_preparer_id[128];
    char application_id[128];

    char copyright_file_id[37];
    char abstract_file_id[37];
    char biographic_file_id[37];

    DescriptorDateTimeISO_t v_creation_date_time;
    DescriptorDateTimeISO_t v_modification_date_time;
    DescriptorDateTimeISO_t v_expiration_date_time;
    DescriptorDateTimeISO_t v_effective_date_time;

    uint8_t file_structure_version;
    uint8_t _unused4;

    uint8_t application_used[512];
    uint8_t reserved[653];
} PrimaryDescriptorISO_t;

typedef struct SetTerminatorDescriptorISO {
    uint8_t _unused[ISO_DESCRIPTOR_DATA_SIZE];
} SetTerminatorDescriptorISO_t;

/*
 * raw is useful for descriptor types that we do not interpret yet.
 * It also provides a common way to read exactly 2041 bytes.
 */
typedef union DescriptorDataISO {
    BootRecordDescriptorISO_t boot_record;
    PrimaryDescriptorISO_t primary;
    SetTerminatorDescriptorISO_t set_terminator;

    uint8_t raw[ISO_DESCRIPTOR_DATA_SIZE];
} DescriptorDataISO_t;

#pragma pack(pop)

#undef BOTH_ENDIAN

/*
 * Program structures.
 *
 * These structures are not binary representations of ISO 9660.
 * Strings have room for a terminating '\0'.
 * Numeric fields are converted to host-endian values.
 */

typedef struct DescriptorHeader {
    uint8_t type;
    char id[ISO_TO_C_STRING(5)];
    uint8_t version;
} DescriptorHeader_t;

typedef struct DescriptorDateTime {
    char year[ISO_TO_C_STRING(4)];
    char month[ISO_TO_C_STRING(2)];
    char day[ISO_TO_C_STRING(2)];
    char hour[ISO_TO_C_STRING(2)];
    char minute[ISO_TO_C_STRING(2)];
    char second[ISO_TO_C_STRING(2)];
    char hundredsec[ISO_TO_C_STRING(2)];

    uint8_t timezone;
} DescriptorDateTime_t;

typedef struct BootRecordDescriptor {
    char system_id[ISO_TO_C_STRING(32)];
    char id[ISO_TO_C_STRING(32)];

    uint8_t system_use[1977];
} BootRecordDescriptor_t;

typedef struct PrimaryDescriptor {
    char system_id[ISO_TO_C_STRING(32)];
    char v_id[ISO_TO_C_STRING(32)];

    uint32_t v_space_size;

    uint16_t v_set_size;
    uint16_t v_sequence_number;
    uint16_t logical_block_size;

    uint32_t path_table_size;

    uint32_t location_typeL_path_table;
    uint32_t location_opt_typeL_path_table;

    uint32_t location_typeM_path_table;
    uint32_t location_opt_typeM_path_table;

    uint8_t direntry_root[34];

    char v_set_id[ISO_TO_C_STRING(128)];
    char publisher_id[ISO_TO_C_STRING(128)];
    char data_preparer_id[ISO_TO_C_STRING(128)];
    char application_id[ISO_TO_C_STRING(128)];

    char copyright_file_id[ISO_TO_C_STRING(37)];
    char abstract_file_id[ISO_TO_C_STRING(37)];
    char biographic_file_id[ISO_TO_C_STRING(37)];

    DescriptorDateTime_t v_creation_date_time;
    DescriptorDateTime_t v_modification_date_time;
    DescriptorDateTime_t v_expiration_date_time;
    DescriptorDateTime_t v_effective_date_time;

    uint8_t file_structure_version;
    uint8_t unused;

    uint8_t application_used[512];
    uint8_t reserved[653];
} PrimaryDescriptor_t;

typedef struct SetTerminatorDescriptor {
    uint8_t _unused[ISO_DESCRIPTOR_DATA_SIZE];
} SetTerminatorDescriptor_t;

typedef union DescriptorData {
    BootRecordDescriptor_t boot_record;
    PrimaryDescriptor_t primary;
    SetTerminatorDescriptor_t set_terminator;

    /*
     * Raw payload for descriptor types that have not been
     * interpreted by the parser yet.
     */
    uint8_t raw[ISO_DESCRIPTOR_DATA_SIZE];
} DescriptorData_t;

typedef struct VolDescriptor {
    DescriptorHeader_t header;
    DescriptorData_t data;
} VolDescriptor_t;

/*
 * Reads one complete Volume Descriptor.
 *
 * On success:
 * - The cursor advances by exactly 2048 bytes.
 * - vd receives the decoded descriptor.
 *
 * On failure:
 * - vd remains unchanged.
 * - The original cursor remains unchanged.
 */
errno_t getVolumeDescriptor(
    VolDescriptor_t *vd,
    ByteCursor_t *cur
);
