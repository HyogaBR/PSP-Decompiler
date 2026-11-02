#include <errno.h>
#include <string.h>

#include "headers.h"

/*
 * Compile-time validation of the ISO binary structures.
 *
 * If one of these assertions fails, the binary structures do not
 * represent the expected ISO 9660 layout.
 */

_Static_assert(
    sizeof(DescriptorHeaderISO_t) == ISO_DESCRIPTOR_HEADER_SIZE,
    "Invalid ISO descriptor header size"
);

_Static_assert(
    sizeof(DescriptorDateTimeISO_t) == 17,
    "Invalid ISO date/time size"
);

_Static_assert(
    sizeof(BootRecordDescriptorISO_t) == ISO_DESCRIPTOR_DATA_SIZE,
    "Invalid ISO boot record size"
);

_Static_assert(
    sizeof(PrimaryDescriptorISO_t) == ISO_DESCRIPTOR_DATA_SIZE,
    "Invalid ISO primary descriptor size"
);

_Static_assert(
    sizeof(SetTerminatorDescriptorISO_t) == ISO_DESCRIPTOR_DATA_SIZE,
    "Invalid ISO set terminator size"
);

_Static_assert(
    sizeof(DescriptorDataISO_t) == ISO_DESCRIPTOR_DATA_SIZE,
    "Invalid ISO descriptor data size"
);


/* ================================================================
 * Internal helpers
 * ================================================================ */

/*
 * Copies a fixed-size ISO field into a C string.
 *
 * ISO fields are not necessarily null-terminated.
 * The destination always receives a terminating '\0'.
 */
static void copyFixedString(
    char *dst,
    const char *src,
    size_t size
) {
    memcpy(dst, src, size);
    dst[size] = '\0';
}


/*
 * Converts a little-endian byte sequence into a host uint16_t.
 *
 * This does not depend on the host machine's endianness.
 */
static uint16_t decodeLE16(const uint8_t *src) {
    return
        ((uint16_t)src[0]) |
        ((uint16_t)src[1] << 8);
}


/*
 * Converts a little-endian byte sequence into a host uint32_t.
 */
static uint32_t decodeLE32(const uint8_t *src) {
    return
        ((uint32_t)src[0]) |
        ((uint32_t)src[1] << 8) |
        ((uint32_t)src[2] << 16) |
        ((uint32_t)src[3] << 24);
}


/*
 * Converts a big-endian byte sequence into a host uint32_t.
 */
static uint32_t decodeBE32(const uint8_t *src) {
    return
        ((uint32_t)src[0] << 24) |
        ((uint32_t)src[1] << 16) |
        ((uint32_t)src[2] << 8) |
        ((uint32_t)src[3]);
}


/*
 * Decodes a 16-bit ISO both-endian field.
 *
 * ISO 9660 stores the same value in little-endian and big-endian
 * representations. A mismatch indicates malformed or inconsistent
 * data.
 */
static errno_t decodeBothEndian16(
    const uint16_t *little,
    const uint16_t *big,
    uint16_t *out
) {
    if (little == NULL || big == NULL || out == NULL) {
        return EINVAL;
    }

    const uint8_t *little_bytes = (const uint8_t *)little;
    const uint8_t *big_bytes = (const uint8_t *)big;

    uint16_t little_value = decodeLE16(little_bytes);
    uint16_t big_value = (uint16_t)(
        ((uint16_t)big_bytes[0] << 8) |
        ((uint16_t)big_bytes[1])
    );

    if (little_value != big_value) {
        return EILSEQ;
    }

    *out = little_value;

    return 0;
}


/*
 * Decodes a 32-bit ISO both-endian field.
 */
static errno_t decodeBothEndian32(
    const uint32_t *little,
    const uint32_t *big,
    uint32_t *out
) {
    if (little == NULL || big == NULL || out == NULL) {
        return EINVAL;
    }

    uint32_t little_value = decodeLE32(
        (const uint8_t *)little
    );

    uint32_t big_value = decodeBE32(
        (const uint8_t *)big
    );

    if (little_value != big_value) {
        return EILSEQ;
    }

    *out = little_value;

    return 0;
}


/* ================================================================
 * Date/time conversion
 * ================================================================ */

