#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct VolDescriptor {
    uint8_t type;
    uint8_t identifier[6];
    uint8_t version;
    uint8_t data[2041];
} VolDescriptor_t;

typedef struct FileInfo {
    size_t    readed;
    FILE       *fptr;
    size_t       len;
    char       *path;
    uint8_t *content;
} FileInfo_t;

enum TypeVolDescriptor {
    BOOTRECORD = 0,
    PRIMARYVD = 1,
    SUPPLEMENTARYVD = 2,
    PARTITIONVD = 3,
    VDSETTERMINATOR = 255
};
