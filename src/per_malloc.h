#ifndef PER_MALLOC_H
#define PER_MALLOC_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

struct block_x {
    uint8_t marker;
    struct block_x *prev;
    bool in_use;
    uint32_t lenght;
    struct block_x *next;
};

typedef struct{
    uint8_t marker;
    uint8_t padding[7]; // Padding to align the structure to 8 bytes
    uint32_t total_blocks;
    uint32_t total_pages;
} my_stats;

typedef struct block_x my_block;

int *my_malloc(size_t size);
int my_free(void *ptr);


#endif // PER_MALLOC_H