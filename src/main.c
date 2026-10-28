#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "structs.h"

#define SECTOR_LEN 2048

errno_t loadFile(FileInfo_t **file, char *path);
void freeFileInfo(FileInfo_t *file);
errno_t getVolumeDescriptor(VolDescriptor_t **vd, uint8_t *initOfVD);

int main(int argc, char **argv) {
    FileInfo_t *sourcefile = NULL;
    VolDescriptor_t *primary_vd = NULL;

    if (argc < 2) {
        fprintf(stderr, "Erro: Uso correto: %s <arquivo_origem>\n", argv[0]);
        return EINVAL;
    }

    loadFile(&sourcefile, argv[1]);

    FILE *destinefile = NULL;
    errno_t err = fopen_s(&destinefile, "destine.bin", "wb");
    if (err != 0) {
        freeFileInfo(sourcefile);
        return err;
    }

    fwrite(sourcefile->content, sizeof(uint8_t), sourcefile->readed, destinefile);

    // FILE *headerFile = NULL;
    // err = fopen_s(&headerFile, "header.bin", "wb");
    // if (err != 0) {
    //     freeFileInfo(sourcefile);
    //     return err;
    // }
    // fwrite(&(sourcefile->content[16 * SECTOR_LEN]), sizeof(uint8_t), 1 * SECTOR_LEN, headerFile);
    // fclose(headerFile);

    err = getVolumeDescriptor(&primary_vd, &(sourcefile->content[16 * SECTOR_LEN]));
    if (err != 0) {
        fprintf(stderr, "Erro ao ler Volume Descriptor: %d\n", err);
        fclose(destinefile);
        freeFileInfo(sourcefile);
        return err;
    }

    printf("Type: %d\nIdentifier: %s\nVersion: %d\n", primary_vd->type, primary_vd->identifier, primary_vd->version);

    fclose(destinefile);
    freeFileInfo(sourcefile);
    return EXIT_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

errno_t getVolumeDescriptor(VolDescriptor_t **vd, uint8_t *initOfVD) {
    errno_t err;

    if (initOfVD == NULL) {
        return EINVAL;
    }

    *vd = (VolDescriptor_t *)malloc(sizeof(VolDescriptor_t));
    if (*vd == NULL) {
        return ENOMEM;
    }

    (*vd)->type = initOfVD[0];
    memcpy_s((*vd)->identifier, sizeof((*vd)->identifier), &initOfVD[1], 5);
    (*vd)->identifier[5] = '\0';
    (*vd)->version = initOfVD[6];
    memcpy_s((*vd)->data, sizeof((*vd)->data), &initOfVD[7], sizeof((*vd)->data));
    return EXIT_SUCCESS;
}
