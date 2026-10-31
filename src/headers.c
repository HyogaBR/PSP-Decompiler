#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "headers.h"
#include "cursor.h"

#define READ_VALUE(dst, cursor, err)                                    \
    do {                                                                \
        (err) = cursor_read((cursor), sizeof((dst)), (byte *)&(dst));   \
        if ((err) != 0) goto cleanup;                                     \
    } while (0)

#define COPY_VALUE(dst, src, err)                               \
    do {                                                        \
    memset(&(dst), 0, sizeof((dst)));                           \
    (err) = memcpy_s(&(dst), sizeof(dst), &(src), sizeof(src)); \
    if ((err) != 0) goto cleanup;                                 \
    } while(0)


errno_t getDescriptorHeaderISO(
    DescriptorHeaderISO_t *header,
    ByteCursor_t *cur
) {
    errno_t err = 0;
    if (header == NULL || cur == NULL) return EINVAL;

    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    READ_VALUE(*header, cursor, err);

    err = cursor_merge(cur, cursor);

    cleanup:
    cursor_close(cursor);
    return err;
}

errno_t getDescriptorDataISO(
    DescriptorDataISO_t *data,
    DescriptorType_t dataType,
    ByteCursor_t *cur
) {
    errno_t err = 0;
    if (data == NULL || cur == NULL) return EINVAL;

    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    switch (dataType) {
        case DESCRIPTOR_BOOT_RECORD:
            READ_VALUE(data->boot_record, cursor, err);
            break;
        case DESCRIPTOR_PRIMARY:
            READ_VALUE(data->primary, cursor, err);
            break;
        case DESCRIPTOR_SET_TERMINATOR:
            READ_VALUE(data->set_terminator, cursor, err);
            break;
        default:
            err = EINVAL;
            goto cleanup;
    }

    err = cursor_merge(cur, cursor);

    cleanup:
    cursor_close(cursor);
    return err;
}

errno_t getDescriptorDateTimeISO(
    DescriptorDateTimeISO_t *dt,
    ByteCursor_t *cur
) {
    errno_t err = 0;
    if (dt == NULL || cur == NULL) return EINVAL;

    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    READ_VALUE(*dt, cursor, err);

    err = cursor_merge(cur, cursor);

    cleanup:
    cursor_close(cursor);
    return err;
}

errno_t getDescriptorHeader(
    DescriptorHeader_t *header,
    ByteCursor_t *cur
) {
    errno_t err = 0;
    if (header == NULL || cur == NULL) return EINVAL;


    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    DescriptorHeaderISO_t headerISO;
    err = getDescriptorHeaderISO(&headerISO, cursor);
    if (err != 0) goto cleanup;

    DescriptorHeader_t tempHd = {0};

    COPY_VALUE(tempHd.type, headerISO.type, err);
    COPY_VALUE(tempHd.id, headerISO.id, err);
    COPY_VALUE(tempHd.version, headerISO.version, err);

    err = cursor_merge(cur, cursor);
    if (err != 0) goto cleanup;

    *header = tempHd;

    cleanup:
    cursor_close(cursor);
    return err;
}

