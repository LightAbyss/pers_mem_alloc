#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

struct block_x {
    uint8_t marker;
    struct free_area *prev;
    bool in_use;
    uint32_t lenght;
    struct free_area *next;
};

struct stats {
    int magic_bytes;
    bool simple_lock;
    uint32_t total_blocks;
    uint32_t total_pages;
};

typedef struct stats my_stats;
typedef struct block_x my_block;

const int SAFE_GUARD = 0x55;
const int HEADER_MARKER = 0x44;
const int PAGE_SIZE = 4096;
const int BLOCK_SIZE = sizeof(my_block);

char *heap_start = NULL;

my_stats *get_malloc_header(){
    assert(heap_start != NULL);
    my_stats *malloc_header = (my_stats *)heap_start;
    assert(malloc_header->magic_bytes == SAFE_GUARD);
    return malloc_header;
}

my_block *find_last_block(){
    my_stats *malloc_header = get_malloc_header();
    my_block *block = (my_block *)((char *)heap_start + sizeof(my_stats));
    while(block->next != NULL){
        block = block->next;
    }
    return block;
}

int *my_malloc(size_t size){
    if(heap_start == NULL){
        heap_start = sbrk(0);
        sbrk(PAGE_SIZE);
    }
    char *heap_end = sbrk(0);
    long int length = heap_end - heap_start;
    // Check if the heap has been initialized
    if((*heap_start) != SAFE_GUARD){
        *(heap_start) = SAFE_GUARD;
        my_stats *malloc_header = (my_stats *)heap_start;
        malloc_header->total_blocks = 1;
        malloc_header->total_pages = 1;
        
        my_block *first_block = (my_block *)((char *)heap_start + sizeof(my_stats));
        first_block->marker = HEADER_MARKER;
        first_block->in_use = false;
        first_block->lenght = length - sizeof(my_stats) - sizeof(my_block);
        first_block->next = NULL;
        first_block->prev = NULL;
    }
    return add_used_block(size);
}

int *add_used_block(size_t size){
    // Get the malloc header
    my_stats *malloc_header = get_malloc_header();
    while(malloc_header->simple_lock){
        // Wait until the lock is released
        sleep(1);
    };
    malloc_header->simple_lock = true;

    my_block *block = (my_block *)((char *)heap_start + sizeof(my_stats));
    my_block *smallest_block = NULL;
    my_block *last_block = block;

    // best fit algorithm
    while(block != NULL){
        assert(block->marker == HEADER_MARKER);
        if((block->lenght + sizeof(my_block)) >= size && block->in_use == false){
            if(smallest_block == NULL || smallest_block->lenght > block->lenght){
                smallest_block = block;
            }
        }

        last_block = block;
        block = block->next;
    }

    // No block big engough was found
    if(smallest_block == NULL){
        my_block *last_block = find_last_block();
        while(last_block->lenght < size){
            sbrk(PAGE_SIZE);
            last_block->lenght += PAGE_SIZE;
            malloc_header->total_pages++;
        }
        smallest_block = last_block;
    }

    // Found a block big enough
    smallest_block->in_use = true;
    // Create a new block, so list always has a free block at the end
    int must_have_new_block = smallest_block->lenght - size - sizeof(my_block) - 1;
    if(must_have_new_block <= 0){
        sbrk(PAGE_SIZE);
        malloc_header->total_pages++;
        last_block->lenght += PAGE_SIZE;
        must_have_new_block = smallest_block->lenght - size - sizeof(my_block) - 1;
    }
    int remaining_size = must_have_new_block + 1;
    malloc_header->total_blocks++;
    my_block *new_block = (my_block *)((char *)smallest_block + sizeof(my_block) + size);
    new_block->marker = HEADER_MARKER;
    new_block->prev = smallest_block;
    new_block->next = smallest_block->next;
    if (new_block->next != NULL) {
        (new_block->next)->prev = new_block;
    }
    smallest_block->next = new_block;
    new_block->lenght = remaining_size;
    smallest_block->lenght = size;
    malloc_header->simple_lock = false;
    return (int *)((char *)smallest_block + sizeof(my_block));
}