static void convertDescriptorDateTime(
    DescriptorDateTime_t *dst,
    const DescriptorDateTimeISO_t *src
) {
    copyFixedString(dst->year, src->year, 4);
    copyFixedString(dst->month, src->month, 2);
    copyFixedString(dst->day, src->day, 2);
    copyFixedString(dst->hour, src->hour, 2);
    copyFixedString(dst->minute, src->minute, 2);
    copyFixedString(dst->second, src->second, 2);
    copyFixedString(dst->hundredsec, src->hundredsec, 2);

    dst->timezone = src->timezone;
}


/* ================================================================
 * Header conversion
 * ================================================================ */

static void convertDescriptorHeader(
    DescriptorHeader_t *dst,
    const DescriptorHeaderISO_t *src
) {
    dst->type = src->type;

    copyFixedString(dst->id, src->id, sizeof(src->id));

    dst->version = src->version;
}


/* ================================================================
 * Boot record conversion
 * ================================================================ */

static void convertBootRecord(
    BootRecordDescriptor_t *dst,
    const BootRecordDescriptorISO_t *src
) {
    copyFixedString(
        dst->system_id,
        src->system_id,
        sizeof(src->system_id)
    );

    copyFixedString(
        dst->id,
        src->id,
        sizeof(src->id)
    );

    memcpy(
        dst->system_use,
        src->system_use,
        sizeof(dst->system_use)
    );
}


/* ================================================================
 * Primary descriptor conversion
 * ================================================================ */

static errno_t convertPrimaryDescriptor(
    PrimaryDescriptor_t *dst,
    const PrimaryDescriptorISO_t *src
) {
    errno_t err;

    /*
     * Text fields.
     */

    copyFixedString(
        dst->system_id,
        src->system_id,
        sizeof(src->system_id)
    );

    copyFixedString(
        dst->v_id,
        src->v_id,
        sizeof(src->v_id)
    );

    copyFixedString(
        dst->v_set_id,
        src->v_set_id,
        sizeof(src->v_set_id)
    );

    copyFixedString(
        dst->publisher_id,
        src->publisher_id,
        sizeof(src->publisher_id)
    );

    copyFixedString(
        dst->data_preparer_id,
        src->data_preparer_id,
        sizeof(src->data_preparer_id)
    );

    copyFixedString(
        dst->application_id,
        src->application_id,
        sizeof(src->application_id)
    );

    copyFixedString(
        dst->copyright_file_id,
        src->copyright_file_id,
        sizeof(src->copyright_file_id)
    );

    copyFixedString(
        dst->abstract_file_id,
        src->abstract_file_id,
        sizeof(src->abstract_file_id)
    );

    copyFixedString(
        dst->biographic_file_id,
        src->biographic_file_id,
        sizeof(src->biographic_file_id)
    );


    /*
     * Both-endian numeric fields.
     */

    err = decodeBothEndian32(
        &src->v_space_size,
        &src->__v_space_size_msb,
        &dst->v_space_size
    );

    if (err != 0) {
        return err;
    }

    err = decodeBothEndian16(
        &src->v_set_size,
        &src->__v_set_size_msb,
        &dst->v_set_size
    );

    if (err != 0) {
        return err;
    }

    err = decodeBothEndian16(
        &src->v_sequence_number,
        &src->__v_sequence_number_msb,
        &dst->v_sequence_number
    );

    if (err != 0) {
        return err;
    }

    err = decodeBothEndian16(
        &src->logical_block_size,
        &src->__logical_block_size_msb,
        &dst->logical_block_size
    );

    if (err != 0) {
        return err;
    }

    err = decodeBothEndian32(
        &src->path_table_size,
        &src->__path_table_size_msb,
        &dst->path_table_size
    );

    if (err != 0) {
        return err;
    }


    /*
     * Path table locations.
     *
     * Type L locations are little-endian.
     * Type M locations are big-endian.
     */

    dst->location_typeL_path_table = decodeLE32(
        (const uint8_t *)&src->location_typeL_path_table
    );

    dst->location_opt_typeL_path_table = decodeLE32(
        (const uint8_t *)&src->location_opt_typeL_path_table
    );

    dst->location_typeM_path_table = decodeBE32(
        (const uint8_t *)&src->location_typeM_path_table
    );

    dst->location_opt_typeM_path_table = decodeBE32(
        (const uint8_t *)&src->location_opt_typeM_path_table
    );


    /*
     * Raw directory record.
     *
     * We will parse this structure in a later step.
     */

    memcpy(
        dst->direntry_root,
        src->direntry_root,
        sizeof(dst->direntry_root)
    );


    /*
     * Date/time fields.
     */

    convertDescriptorDateTime(
        &dst->v_creation_date_time,
        &src->v_creation_date_time
    );

    convertDescriptorDateTime(
        &dst->v_modification_date_time,
        &src->v_modification_date_time
    );

    convertDescriptorDateTime(
        &dst->v_expiration_date_time,
        &src->v_expiration_date_time
    );

    convertDescriptorDateTime(
        &dst->v_effective_date_time,
        &src->v_effective_date_time
    );


    /*
     * Remaining fields.
     */

    dst->file_structure_version = src->file_structure_version;
    dst->unused = src->_unused4;

    memcpy(
        dst->application_used,
        src->application_used,
        sizeof(dst->application_used)
    );

    memcpy(
        dst->reserved,
        src->reserved,
        sizeof(dst->reserved)
    );

    return 0;
}


