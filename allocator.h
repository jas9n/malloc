#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct allocator_stats {
    size_t total_blocks;
    size_t allocated_blocks;
    size_t free_blocks;
    size_t allocated_bytes;
    size_t free_bytes;
    size_t largest_free_block;
    double external_fragmentation;
} allocator_stats_t;

void *my_malloc(size_t size);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);
void my_free(void *ptr);
allocator_stats_t my_allocator_get_stats(void);
void my_allocator_dump(void);
void my_allocator_print_heap(void);

#endif