errno_t getDescriptorData(
    DescriptorData_t *data,
    DescriptorType_t dataType,
    ByteCursor_t *cur
) {
    errno_t err = 0;
    if (data == NULL || cur == NULL) return EINVAL;

    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    DescriptorDataISO_t dataISO;
    err = getDescriptorDataISO(&dataISO, dataType, cursor);
    if (err != 0) goto cleanup;

    DescriptorData_t tempData = {0};

    switch (dataType) {
        case DESCRIPTOR_BOOT_RECORD:
            COPY_VALUE(tempData.boot_record.system_id, dataISO.boot_record.system_id, err);
            COPY_VALUE(tempData.boot_record.id, dataISO.boot_record.id, err);
            COPY_VALUE(tempData.boot_record.system_use, dataISO.boot_record.system_use, err);
            break;

        case DESCRIPTOR_PRIMARY:
            COPY_VALUE(tempData.primary.system_id, dataISO.primary.system_id, err);
            COPY_VALUE(tempData.primary.v_id, dataISO.primary.v_id, err);
            COPY_VALUE(tempData.primary.v_space_size, dataISO.primary.v_space_size, err);
            COPY_VALUE(tempData.primary.v_set_size, dataISO.primary.v_set_size, err);
            COPY_VALUE(tempData.primary.v_sequence_number, dataISO.primary.v_sequence_number, err);
            COPY_VALUE(tempData.primary.logical_block_size, dataISO.primary.logical_block_size, err);
            COPY_VALUE(tempData.primary.path_table_size, dataISO.primary.path_table_size, err);
            COPY_VALUE(tempData.primary.location_path_table, dataISO.primary.location_typeL_path_table, err);
            COPY_VALUE(tempData.primary.location_opt_path_table, dataISO.primary.location_opt_typeL_path_table, err);
            COPY_VALUE(tempData.primary.direntry_root, dataISO.primary.direntry_root, err);
            COPY_VALUE(tempData.primary.v_set_id, dataISO.primary.v_set_id, err);
            COPY_VALUE(tempData.primary.publisher_id, dataISO.primary.publisher_id, err);
            COPY_VALUE(tempData.primary.data_preparer_id, dataISO.primary.data_preparer_id, err);
            COPY_VALUE(tempData.primary.application_id, dataISO.primary.application_id, err);
            COPY_VALUE(tempData.primary.copyright_file_id, dataISO.primary.copyright_file_id, err);
            COPY_VALUE(tempData.primary.abstract_file_id, dataISO.primary.abstract_file_id, err);
            COPY_VALUE(tempData.primary.biographic_file_id, dataISO.primary.biographic_file_id, err);
            COPY_VALUE(tempData.primary.v_creation_date_time, dataISO.primary.v_creation_date_time, err);
            COPY_VALUE(tempData.primary.v_modification_date_time, dataISO.primary.v_modification_date_time, err);
            COPY_VALUE(tempData.primary.v_expiration_date_time, dataISO.primary.v_expiration_date_time, err);
            COPY_VALUE(tempData.primary.v_effective_date_time, dataISO.primary.v_effective_date_time, err);
            COPY_VALUE(tempData.primary.file_structure_version, dataISO.primary.file_structure_version, err);
            COPY_VALUE(tempData.primary.application_used, dataISO.primary.application_used, err);
            COPY_VALUE(tempData.primary.reserved, dataISO.primary.reserved, err);
            break;

        case DESCRIPTOR_SET_TERMINATOR:
            COPY_VALUE(tempData.set_terminator._unused, dataISO.set_terminator._unused, err);
            break;

        default:
            err = EINVAL;
            goto cleanup;
    }

    err = cursor_merge(cur, cursor);
    if (err != 0) goto cleanup;

    *data = tempData;

    cleanup:
    cursor_close(cursor);
    return err;
}

errno_t getDescriptorDateTime(
    DescriptorDateTime_t *dt,
    ByteCursor_t *cur
) {
    errno_t err = 0;

    if (dt == NULL || cur == NULL) return EINVAL;

    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    DescriptorDateTimeISO_t dtISO;
    err = getDescriptorDateTimeISO(&dtISO, cursor);
    if (err != 0) goto cleanup;

    DescriptorDateTime_t tempDT = {0};

    COPY_VALUE(tempDT.year, dtISO.year, err);
    COPY_VALUE(tempDT.month, dtISO.month, err);
    COPY_VALUE(tempDT.day, dtISO.day, err);
    COPY_VALUE(tempDT.hour, dtISO.hour, err);
    COPY_VALUE(tempDT.minute, dtISO.minute, err);
    COPY_VALUE(tempDT.second, dtISO.second, err);
    COPY_VALUE(tempDT.hundredsec, dtISO.hundredsec, err);
    COPY_VALUE(tempDT.timezone, dtISO.timezone, err);

    err = cursor_merge(cur, cursor);
    if (err != 0) goto cleanup;

    *dt = tempDT;

    cleanup:
    cursor_close(cursor);
    return err;
}

errno_t getVolumeDescriptor(
    VolDescriptor_t *vd,
    ByteCursor_t *cur
) {
    errno_t err = 0;
    if (cur == NULL || vd == NULL) return EINVAL;

    ByteCursor_t *cursor = cursor_fork(cur);
    if (cursor == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    VolDescriptor_t tempVD = {0};

    // Filling the header of struct VolumeDescriptor_t
    err = getDescriptorHeader(&tempVD.header, cursor);
    if (err != 0) goto cleanup;

    if (memcmp(tempVD.header.id, "CD001", 5) != 0) {
       err = EINVAL;
       goto cleanup;
    }

    if (tempVD.header.version != 1) {
        err = EINVAL;
        goto cleanup;
    }

    err = getDescriptorData(&tempVD.data, tempVD.header.type, cursor);
    if (err != 0) goto cleanup;

    err = cursor_merge(cur, cursor);
    if (err != 0) goto cleanup;

    *vd = tempVD;

    cleanup:
    cursor_close(cursor);
    return err;
}
