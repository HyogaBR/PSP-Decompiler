#include <corecrt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "headers.h"

typedef struct FileInfo {
    size_t    readed;
    FILE       *fptr;
    size_t       len;
    char       *path;
    uint8_t *content;
} FileInfo_t;

errno_t loadFile(FileInfo_t **file, char *path);
void freeFileInfo(FileInfo_t *file);

static void logText(const char *label, const uint8_t *data, size_t size)
{
    printf("  %-20s: %.*s\n",
           label, (int)size, (const char *)data);
}

void logVolumeDescriptor(const VolDescriptor_t *vd)
{
    if (vd == NULL) {
        printf("[VolumeDescriptor] NULL\n");
        return;
    }

    printf("\n========== Volume Descriptor ==========\n");
    printf("Type:    %u\n", (unsigned)vd->header.type);
    logText("Identifier", vd->header.id, sizeof(vd->header.id));
    printf("Version: %u\n", (unsigned)vd->header.version);

    switch (vd->header.type) {
        case DESCRIPTOR_BOOT_RECORD:
            printf("\n-- Boot Record --\n");
            logText("Boot system ID",
                    vd->data.boot_record.boot_system_id, 32);
            logText("Boot ID",
                    vd->data.boot_record.boot_id, 32);
            break;

        case DESCRIPTOR_PRIMARY:
            printf("\n-- Primary Volume Descriptor --\n");
            logText("System ID",
                    vd->data.primary.system_id, 32);
            logText("Volume ID",
                    vd->data.primary.v_id, 32);
            logText("Volume set ID",
                    vd->data.primary.v_set_id, 128);
            logText("Publisher ID",
                    vd->data.primary.publisher_id, 128);
            logText("Application ID",
                    vd->data.primary.application_id, 128);

            printf("  Logical block size  : %u (raw first byte)\n",
                   (unsigned)vd->data.primary.logical_block_size[0]);

            break;

        case DESCRIPTOR_SET_TERMINATOR:
            printf("\n-- Volume Set Terminator --\n");
            printf("  No additional fields to display.\n");
            break;

        default:
            printf("\nUnknown descriptor type.\n");
            break;
    }

    printf("=======================================\n\n");
}

int main(int argc, char **argv) {
    errno_t err;
    FileInfo_t *sourcefile = NULL;
    VolDescriptor_t my_first_vd;

    if (argc < 2) {
        fprintf(stderr, "Erro: Uso correto: %s <arquivo_origem>\n", argv[0]);
        return EINVAL;
    }

    loadFile(&sourcefile, argv[1]);

    ByteCursor_t cursor = {
        .ptr = sourcefile->content,
        .remaining = sourcefile->readed
    };

    cursor.ptr += 16 * SECTOR_LEN;
    cursor.remaining -= 16 * SECTOR_LEN;

    err = getVolumeDescriptor(&my_first_vd, &cursor);
    if (err != 0) {
        fprintf(stderr, "Erro ao ler Volume Descriptor: %d\n", err);
        freeFileInfo(sourcefile);
        return err;
    }

    logVolumeDescriptor(&my_first_vd);

    printf("Volume set ID: %.*s\n",
           (int)sizeof(vd->data.primary.v_set_id),
           (const char *)vd->data.primary.v_set_id);

    freeFileInfo(sourcefile);
    return EXIT_SUCCESS;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------

errno_t loadFile(FileInfo_t **file, char *path) {
    errno_t err;
    if (path == NULL) {
        return EINVAL;
    }

    *file = (FileInfo_t *)malloc(sizeof(FileInfo_t));
    if (*file == NULL) {
        return ENOMEM;
    }

    (*file)->path = _strdup(path);
    if ((*file)->path == NULL) {
        return ENOMEM;
    }

    err = fopen_s(&((*file)->fptr), (*file)->path, "rb+");
    if (err != 0) {
        return err;
    }

    fseek((*file)->fptr, 0, SEEK_END);
    (*file)->len = ftell((*file)->fptr);
    fseek((*file)->fptr, 0, SEEK_SET);

    (*file)->content = malloc((*file)->len * sizeof(uint8_t));
    if ((*file)->content == NULL){
        return ENOMEM;
    }

    (*file)->readed = fread((*file)->content, sizeof(uint8_t), (*file)->len, (*file)->fptr);

    err = ferror((*file)->fptr);
    if (err != 0) {
        return err;
    }

    return 0;
}

void freeFileInfo(FileInfo_t *file) {
    free(file->content);
    fclose(file->fptr);
    free(file->path);
    free(file);
}
