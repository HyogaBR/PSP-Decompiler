#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "cursor.h"
#include "headers.h"
#include "directories.h"

typedef struct Node {
    void *value;
    struct Node *next;
} Node_t;

typedef struct WinFile {
    HANDLE hFile;
    HANDLE hMap;
    LARGE_INTEGER size;
    const byte *ptr;
} WinFile_t;

static void closeFile(WinFile_t *file) {
    if (file == NULL)
        return;

    if (file->ptr != NULL) {
        UnmapViewOfFile(file->ptr);
        file->ptr = NULL;
    }

    if (file->hMap != NULL) {
        CloseHandle(file->hMap);
        file->hMap = NULL;
    }

    if (file->hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(file->hFile);
        file->hFile = INVALID_HANDLE_VALUE;
    }

    file->size.QuadPart = 0;
}

static errno_t openFile(WinFile_t *file, const wchar_t *path) {
    if (path == NULL || file == NULL)
        return EINVAL;

    file->hFile = INVALID_HANDLE_VALUE;
    file->hMap = NULL;
    file->ptr = NULL;
    file->size.QuadPart = 0;

    file->hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file->hFile == INVALID_HANDLE_VALUE)
        goto win_error;

    if (!GetFileSizeEx(file->hFile, &file->size))
        goto win_error;

    if (file->size.QuadPart <= 0 ||
        (uint64_t)file->size.QuadPart > SIZE_MAX) {
        closeFile(file);
        return EINVAL;
    }

    file->hMap = CreateFileMappingW(
        file->hFile,
        NULL,
        PAGE_READONLY,
        0, 0,
        NULL
    );

    if (file->hMap == NULL)
        goto win_error;

    file->ptr = MapViewOfFile(
        file->hMap,
        FILE_MAP_READ,
        0, 0, 0
    );

    if (file->ptr == NULL)
        goto win_error;

    return 0;

win_error:
    closeFile(file);
    return EIO;
}


/* ================================================================
 * Linked list
 * ================================================================ */

errno_t addNode(Node_t **head, VolDescriptor_t *value) {
    if (head == NULL || value == NULL) {
        return EINVAL;
    }

    Node_t *new_node = malloc(sizeof(Node_t));

    if (new_node == NULL) {
        return ENOMEM;
    }

    new_node->value = value;
    new_node->next = *head;

    *head = new_node;

    return 0;
}

void executeAtAll(
    Node_t *head,
    void (*callback)(void *)
) {
    if (callback == NULL) {
        return;
    }

    Node_t *actual = head;

    while (actual != NULL) {
        callback(actual->value);
        actual = actual->next;
    }
}

void freeList(Node_t **head) {
    if (head == NULL) {
        return;
    }

    Node_t *actual = *head;

    while (actual != NULL) {
        Node_t *temp = actual;

        actual = actual->next;

        free(temp->value);
        free(temp);
    }

    *head = NULL;
}


/* ================================================================
 * Descriptor printing
 * ================================================================ */

void printElement(void *vd) {
    if (vd == NULL) {
        return;
    }

    VolDescriptor_t *volume = (VolDescriptor_t *)vd;

    printf("ID: %s\n", volume->header.id);
    printf(" - Type: %u\n", (unsigned)volume->header.type);
    printf(" - Version: %u\n", (unsigned)volume->header.version);

    switch (volume->header.type) {
        case DESCRIPTOR_BOOT_RECORD:
            printf(" - System ID: %s\n",
                volume->data.boot_record.system_id);
            break;

        case DESCRIPTOR_PRIMARY:
            printf(" - System ID: %s\n",
                volume->data.primary.system_id);

            printf(" - Volume ID: %s\n",
                volume->data.primary.v_id);

            printf(" - Copyright file: %s\n",
                volume->data.primary.copyright_file_id);

            printf(" - Volume space size: %u\n",
                (unsigned)volume->data.primary.v_space_size);

            printf(" - Logical block size: %u\n",
                (unsigned)volume->data.primary.logical_block_size);

            printf(" - Path table size: %u\n",
                (unsigned)volume->data.primary.path_table_size);
            break;

        case DESCRIPTOR_SET_TERMINATOR:
            printf(" - Set Terminator\n");
            break;

        default:
            printf(" - Unsupported descriptor type\n");
            break;
    }

    printf("\n");
}

