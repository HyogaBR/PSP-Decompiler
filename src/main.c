#include <corecrt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h> // CORREÇÃO 1: Necessário para usar 'true' sem dar erro de compilação

#include "cursor.h"
#include "headers.h"

typedef struct FileInfo {
    size_t    readed;
    FILE       *fptr;
    size_t       len;
    char       *path;
    uint8_t *content;
} FileInfo_t;

typedef struct Node {
    VolDescriptor_t *vd;
    struct Node *next;
} Node_t;

void addNode(Node_t **head, VolDescriptor_t *value) {
    Node_t *new = (Node_t *)malloc(sizeof(Node_t));
    if (new == NULL) {
        printf("Erro criando nó");
        return;
    }
    new->vd = value;
    new->next = *head;
    *head = new;
}

void executeAtAll(Node_t* head, void (*callback)(VolDescriptor_t *)) {
    Node_t* actual = head;
    while (actual != NULL) {
        callback(actual->vd);
        actual = actual->next;
    }
}

void liberarLista(Node_t** head) {
    Node_t* actual = *head;
    Node_t* temp;

    while (actual != NULL) {
        temp = actual;
        actual = actual->next;
        free(temp->vd); // CORREÇÃO 4: Libera a memória de cada descriptor alocado no heap
        free(temp);
    }

    *head = NULL;
}

void printElement(VolDescriptor_t *vd) {
    printf("type: %d\n", vd->header.type);
}

errno_t loadFile(FileInfo_t **file, char *path);
void freeFileInfo(FileInfo_t *file);

int main(int argc, char **argv) {
    errno_t err = 0;
    FileInfo_t *sourcefile = NULL;
    Node_t *VDlsit = NULL;

    if (argc < 2) {
        fprintf(stderr, "Erro: Uso correto: %s <arquivo_origem>\n", argv[0]);
        return EINVAL;
    }

    loadFile(&sourcefile, argv[1]);

    ByteCursor_t *cursor = cursor_init(sourcefile->content, sourcefile->readed);

    cursor_skip(cursor, 16 * SECTOR_LEN);

    while (true) {
        // CORREÇÃO 2: Aloca dinamicamente o descriptor no HEAP para que ele sobreviva fora do loop
        VolDescriptor_t *temp = (VolDescriptor_t *)malloc(sizeof(VolDescriptor_t));
        if (temp == NULL) {
            printf("Erro de alocação no loop");
            break;
        }

        getVolumeDescriptor(temp, cursor); // CORREÇÃO 3: Passa 'temp' diretamente (já é um ponteiro)

        addNode(&VDlsit, temp);

        if (temp->header.type == DESCRIPTOR_SET_TERMINATOR) {
            break;
        }
    }

    executeAtAll(VDlsit, printElement);

    // CORREÇÃO 5: Adicionada a chamada para evitar vazamento de memória massivo
    liberarLista(&VDlsit);

    cursor_close(cursor);
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
