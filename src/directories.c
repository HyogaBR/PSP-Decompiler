
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
