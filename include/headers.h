#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SECTOR_LEN 2048

typedef struct ByteCursor{
    const uint8_t *ptr;
    size_t remaining;
} ByteCursor_t;

typedef enum DescriptorType {
    DESCRIPTOR_BOOT_RECORD = 0,
    DESCRIPTOR_PRIMARY = 1,
    DESCRIPTOR_SET_TERMINATOR = 255
} DescriptorType_t;

typedef struct DescriptorHeader {
    uint8_t type;
    uint8_t id[5];
    uint8_t version;
} DescriptorHeader_t;

typedef struct BootRecordDescriptor {
    uint8_t boot_system_id[32];
    uint8_t boot_id[32];
    uint8_t boot_system_use[1977];
} BootRecordDescriptor_t;

typedef struct PrimaryDescriptor {
    uint8_t _unused1;
    uint8_t system_id[32];
    uint8_t v_id[32];
    uint8_t _unused2[8];
    uint8_t v_space_size[8];
    uint8_t _unused3[32];
    uint8_t v_set_size[4];
    uint8_t v_sequence_number[4];
    uint8_t logical_block_size[4];
    uint8_t path_table_size[8];
    uint8_t location_typeL_path_table[4];
    uint8_t location_opt_typeL_path_table[4];
    uint8_t location_typeM_path_table[4];
    uint8_t location_opt_typeM_path_table[4];
    uint8_t direntry_root[34];
    uint8_t v_set_id[128];
    uint8_t publisher_id[128];
    uint8_t data_preparer_id[128];
    uint8_t application_id[128];
    uint8_t copyright_file_id[37];
    uint8_t abstract_file_id[37];
    uint8_t biographic_file_id[37];
    uint8_t v_creation_date_time[17];
    uint8_t v_modification_date_time[17];
    uint8_t v_expiration_date_time[17];
    uint8_t v_effective_date_time[17];
    uint8_t file_structure_version;
    uint8_t _unused4;
    uint8_t application_used[512];
    uint8_t reserved[653];
} PrimaryDescriptor_t;

typedef struct SetTerminatorDescriptor {
    uint8_t _unused[2041];
} SetTerminatorDescriptor_t;

typedef struct VolDescriptor {
    DescriptorHeader_t header;
    union {
        BootRecordDescriptor_t boot_record;
        PrimaryDescriptor_t primary;
        SetTerminatorDescriptor_t set_terminator;
    } data;
} VolDescriptor_t;

errno_t getVolumeDescriptor(VolDescriptor_t *vd, ByteCursor_t *cur);
errno_t getBootRecordVD(BootRecordDescriptor_t *boot_record, ByteCursor_t *cur);
errno_t getPrimaryVD(PrimaryDescriptor_t *primary, ByteCursor_t *cur);
errno_t getSetTerminatorVD(SetTerminatorDescriptor_t *set_terminator, ByteCursor_t *cur);
