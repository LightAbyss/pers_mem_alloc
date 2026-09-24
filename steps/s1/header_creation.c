#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

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

const uint8_t HEADER_MARKER = 0x55;
const uint8_t BLOCK_MARKER = 0x44;
const int PAGE_SIZE = 4096;
const int BLOCK_SIZE = sizeof(my_block);

static char *heap_start = NULL;

int main() {
    heap_start = sbrk(0);
    sbrk(PAGE_SIZE);
    
    char *heap_end = sbrk(0);
    long int length = heap_end - heap_start;
    
    *(heap_start) = HEADER_MARKER;

    printf("Addr inicial: %p\n", heap_start);
    printf("Addr final: %p\n", heap_end);
    printf("Label heap_start: %d\n", (*heap_start));
    printf("Heap inicial size: %ld\n", length);
    
    // Creamos el header
    my_stats *malloc_header = (my_stats *)heap_start;
    malloc_header->total_blocks = 1;
    malloc_header->total_pages = 1;
    // Creamos el primer bloque
    my_block *first_block = (my_block *)((char *)heap_start + sizeof(my_stats));
    first_block->marker = BLOCK_MARKER;
    first_block->in_use = false;
    first_block->lenght = length - sizeof(my_stats) - sizeof(my_block);
    first_block->next = NULL;
    first_block->prev = NULL;

    printf("Label heap_start: %d\n", (*heap_start));
    printf("Header label: %d\n", malloc_header->marker);
    printf("Header lock: %d\n", malloc_header->simple_lock);
    printf("Header pages: %d\t Header blocks: %d\n", malloc_header->total_pages, malloc_header->total_blocks);
    printf("First block size: %d\n", first_block->lenght);
    printf("First block marker: %d\n", first_block->marker);
  
  return 0;
}