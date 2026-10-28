#include <corecrt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    size_t    readed;
    FILE       *fptr;
    size_t       len;
    char       *path;
    uint8_t *content;
} FileInfo;

errno_t loadFile(FileInfo **file, char *path);
void freeFileInfo(FileInfo *file);

int main(int argc, char **argv) {
    FileInfo *sourcefile = NULL;
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

    FILE *headerFile = NULL;
    err = fopen_s(&headerFile, "header.bin", "wb");
    if (err != 0) {
        freeFileInfo(sourcefile);
        return err;
    }
    fwrite(sourcefile->content, sizeof(uint8_t), 32 * 1024, headerFile);
    fclose(headerFile);

    fclose(destinefile);
    freeFileInfo(sourcefile);
    return EXIT_SUCCESS;
}

errno_t loadFile(FileInfo **file, char *path) {
    errno_t err;
    if (path == NULL) {
        return EINVAL;
    }

    *file = (FileInfo *)malloc(sizeof(FileInfo));
    if (file == NULL) {
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

void freeFileInfo(FileInfo *file) {
    free(file->content);
    fclose(file->fptr);
    free(file->path);
    free(file);
}