/* ================================================================
 * Main
 * ================================================================ */

int wmain(int argc, wchar_t **argv) {
    errno_t err = 0;

    WinFile_t sourcefile = {
        .hFile = INVALID_HANDLE_VALUE,
        .hMap = NULL,
        .size = { .QuadPart = 0 },
        .ptr = NULL
    };

    ByteCursor_t *cursor = NULL;

    VolDescriptor_t *primary = NULL;

    Node_t *VDlist = NULL;

    if (argc < 2) {
        fwprintf(
            stderr,
            L"Usage: %ls <source_file>\n",
            argv[0]
        );

        return EINVAL;
    }

    err = openFile(&sourcefile, argv[1]);
    if (err != 0) goto cleanup;

    err = cursor_init(
        &cursor,
        sourcefile.ptr,
        (size_t)sourcefile.size.QuadPart
    );
    if (err != 0) goto cleanup;

    err = cursor_skip(cursor, 16 * SECTOR_LEN);
    if (err != 0) goto cleanup;

    /*
    * Read Volume Descriptors.
    *
    * Each descriptor occupies exactly 2048 bytes.
    */

    bool found_terminator = false;
    bool found_primary = false;

    while (cursor_remaining(cursor) >= SECTOR_LEN) {
        VolDescriptor_t *temp = malloc(sizeof(VolDescriptor_t));
        if (temp == NULL) {
            err = ENOMEM;
            goto cleanup;
        }

        err = getVolumeDescriptor(temp, cursor);
        if (err != 0) {
            free(temp);
            goto cleanup;
        }

        err = addNode(&VDlist, temp);
        if (err != 0) {
            free(temp);
            goto cleanup;
        }

        if (temp->header.type == DESCRIPTOR_SET_TERMINATOR) {
            found_terminator = true;
            break;
        }

        if (temp->header.type == DESCRIPTOR_PRIMARY && !found_primary) {
            found_primary = true;
            primary = temp;
        }
    }

    if (!found_terminator) {
        err = ENODATA;
        goto cleanup;
    }

    executeAtAll(VDlist, printElement);

    // Reading the Path Table
    if (primary == NULL) {
        err = ENODATA;
        goto cleanup;
    }

    PathTable_t path_table = { 0 };

    err = getPathTable(&path_table, cursor, primary);

    if (err != 0)
        goto cleanup;

    for (size_t i = 0; i < path_table.count; i++) {
        PathTableEntry_t *entry = &path_table.entries[i];

        printf(
            "[%zu] %s | LBA: %u | Parent: %u\n",
            i + 1,
            entry->dir_id,
            (unsigned)entry->header.location_extent,
            (unsigned)entry->header.dir_number_parent
        );
    }

    DirectoryRecord_t directory_records = { 0 };

    err = getDirectoryRecords(&directory_records, cursor, primary);

    if (err != 0)
        goto cleanup;

    for (size_t i = 0; i < directory_records.count; i++) {
        DirectoryRecordEntry_t *entry = &directory_records.entries[i];

        printf(
            "%s | %s | %u bytes | LBA: %u\n",
            entry->header.flags.is_directory ? "DIR" : "FILE",
            entry->file_id,
            entry->header.data_length,
            entry->header.location_extent
        );
    }

    cleanup:

    freeDirectoryRecords(&directory_records);

    freePathTable(&path_table);

    if (VDlist != NULL) {
        freeList(&VDlist);
    }

    if (cursor != NULL) {
        cursor_close(cursor);
    }

    closeFile(&sourcefile);

    return err;
}
