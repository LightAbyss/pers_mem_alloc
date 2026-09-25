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

struct stats {
    uint8_t marker;
    bool simple_lock;
    uint32_t total_blocks;
    uint32_t total_pages;
};

typedef struct stats my_stats;
typedef struct block_x my_block;

int *my_malloc(size_t size);
int my_free(void *ptr);


#endif // PER_MALLOC_H