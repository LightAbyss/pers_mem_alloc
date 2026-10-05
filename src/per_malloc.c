/* ****************************************************************************
 * HEADER INCLUSIONS                                                          *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "per_malloc.h"

/* ****************************************************************************
 * VARIABLES                                                                  *
 ******************************************************************************/
const uint8_t HEADER_MARKER = 0x55;
const uint8_t BLOCK_MARKER = 0x44;
const int PAGE_SIZE = 4096;
const int BLOCK_SIZE = sizeof(my_block);

static char *heap_start = NULL;
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

/* ****************************************************************************
 * LOCAL FUNCTION PROTOTYPES                                                  *
 ******************************************************************************/
my_stats *get_malloc_header();
my_block *find_last_block();
char *manage_heap(size_t size);
int compose_heap();
int join_if_possible(my_block *block, my_stats *malloc_header);
int free_page_if_possible(my_stats *malloc_header);
int *add_used_block(size_t size);

/* ****************************************************************************
 * EXPORTED FUNCTIONS                                                         *
 ******************************************************************************/
int *my_malloc(size_t size){
    if (size > UINT32_MAX || size == 0) {
        return NULL;
    }

    if(heap_start == NULL){
        heap_start = manage_heap(0);
        manage_heap(PAGE_SIZE);
        compose_heap();
    }
    
    return add_used_block(size);
}

int my_free(void *ptr){
    if(ptr == NULL){
        return -1;
    }
    // Get the malloc header
    my_stats *malloc_header = get_malloc_header();
    pthread_mutex_lock(&global_lock);

    my_block *block = (my_block *)((char *)ptr - sizeof(my_block));
    // Check if the block is valid
    if(block->marker != BLOCK_MARKER){
        pthread_mutex_unlock(&global_lock);
        return -1;
    }
    // Mark the block as free and clear its contents
    block->in_use = false;
    memset(ptr, 0, block->lenght);
    
    join_if_possible(block, malloc_header);
    pthread_mutex_unlock(&global_lock);
    return 0;
}

/* ****************************************************************************
 * LOCAL FUNCTIONS                                                            *
 ******************************************************************************/
my_stats *get_malloc_header(){
    assert(heap_start != NULL);
    my_stats *malloc_header = (my_stats *)heap_start;
    assert(malloc_header->marker == HEADER_MARKER);
    return malloc_header;
}

my_block *find_last_block(){
    my_block *block = (my_block *)((char *)heap_start + sizeof(my_stats));
    while(block->next != NULL){
        block = block->next;
    }
    return block;
}

char *manage_heap(size_t size){
    // Wrapper to access sbrk() syscall in a safety way
    const char *result = sbrk(size);
    if (result == (void *)-1) {
        return NULL; // sbrk failed
    }
    return result;
}

int compose_heap(){
    // Check if the heap has been initialized
    if((*heap_start) != HEADER_MARKER){
        const char *heap_end = manage_heap(0);
        assert(heap_end != (void *)-1);
        long int length = heap_end - heap_start;

        my_stats *malloc_header = (my_stats *)heap_start;
        pthread_mutex_lock(&global_lock);
        
        malloc_header->marker = HEADER_MARKER;
        malloc_header->total_blocks = 1;
        malloc_header->total_pages = 1;
        
        my_block *first_block = (my_block *)((char *)heap_start + sizeof(my_stats));
        first_block->marker = BLOCK_MARKER;
        first_block->in_use = false;
        first_block->lenght = length - sizeof(my_stats) - sizeof(my_block);
        first_block->next = NULL;
        first_block->prev = NULL;

        pthread_mutex_unlock(&global_lock);
    }
    
    return 0;
}

int join_if_possible(my_block *block, my_stats *malloc_header){
    // Join with the next, if possible
    if(block->next != NULL && (block->next)->in_use == false){
        my_block *next_block = (my_block *)block->next;
        next_block->marker = NULL;
        block->lenght += next_block->lenght + sizeof(my_block);
        block->next = next_block->next;
        if(block->next != NULL){
            (block->next)->prev = block;
        }
        // Clean up the merged block header to avoid dangling pointers and potential misuse
        memset(next_block, 0, sizeof(my_block));
        malloc_header->total_blocks--;
    }
    // Join with the previous, if possible
    if(block->prev != NULL && (block->prev)->in_use == false){
        my_block *prev_block = (my_block *)block->prev;
        block->marker = NULL;
        prev_block->lenght += block->lenght + sizeof(my_block);
        prev_block->next = block->next;
        if(prev_block->next != NULL){
            (prev_block->next)->prev = prev_block;
        }
        // Clean up the merged block header to avoid dangling pointers and potential misuse
        memset(block, 0, sizeof(my_block));
        malloc_header->total_blocks--;
    }
    if(malloc_header->total_pages > 1){
        free_page_if_possible(malloc_header);
    }

    return 0;
}

int free_page_if_possible(my_stats *malloc_header){
    my_block *last_block = find_last_block();
    if(last_block->lenght > PAGE_SIZE && last_block->in_use == false){
        manage_heap(-PAGE_SIZE);
        last_block->lenght -= PAGE_SIZE;
    }
    return 0;
}

int *add_used_block(size_t size){
    // Get the malloc header
    my_stats *malloc_header = get_malloc_header();
    pthread_mutex_lock(&global_lock);

    my_block *block = (my_block *)((char *)malloc_header + sizeof(my_stats));
    my_block *smallest_block = NULL;
    my_block *last_block = find_last_block();

    // best fit algorithm
    while(block != NULL){
        assert(block->marker == BLOCK_MARKER);
        if(block->lenght >= size && block->in_use == false){
            if(smallest_block == NULL || smallest_block->lenght > block->lenght){
                smallest_block = block;
            }
        }

        //last_block = block;
        block = block->next;
    }

    // No block big engough was found
    if(smallest_block == NULL){
        while(last_block->lenght < size + sizeof(my_block) + 1){
            manage_heap(PAGE_SIZE);
            last_block->lenght += PAGE_SIZE;
            malloc_header->total_pages++;
        }
        smallest_block = last_block;
    }

    // Found a block big enough
    smallest_block->in_use = true;
    // Divide the block if it is big enough to hold the requested size and a new block header
    // If not, check if its the last block, so can extend the heap, otherwise return the block as is
    if(smallest_block->lenght < size + sizeof(my_block) + 1){
        if(smallest_block != last_block){
            pthread_mutex_unlock(&global_lock);
            return (int *)((char *)smallest_block + sizeof(my_block));
        }else{
            manage_heap(PAGE_SIZE);
            malloc_header->total_pages++;
            smallest_block->lenght += PAGE_SIZE;
        }
    }

    size_t remaining_size = smallest_block->lenght - size - sizeof(my_block);
    malloc_header->total_blocks++;
    my_block *new_block = (my_block *)((char *)smallest_block + sizeof(my_block) + size);
    new_block->marker = BLOCK_MARKER;
    new_block->prev = smallest_block;
    new_block->next = smallest_block->next;
    if (new_block->next != NULL) {
        (new_block->next)->prev = new_block;
    }
    smallest_block->next = new_block;
    new_block->lenght = remaining_size;
    smallest_block->lenght = size;
    
    pthread_mutex_unlock(&global_lock);
    return (int *)((char *)smallest_block + sizeof(my_block));
}