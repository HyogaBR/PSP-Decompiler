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

typedef struct Node {
    VolDescriptor_t *vd;
    struct Node *next;
} Node_t;


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

    new_node->vd = value;
    new_node->next = *head;

    *head = new_node;

    return 0;
}

void executeAtAll(
    Node_t *head,
    void (*callback)(VolDescriptor_t *)
) {
    if (callback == NULL) {
        return;
    }

    Node_t *actual = head;

    while (actual != NULL) {
        callback(actual->vd);
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

        free(temp->vd);
        free(temp);
    }

    *head = NULL;
}


/* ================================================================
 * Descriptor printing
 * ================================================================ */

void printElement(VolDescriptor_t *vd) {
    if (vd == NULL) {
        return;
    }

    printf("ID: %s\n", vd->header.id);
    printf(" - Type: %u\n", (unsigned)vd->header.type);
    printf(" - Version: %u\n", (unsigned)vd->header.version);

    switch (vd->header.type) {
        case DESCRIPTOR_BOOT_RECORD:
            printf(" - System ID: %s\n",
                vd->data.boot_record.system_id);
            break;

        case DESCRIPTOR_PRIMARY:
            printf(" - System ID: %s\n",
                vd->data.primary.system_id);

            printf(" - Volume ID: %s\n",
                vd->data.primary.v_id);

            printf(" - Copyright file: %s\n",
                vd->data.primary.copyright_file_id);

            printf(" - Volume space size: %u\n",
                (unsigned)vd->data.primary.v_space_size);

            printf(" - Logical block size: %u\n",
                (unsigned)vd->data.primary.logical_block_size);
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

    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;

    const byte *sourcefile = NULL;
    ByteCursor_t *cursor = NULL;

    Node_t *VDlist = NULL;

    if (argc < 2) {
        fwprintf(
            stderr,
            L"Usage: %ls <source_file>\n",
            argv[0]
        );

        return EINVAL;
    }

    // create mapping
    hFile = CreateFileW(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        err = (errno_t)GetLastError();
        goto cleanup;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size)) {
        err = (errno_t)GetLastError();
        goto cleanup;
    }

    if (size.QuadPart <= 0 ||
        (uint64_t)size.QuadPart > SIZE_MAX) {
        err = EINVAL;
        goto cleanup;
    }

    hMap = CreateFileMappingW(
        hFile,
        NULL,
        PAGE_READONLY,
        0, 0,
        NULL
    );
    if (hMap == NULL) {
        err = (errno_t)GetLastError();
        goto cleanup;
    }

    sourcefile = MapViewOfFile(
        hMap,
        FILE_MAP_READ,
        0, 0, 0
    );
    if (sourcefile == NULL) {
        err = (errno_t)GetLastError();
        goto cleanup;
    }

    err = cursor_init(
        &cursor,
        sourcefile,
        (size_t)size.QuadPart
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
    }

    if (!found_terminator) goto cleanup;

    executeAtAll(VDlist, printElement);

    cleanup:

    if (cursor != NULL) {
        cursor_close(cursor);
    }

    if (sourcefile != NULL) {
        UnmapViewOfFile(sourcefile);
    }

    if (hMap != NULL) {
        CloseHandle(hMap);
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return err;
}
