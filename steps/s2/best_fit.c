#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

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

my_stats *get_malloc_header(){
    assert(heap_start != NULL);
    my_stats *malloc_header = (my_stats *)heap_start;
    assert(malloc_header->marker == HEADER_MARKER);
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

int start_malloc(){
    if(heap_start == NULL){
        heap_start = sbrk(0);
        sbrk(PAGE_SIZE);
    }
    char *heap_end = sbrk(0);
    long int length = heap_end - heap_start;
    // Check if the heap has been initialized
    if((*heap_start) != HEADER_MARKER){
        *(heap_start) = HEADER_MARKER;
        my_stats *malloc_header = (my_stats *)heap_start;
        malloc_header->total_blocks = 1;
        malloc_header->total_pages = 1;
        
        my_block *first_block = (my_block *)((char *)heap_start + sizeof(my_stats));
        first_block->marker = BLOCK_MARKER;
        first_block->in_use = false;
        first_block->lenght = length - sizeof(my_stats) - sizeof(my_block);
        first_block->next = NULL;
        first_block->prev = NULL;
    }
    return 1;
}

int main(){
    if (!start_malloc()) {
        return 1;
    }

    size_t size = sizeof(int) * 10; // Example size for allocation

    // Get the malloc header
    my_stats *malloc_header = get_malloc_header();
    while(malloc_header->simple_lock){
        // Wait until the lock is released
        sleep(1);
    };
    malloc_header->simple_lock = true;
    printf("Lock acquired\n");

    my_block *block = (my_block *)((char *)heap_start + sizeof(my_stats));
    my_block *smallest_block = NULL;
    my_block *last_block = block;
    printf("Aquire first block. Addrs: %p\n", block);
    printf("First block points to next: %p\n", block->next);
    printf("First block points to prev: %p\n", block->prev);

    // best fit algorithm
    int cycle_count = 0;
    printf("Starting best fit search for size: %zu\n", size);
    while(block != NULL){
        printf("Cycle count: %d\n", cycle_count++);
        assert(block->marker == BLOCK_MARKER);
        if((block->lenght + sizeof(my_block)) >= size && block->in_use == false){
            if(smallest_block == NULL || smallest_block->lenght > block->lenght){
                smallest_block = block;
            }
        }
        printf("Current block: %p, Size: %zu\n", block, block->lenght);
        if(smallest_block != NULL){
            printf("Smallest block: %p, Size: %zu\n", smallest_block, smallest_block->lenght);
        }
        last_block = block;
        block = block->next;
    }

    printf("Found a block big enough: %p, Size: %zu\n", smallest_block, smallest_block->lenght);
    // Found a block big enough
    smallest_block->in_use = true;

    // Create a new free block
    /* First, check if we have enough space to create a new block 
     * We need AT LEAST sizeof(my_block) + 1 bytes to create a new block.
    */
    int check_remaining_space = smallest_block->lenght - size - sizeof(my_block) - 1;
    if (check_remaining_space <= 0) {
        printf("Not enough space to create a new block. Should expand the heap.\n");
    }
    
    int remaining_size = check_remaining_space + 1;
    printf("Remaining space after allocation: %d\n", remaining_size);
    
    printf("Creating a new block\n");
    malloc_header->total_blocks++;
    my_block *new_block = (my_block *)((char *)smallest_block + sizeof(my_block) + size);
    new_block->marker = BLOCK_MARKER;
    new_block->prev = smallest_block;
    new_block->next = smallest_block->next;
    if (new_block->next != NULL) {
        (new_block->next)->prev = new_block;
    }
    new_block->lenght = remaining_size;
    new_block->in_use = false;
    printf("New block created at: %p, Size: %zu\n", new_block, new_block->lenght);
    printf("New block points to next: %p\n", new_block->next);
    printf("New block points to prev: %p\n", new_block->prev);

    printf("Updating smallest block");
    smallest_block->lenght = size;
    smallest_block->in_use = true;
    smallest_block->next = new_block;
    malloc_header->simple_lock = false;
    printf("Smallest block updated at: %p, Size: %zu\n", smallest_block, smallest_block->lenght);
    printf("Smallest block points to next: %p\n", smallest_block->next);
    printf("Smallest block points to prev: %p\n", smallest_block->prev);

    printf("Lock released\n");

    return 0;
}