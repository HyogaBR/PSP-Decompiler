
#include <corecrt.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "directories.h"
#include "cursor.h"
#include "headers.h"

_Static_assert(
    sizeof(PathTableHeader_t) == 8,
    "PathTableHeader_t must be exactly 8 bytes"
);

_Static_assert(
    sizeof(DirectoryRecordHeader_t) == 33,
    "DirectoryRecordHeader_t must be exactly 33 bytes"
);

static void freePathTableEntries(
    PathTableEntry_t *entries,
    size_t count
) {
    if (entries == NULL)
        return;

    for (size_t i = 0; i < count; i++) {
        free(entries[i].dir_id);
    }

    free(entries);
}

errno_t getPathTable(
    PathTable_t *path_table,
    ByteCursor_t *cur,
    const VolDescriptor_t *primary
) {
    if (path_table == NULL || cur == NULL || primary == NULL)
        return EINVAL;

    if (primary->header.type != DESCRIPTOR_PRIMARY)
        return EINVAL;

    const PrimaryDescriptor_t *pvd = &primary->data.primary;

    if (pvd->logical_block_size == 0 ||
        pvd->path_table_size == 0 ||
        pvd->location_typeL_path_table == 0) {
        return EINVAL;
    }

    errno_t err = 0;

    ByteCursor_t *cursor = NULL;

    PathTable_t result = { 0 };

    uint64_t table_offset =
        (uint64_t)pvd->location_typeL_path_table *
        pvd->logical_block_size;

    if (table_offset > SIZE_MAX)
        return ERANGE;

    /*
     * Create a bounded view of the complete Path Table.
     * Both passes will operate only inside this region.
     */
    err = cursor_slice_at(
        &cursor,
        cur,
        (size_t)table_offset,
        pvd->path_table_size
    );

    if (err != 0)
        goto cleanup;

    size_t table_size = pvd->path_table_size;
    size_t consumed = 0;
    size_t count = 0;

    /*
     * PASS 1:
     * Count the entries.
     */

    while (consumed < table_size) {
        PathTableHeader_t header;

        err = cursor_read(
            cursor,
            &header,
            sizeof(header),
            sizeof(header)
        );

        if (err != 0)
            goto cleanup;

        consumed += sizeof(header);

        size_t id_length = header.dir_id_length;
        size_t padding = id_length & 1;
        size_t remaining = table_size - consumed;

        if (id_length == 0 ||
            id_length + padding > remaining) {
            err = EINVAL;
            goto cleanup;
        }

        err = cursor_skip(cursor, id_length + padding);

        if (err != 0)
            goto cleanup;

        consumed += id_length + padding;
        count++;
    }

    if (consumed != table_size) {
        err = EINVAL;
        goto cleanup;
    }

    /*
     * Allocate the exact number of entries.
     */

    if (count > SIZE_MAX / sizeof(*result.entries)) {
        err = ERANGE;
        goto cleanup;
    }

    result.entries = calloc(
        count,
        sizeof(*result.entries)
    );

    if (result.entries == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    result.count = count;

    /*
     * Reset the cursor to the beginning of the Path Table.
     */

    err = cursor_seek(cursor, (size_t)table_offset);

    if (err != 0)
        goto cleanup;

    consumed = 0;

    /*
     * PASS 2:
     * Read and store every entry.
     */

    for (size_t i = 0; i < count; i++) {
        PathTableEntry_t *entry = &result.entries[i];

        err = cursor_read(
            cursor,
            &entry->header,
            sizeof(entry->header),
            sizeof(entry->header)
        );

        if (err != 0)
            goto cleanup;

        consumed += sizeof(entry->header);

        size_t id_length = entry->header.dir_id_length;
        size_t padding = id_length & 1;

        if (id_length == 0 ||
            id_length + padding > table_size - consumed) {
            err = EINVAL;
            goto cleanup;
        }

        if (id_length == 1 && entry->header.dir_number_parent == 1) {
            char root[] = "ROOT";
            entry->dir_id = malloc(strlen(root) + 1);
            if (entry->dir_id == NULL) {
                err = ENOMEM;
                goto cleanup;
            }

            err = strncpy_s(entry->dir_id, strlen(root) + 1, root, strlen(root) + 1);
            if (err != 0)
                goto cleanup;

            consumed += id_length + padding;

            err = cursor_skip(cursor, id_length + padding);
            if (err != 0)
                goto cleanup;

            continue;
        }

        entry->dir_id = malloc(id_length + 1);

        if (entry->dir_id == NULL) {
            err = ENOMEM;
            goto cleanup;
        }

        err = cursor_read(
            cursor,
            entry->dir_id,
            id_length + 1,
            id_length
        );

        if (err != 0)
            goto cleanup;

        entry->dir_id[id_length] = '\0';

        consumed += id_length;

        if (padding != 0) {
            err = cursor_skip(cursor, padding);

            if (err != 0)
                goto cleanup;

            consumed += padding;
        }
    }

    if (consumed != table_size) {
        err = EINVAL;
        goto cleanup;
    }

    /*
     * Publish the result only after everything succeeds.
     */

    *path_table = result;
    result = (PathTable_t){ 0 };

cleanup:

    if (cursor != NULL)
        cursor_close(cursor);

    freePathTableEntries(result.entries, result.count);

    return err;
}

void freePathTable(PathTable_t *path_table) {
    if (path_table == NULL)
        return;

    for (size_t i = 0; i < path_table->count; i++) {
        free(path_table->entries[i].dir_id);
    }

    free(path_table->entries);

    *path_table = (PathTable_t){0};
}

#define MAX_DIRECTORY_DEPTH 256

static void freeDirectoryRecordEntries(
    DirectoryRecordEntry_t *entries,
    size_t count
) {
    if (entries == NULL)
        return;

    for (size_t i = 0; i < count; i++) {
        free(entries[i].file_id);
        free(entries[i].system_use);
    }

    free(entries);
}

static errno_t getDirectoryOffset(
    size_t *offset,
    uint32_t extent,
    uint8_t extended_attribute_length,
    uint16_t block_size
) {
    if (offset == NULL || block_size == 0)
        return EINVAL;

    uint64_t block =
        (uint64_t)extent + extended_attribute_length;

    uint64_t result = block * block_size;

    if (result > SIZE_MAX)
        return ERANGE;

    *offset = (size_t)result;

    return 0;
}

static errno_t walkDirectoryRecords(
    ByteCursor_t *source,
    uint32_t extent,
    uint8_t extended_attribute_length,
    uint32_t data_length,
    uint16_t block_size,
    size_t depth,
    DirectoryRecordEntry_t *entries,
    size_t capacity,
    size_t *count
) {
    if (depth > MAX_DIRECTORY_DEPTH || data_length == 0)
        return EINVAL;

    errno_t err = 0;

    ByteCursor_t *cursor = NULL;

    size_t directory_offset = 0;

    err = getDirectoryOffset(
        &directory_offset,
        extent,
        extended_attribute_length,
        block_size
    );

    if (err != 0)
        goto cleanup;

    err = cursor_slice_at(
        &cursor,
        source,
        directory_offset,
        data_length
    );

    if (err != 0)
        goto cleanup;

    size_t consumed = 0;

    while (consumed < data_length) {
        uint8_t record_length = 0;

        err = cursor_read(
            cursor,
            &record_length,
            sizeof(record_length),
            sizeof(record_length)
        );

        if (err != 0)
            goto cleanup;

        /*
         * Padding until the next logical block.
         */
        if (record_length == 0) {
            size_t padding =
                block_size - (consumed % block_size);

            size_t remaining = data_length - consumed;

            if (padding > remaining)
                padding = remaining;

            err = cursor_skip(cursor, padding - 1);

            if (err != 0)
                goto cleanup;

            consumed += padding;
            continue;
        }

        size_t block_remaining =
            block_size - (consumed % block_size);

        if (record_length < sizeof(DirectoryRecordHeader_t) ||
            record_length > block_remaining ||
            record_length > data_length - consumed) {
            err = EINVAL;
            goto cleanup;
        }

        DirectoryRecordHeader_t header = { 0 };

        header.dir_record_length = record_length;

        err = cursor_read(
            cursor,
            (uint8_t *)&header + 1,
            sizeof(header) - 1,
            sizeof(header) - 1
        );

        if (err != 0)
            goto cleanup;

        consumed += sizeof(header);

        size_t id_length = header.file_id_length;

        size_t padding = (id_length % 2 == 0) ? 1 : 0;

        size_t payload_size =
            record_length - sizeof(header);

        if (id_length == 0 ||
            id_length + padding > payload_size) {
            err = EINVAL;
            goto cleanup;
        }

        size_t system_use_length =
            payload_size - id_length - padding;

        uint8_t file_id[UINT8_MAX];

        err = cursor_read(
            cursor,
            file_id,
            sizeof(file_id),
            id_length
        );

        if (err != 0)
            goto cleanup;

        consumed += id_length;

        if (padding != 0) {
            err = cursor_skip(cursor, padding);

            if (err != 0)
                goto cleanup;

            consumed += padding;
        }

        /*
         * Store record.
         */
        if (entries != NULL) {
            if (*count >= capacity) {
                err = ERANGE;
                goto cleanup;
            }

            DirectoryRecordEntry_t *entry = &entries[*count];

            entry->header = header;

            entry->file_id = malloc(id_length + 1);

            if (entry->file_id == NULL) {
                err = ENOMEM;
                goto cleanup;
            }

            memcpy(entry->file_id, file_id, id_length);

            entry->file_id[id_length] = '\0';

            if (system_use_length != 0) {
                entry->system_use = malloc(system_use_length);

                if (entry->system_use == NULL) {
                    err = ENOMEM;
                    goto cleanup;
                }

                err = cursor_read(
                    cursor,
                    entry->system_use,
                    system_use_length,
                    system_use_length
                );

                if (err != 0)
                    goto cleanup;
            }
        } else {
            err = cursor_skip(cursor, system_use_length);

            if (err != 0)
                goto cleanup;
        }

        consumed += system_use_length;

        if (*count == SIZE_MAX) {
            err = ERANGE;
            goto cleanup;
        }

        (*count)++;

        /*
         * Skip recursive traversal for "." and "..".
         */
        bool special_directory =
            id_length == 1 &&
            (file_id[0] == 0x00 || file_id[0] == 0x01);

        if (header.flags.is_directory && !special_directory) {
            err = walkDirectoryRecords(
                source,
                header.location_extent,
                header.extended_attribute_record_length,
                header.data_length,
                block_size,
                depth + 1,
                entries,
                capacity,
                count
            );

            if (err != 0)
                goto cleanup;
        }
    }

cleanup:

    if (cursor != NULL)
        cursor_close(cursor);

    return err;
}

errno_t getDirectoryRecords(
    DirectoryRecord_t *directory_records,
    ByteCursor_t *cur,
    const VolDescriptor_t *primary
) {
    if (directory_records == NULL || cur == NULL || primary == NULL)
        return EINVAL;

    if (primary->header.type != DESCRIPTOR_PRIMARY)
        return EINVAL;

    const PrimaryDescriptor_t *pvd = &primary->data.primary;

    if (pvd->logical_block_size == 0)
        return EINVAL;

    errno_t err = 0;

    DirectoryRecord_t result = { 0 };

    DirectoryRecordHeader_t root = { 0 };

    memcpy(
        &root,
        pvd->direntry_root,
        sizeof(root)
    );

    if (root.dir_record_length != 34 ||
        root.file_id_length != 1 ||
        pvd->direntry_root[33] != 0x00 ||
        !root.flags.is_directory) {
        err = EINVAL;
        goto cleanup;
    }

    /*
     * PASS 1: Count all records.
     */
    err = walkDirectoryRecords(
        cur,
        root.location_extent,
        root.extended_attribute_record_length,
        root.data_length,
        pvd->logical_block_size,
        0,
        NULL,
        0,
        &result.count
    );

    if (err != 0)
        goto cleanup;

    if (result.count > SIZE_MAX / sizeof(*result.entries)) {
        err = ERANGE;
        goto cleanup;
    }

    result.entries = calloc(
        result.count,
        sizeof(*result.entries)
    );

    if (result.entries == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    /*
     * PASS 2: Read all records.
     */
    size_t index = 0;

    err = walkDirectoryRecords(
        cur,
        root.location_extent,
        root.extended_attribute_record_length,
        root.data_length,
        pvd->logical_block_size,
        0,
        result.entries,
        result.count,
        &index
    );

    if (err != 0)
        goto cleanup;

    if (index != result.count) {
        err = EINVAL;
        goto cleanup;
    }

    *directory_records = result;
    result = (DirectoryRecord_t){ 0 };

cleanup:

    freeDirectoryRecordEntries(result.entries, result.count);

    return err;
}

void freeDirectoryRecords(DirectoryRecord_t *directory_records) {
    if (directory_records == NULL)
        return;

    freeDirectoryRecordEntries(
        directory_records->entries,
        directory_records->count
    );

    *directory_records = (DirectoryRecord_t){ 0 };
}
