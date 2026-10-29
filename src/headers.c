#include <corecrt.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "headers.h"

#define READ_FIELD(dst, cursor, err)                    \
    do {                                                \
        size_t fieldSize = sizeof(dst);                 \
                                                        \
        if ((cursor).remaining < fieldSize) {           \
            return EINVAL;                              \
        }                                               \
                                                        \
        (err) = memcpy_s(                               \
            (dst),                                      \
            sizeof(dst),                                \
            (cursor).ptr,                               \
            fieldSize                                   \
        );                                              \
                                                        \
        if ((err) != 0) {                               \
            return (err);                               \
        }                                               \
                                                        \
        (cursor).ptr += fieldSize;                      \
        (cursor).remaining -= fieldSize;                \
    } while (0)

#define READ_VALUE(dst, cursor, err)                    \
    do {                                                \
        size_t fieldSize = sizeof(dst);                 \
        if ((cursor).remaining < fieldSize) {           \
            return EINVAL;                              \
        }                                               \
        (err) = memcpy_s(                               \
            &(dst),                                     \
            sizeof(dst),                                \
            (cursor).ptr,                               \
            fieldSize                                   \
        );                                              \
        if ((err) != 0) {                               \
            return (err);                               \
        }                                               \
        (cursor).ptr += fieldSize;                      \
        (cursor).remaining -= fieldSize;                \
    } while (0)


errno_t getVolumeDescriptor(
    VolDescriptor_t *vd,
    ByteCursor_t *cur
) {

    if (cur == NULL || vd == NULL) {
        return EINVAL;
    }

    ByteCursor_t cursor = *cur;

    memset(vd, 0, sizeof(*vd));

    // Filling the header of struct VolumeDescriptor_t
    errno_t err;

    READ_VALUE(vd->header.type,     cursor, err);
    READ_FIELD(vd->header.id,       cursor, err);
    READ_VALUE(vd->header.version,  cursor, err);

    if (memcmp(vd->header.id, "CD001", 5) != 0) {
        return EINVAL;
    }

    if (vd->header.version != 1) {
        return EINVAL;
    }


    switch (vd->header.type) {
        case DESCRIPTOR_BOOT_RECORD:
            err = getBootRecordVD(
                &vd->data.boot_record,
                &cursor
            );

            if (err != 0) {
                return err;
            }
            break;

        case DESCRIPTOR_PRIMARY:
            err = getPrimaryVD(
                &vd->data.primary,
                &cursor
            );

            if (err != 0) {
                return err;
            }
            break;
        case DESCRIPTOR_SET_TERMINATOR:
            err = getSetTerminatorVD(
                &vd->data.set_terminator,
                &cursor
            );

            if (err != 0) {
                return err;
            }
            break;

        default:
            return EINVAL;
    }

    *cur = cursor;
    return 0;
}

errno_t getBootRecordVD(
    BootRecordDescriptor_t *boot_record,
    ByteCursor_t *cur
) {
    if (cur == NULL || boot_record == NULL) {
        return EINVAL;
    }

    ByteCursor_t cursor = *cur;

    // Filling the struct BootRecord_t
    errno_t err;

    READ_FIELD(boot_record->boot_system_id,     cursor, err);
    READ_FIELD(boot_record->boot_id,            cursor, err);
    READ_FIELD(boot_record->boot_system_use,    cursor, err);

    *cur = cursor;
    return 0;
}

errno_t getPrimaryVD(
    PrimaryDescriptor_t *primary,
    ByteCursor_t *cur
) {
    if (cur == NULL || primary == NULL) {
        return EINVAL;
    }

    ByteCursor_t cursor = *cur;

    // Filling the struct PrimaryDescryptor_t
    errno_t err;

    READ_VALUE(primary->_unused1,                       cursor, err);
    READ_FIELD(primary->system_id,                      cursor, err);
    READ_FIELD(primary->v_id,                           cursor, err);
    READ_FIELD(primary->_unused2,                       cursor, err);
    READ_FIELD(primary->v_space_size,                   cursor, err);
    READ_FIELD(primary->_unused3,                       cursor, err);
    READ_FIELD(primary->v_set_size,                     cursor, err);
    READ_FIELD(primary->v_sequence_number,              cursor, err);
    READ_FIELD(primary->logical_block_size,             cursor, err);
    READ_FIELD(primary->path_table_size,                cursor, err);
    READ_FIELD(primary->location_typeL_path_table,      cursor, err);
    READ_FIELD(primary->location_opt_typeL_path_table,  cursor, err);
    READ_FIELD(primary->location_typeM_path_table,      cursor, err);
    READ_FIELD(primary->location_opt_typeM_path_table,  cursor, err);
    READ_FIELD(primary->direntry_root,                  cursor, err);
    READ_FIELD(primary->v_set_id,                       cursor, err);
    READ_FIELD(primary->publisher_id,                   cursor, err);
    READ_FIELD(primary->data_preparer_id,               cursor, err);
    READ_FIELD(primary->application_id,                 cursor, err);
    READ_FIELD(primary->copyright_file_id,              cursor, err);
    READ_FIELD(primary->abstract_file_id,               cursor, err);
    READ_FIELD(primary->biographic_file_id,             cursor, err);
    READ_FIELD(primary->v_creation_date_time,           cursor, err);
    READ_FIELD(primary->v_modification_date_time,       cursor, err);
    READ_FIELD(primary->v_expiration_date_time,         cursor, err);
    READ_FIELD(primary->v_effective_date_time,          cursor, err);
    READ_VALUE(primary->file_structure_version,         cursor, err);
    READ_VALUE(primary->_unused4,                       cursor, err);
    READ_FIELD(primary->application_used,               cursor, err);
    READ_FIELD(primary->reserved,                       cursor, err);

    *cur = cursor;
    return 0;
}

errno_t getSetTerminatorVD(
    SetTerminatorDescriptor_t *set_terminator,
    ByteCursor_t *cur
) {
    if (cur == NULL || set_terminator == NULL) {
        return EINVAL;
    }

    ByteCursor_t cursor = *cur;

    // Filling the struct PrimaryDescryptor_t
    errno_t err;

    READ_FIELD(set_terminator->_unused, cursor, err);

    *cur = cursor;
    return 0;
}
