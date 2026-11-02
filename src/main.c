#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "cursor.h"
#include "headers.h"

typedef struct FileInfo {
    size_t readed;
    FILE *fptr;
    size_t len;
    char *path;
    uint8_t *content;
} FileInfo_t;

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
 * File handling
 * ================================================================ */

errno_t loadFile(FileInfo_t **file, const char *path);
void freeFileInfo(FileInfo_t *file);


errno_t loadFile(FileInfo_t **file, const char *path) {
    if (file == NULL || path == NULL) {
        return EINVAL;
    }

    *file = NULL;

    errno_t err = 0;

    FileInfo_t *result = calloc(1, sizeof(FileInfo_t));

    if (result == NULL) {
        return ENOMEM;
    }

    result->path = _strdup(path);

    if (result->path == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    err = fopen_s(&result->fptr, result->path, "rb");

    if (err != 0) {
        goto cleanup;
    }

    /*
     * Determine file size.
     */

    if (_fseeki64(result->fptr, 0, SEEK_END) != 0) {
        err = errno ? errno : EIO;
        goto cleanup;
    }

    __int64 file_size = _ftelli64(result->fptr);

    if (file_size < 0) {
        err = EIO;
        goto cleanup;
    }

    /*
     * Ensure the file size fits into size_t.
     */

    if ((uintmax_t)file_size > SIZE_MAX) {
        err = EOVERFLOW;
        goto cleanup;
    }

    result->len = (size_t)file_size;

    /*
     * Return to the beginning of the file.
     */

    if (_fseeki64(result->fptr, 0, SEEK_SET) != 0) {
        err = errno ? errno : EIO;
        goto cleanup;
    }

    /*
     * Allocate memory.
     *
     * Allocate at least one byte even for an empty file.
     */

    result->content = malloc(
        result->len > 0 ? result->len : 1
    );

    if (result->content == NULL) {
        err = ENOMEM;
        goto cleanup;
    }

    /*
     * Read the complete file.
     */

    result->readed = fread(
        result->content,
        sizeof(uint8_t),
        result->len,
        result->fptr
    );

    if (result->readed != result->len) {
        err = ferror(result->fptr)
            ? (errno ? errno : EIO)
            : EIO;

        goto cleanup;
    }

    *file = result;

    return 0;


cleanup:

    freeFileInfo(result);

    return err;
}


void freeFileInfo(FileInfo_t *file) {
    if (file == NULL) {
        return;
    }

    free(file->content);

    if (file->fptr != NULL) {
        fclose(file->fptr);
    }

    free(file->path);
    free(file);
}


/* ================================================================
 * Main
 * ================================================================ */

int main(int argc, char **argv) {
    errno_t err = 0;

    FileInfo_t *sourcefile = NULL;
    ByteCursor_t *cursor = NULL;

    Node_t *VDlist = NULL;

    if (argc < 2) {
        fprintf(
            stderr,
            "Usage: %s <source_file>\n",
            argv[0]
        );

        return EINVAL;
    }


    /*
     * Load ISO image.
     */

    err = loadFile(&sourcefile, argv[1]);

    if (err != 0) {
        fprintf(
            stderr,
            "Error loading file: %d\n",
            (int)err
        );

        goto cleanup;
    }


    /*
     * Initialize cursor.
     */

    err = cursor_init(
        &cursor,
        sourcefile->content,
        sourcefile->readed
    );

    if (err != 0) {
        fprintf(
            stderr,
            "Error initializing cursor: %d\n",
            (int)err
        );

        goto cleanup;
    }


    /*
     * ISO 9660 Volume Descriptor Set starts at sector 16.
     */

    err = cursor_skip(
        cursor,
        16 * SECTOR_LEN
    );

    if (err != 0) {
        fprintf(
            stderr,
            "Error locating Volume Descriptor Set: %d\n",
            (int)err
        );

        goto cleanup;
    }


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

        /*
         * Parse one complete descriptor.
         *
         * getVolumeDescriptor uses an internal transaction.
         * On failure, the cursor remains unchanged.
         */

        err = getVolumeDescriptor(temp, cursor);

        if (err != 0) {
            free(temp);

            fprintf(
                stderr,
                "Error reading Volume Descriptor: %d\n",
                (int)err
            );

            goto cleanup;
        }

        /*
         * Store descriptor in the linked list.
         */

        err = addNode(&VDlist, temp);

        if (err != 0) {
            free(temp);
            goto cleanup;
        }

        /*
         * The terminator marks the end of the descriptor sequence.
         */

        if (temp->header.type == DESCRIPTOR_SET_TERMINATOR) {
            found_terminator = true;
            break;
        }
    }

    if (!found_terminator) {
        err = ENODATA;

        fprintf(
            stderr,
            "Volume Descriptor Set Terminator not found.\n"
        );

        goto cleanup;
    }


    /*
     * Display parsed descriptors.
     */

    executeAtAll(VDlist, printElement);


cleanup:

    freeList(&VDlist);

    cursor_close(cursor);

    freeFileInfo(sourcefile);

    return err == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
