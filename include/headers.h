#pragma once

#include <corecrt.h>
#include <stddef.h>
#include <stdint.h>

#include "cursor.h"

#define SECTOR_LEN 2048

#define BOTH_ENDIAN(type, name) \
    type name;           \
    type __##name##_msb

#define ISO_TO_C_STRING(param) (param + 1)



typedef enum DescriptorType {
    DESCRIPTOR_BOOT_RECORD = 0,
    DESCRIPTOR_PRIMARY = 1,
    DESCRIPTOR_SET_TERMINATOR = 255
} DescriptorType_t;

// ISO structs

#pragma pack(push, 1)

typedef struct DescriptorDateTimeISO {
    char    year[4];
    char    month[2];
    char    day[2];
    char    hour[2];
    char    minute[2];
    char    second[2];
    char    hundredsec[2];
    uint8_t timezone;
} DescriptorDateTimeISO_t;

typedef struct DescriptorHeaderISO {
    uint8_t type;
    char    id[5];
    uint8_t version;
} DescriptorHeaderISO_t;

typedef struct BootRecordDescriptorISO {
    char     system_id[32];
    char            id[32];
    uint8_t system_use[1977];
} BootRecordDescriptorISO_t;

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
    uint32_t __location_typeM_path_table;
    uint32_t __location_opt_typeM_path_table;

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
    uint8_t _unused[2041];
} SetTerminatorDescriptorISO_t;

typedef union DescriptorDataISO {
    BootRecordDescriptorISO_t boot_record;
    PrimaryDescriptorISO_t primary;
    SetTerminatorDescriptorISO_t set_terminator;
} DescriptorDataISO_t;

#pragma pack(pop)

// program structures

typedef struct DescriptorHeader {
    uint8_t type;
    char    id[ISO_TO_C_STRING(5)];
    uint8_t version;
} DescriptorHeader_t;

typedef struct DescriptorDateTime {
    char          year[ISO_TO_C_STRING(4)];
    char         month[ISO_TO_C_STRING(2)];
    char           day[ISO_TO_C_STRING(2)];
    char          hour[ISO_TO_C_STRING(2)];
    char        minute[ISO_TO_C_STRING(2)];
    char        second[ISO_TO_C_STRING(2)];
    char    hundredsec[ISO_TO_C_STRING(2)];
    uint8_t timezone;
} DescriptorDateTime_t;

typedef struct BootRecordDescriptor {
    char     system_id[ISO_TO_C_STRING(32)];
    char            id[ISO_TO_C_STRING(32)];
    uint8_t system_use[1977];
} BootRecordDescriptor_t;

typedef struct PrimaryDescriptor {

    char system_id[ISO_TO_C_STRING(32)];
    char      v_id[ISO_TO_C_STRING(32)];

    uint32_t v_space_size;
    uint16_t v_set_size;
    uint16_t v_sequence_number;
    uint16_t logical_block_size;
    uint32_t path_table_size;
    uint32_t location_path_table;
    uint32_t location_opt_path_table;

    uint8_t direntry_root[34]; // next step

    char           v_set_id[ISO_TO_C_STRING(128)];
    char       publisher_id[ISO_TO_C_STRING(128)];
    char   data_preparer_id[ISO_TO_C_STRING(128)];
    char     application_id[ISO_TO_C_STRING(128)];
    char  copyright_file_id[ISO_TO_C_STRING( 37)];
    char   abstract_file_id[ISO_TO_C_STRING( 37)];
    char biographic_file_id[ISO_TO_C_STRING( 37)];

    DescriptorDateTime_t v_creation_date_time;
    DescriptorDateTime_t v_modification_date_time;
    DescriptorDateTime_t v_expiration_date_time;
    DescriptorDateTime_t v_effective_date_time;

    uint8_t file_structure_version;

    uint8_t application_used[512];
    uint8_t reserved[653];
} PrimaryDescriptor_t;

typedef struct SetTerminatorDescriptor {
    uint8_t _unused[2041];
} SetTerminatorDescriptor_t;

typedef union DescriptorData {
    BootRecordDescriptor_t boot_record;
    PrimaryDescriptor_t primary;
    SetTerminatorDescriptor_t set_terminator;
} DescriptorData_t;

typedef struct VolDescriptor {
    DescriptorHeader_t header;
    DescriptorData_t data;
} VolDescriptor_t;

errno_t getVolumeDescriptor(VolDescriptor_t *vd, ByteCursor_t *cur);
errno_t getDescriptorHeader(DescriptorHeader_t *header, ByteCursor_t *cur);
errno_t getDescriptorData(DescriptorData_t *data, DescriptorType_t dataType, ByteCursor_t *cur);
errno_t getDescriptorDateTime(DescriptorDateTime_t *dt, ByteCursor_t *cur);