/* ================================================================
 * Descriptor data conversion
 * ================================================================ */

static errno_t convertDescriptorData(
    DescriptorData_t *dst,
    const DescriptorDataISO_t *src,
    uint8_t type
) {
    switch (type) {
        case DESCRIPTOR_BOOT_RECORD:
            convertBootRecord(
                &dst->boot_record,
                &src->boot_record
            );

            return 0;

        case DESCRIPTOR_PRIMARY:
            return convertPrimaryDescriptor(
                &dst->primary,
                &src->primary
            );

        case DESCRIPTOR_SET_TERMINATOR:
            memcpy(
                &dst->set_terminator,
                &src->set_terminator,
                sizeof(dst->set_terminator)
            );

            return 0;

        /*
         * Supplementary and Volume Partition descriptors are not
         * interpreted yet.
         *
         * Preserve their bytes instead of stopping the parser.
         */
        default:
            memcpy(
                dst->raw,
                src->raw,
                ISO_DESCRIPTOR_DATA_SIZE
            );

            return 0;
    }
}


/* ================================================================
 * Public API
 * ================================================================ */

errno_t getVolumeDescriptor(
    VolDescriptor_t *vd,
    ByteCursor_t *cur
) {
    if (vd == NULL || cur == NULL) {
        return EINVAL;
    }

    errno_t err = 0;

    ByteCursor_t *transaction = NULL;

    DescriptorHeaderISO_t iso_header = { 0 };
    DescriptorDataISO_t iso_data = { 0 };

    VolDescriptor_t result = { 0 };


    /*
     * Start transaction.
     *
     * The original cursor is not modified until commit.
     */

    err = cursor_fork(&transaction, cur);

    if (err != 0) {
        goto cleanup;
    }


    /*
     * Read the 7-byte descriptor header.
     */

    err = cursor_read(
        transaction,
        &iso_header,
        sizeof(iso_header),
        sizeof(iso_header)
    );

    if (err != 0) {
        goto cleanup;
    }


    /*
     * Validate descriptor identifier.
     *
     * A valid ISO 9660 descriptor must contain "CD001".
     */

    if (memcmp(iso_header.id, "CD001", 5) != 0) {
        err = EINVAL;
        goto cleanup;
    }

    if (iso_header.version != 1) {
        err = EINVAL;
        goto cleanup;
    }


    /*
     * Read the entire 2041-byte payload.
     *
     * Reading the complete payload is important: even when we do not
     * support a descriptor type, the next descriptor must begin at
     * the correct position.
     */

    err = cursor_read(
        transaction,
        iso_data.raw,
        sizeof(iso_data.raw),
        ISO_DESCRIPTOR_DATA_SIZE
    );

    if (err != 0) {
        goto cleanup;
    }


    /*
     * Convert the header.
     */

    convertDescriptorHeader(
        &result.header,
        &iso_header
    );


    /*
     * Convert the descriptor payload.
     */

    err = convertDescriptorData(
        &result.data,
        &iso_data,
        iso_header.type
    );

    if (err != 0) {
        goto cleanup;
    }


    /*
     * Commit transaction.
     *
     * If this fails, the original cursor remains unchanged.
     */

    err = cursor_commit(cur, transaction);

    if (err != 0) {
        goto cleanup;
    }


    /*
     * Only publish the result after the transaction succeeds.
     */

    *vd = result;


cleanup:

    if (transaction != NULL) {
        cursor_close(transaction);
    }

    return err;
}
