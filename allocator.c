#include "allocator.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef struct block_header {
    size_t size;
    int is_free;
    struct block_header *next;
    struct block_header *prev;
} block_header_t;

static block_header_t *heap_head = NULL;
static block_header_t *heap_tail = NULL;

static size_t align_size(size_t size) {
    const size_t alignment = sizeof(void *);
    return (size + alignment - 1) & ~(alignment - 1);
}

static block_header_t *find_free_block(size_t size) {
    block_header_t *current = heap_head;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static block_header_t *request_from_os(size_t size) {
    void *request = sbrk(0);
    if (sbrk((intptr_t)(sizeof(block_header_t) + size)) == (void *)-1) {
        return NULL;
    }

    block_header_t *block = (block_header_t *)request;
    block->size = size;
    block->is_free = 0;
    block->next = NULL;
    block->prev = heap_tail;

    if (heap_tail != NULL) {
        heap_tail->next = block;
    } else {
        heap_head = block;
    }
    heap_tail = block;
    return block;
}

static void split_block(block_header_t *block, size_t size) {
    size_t min_split_remainder = sizeof(block_header_t) + sizeof(void *);
    if (block->size < size + min_split_remainder) {
        return;
    }

    char *block_start = (char *)block;
    block_header_t *new_block =
        (block_header_t *)(block_start + sizeof(block_header_t) + size);

    new_block->size = block->size - size - sizeof(block_header_t);
    new_block->is_free = 1;
    new_block->next = block->next;
    new_block->prev = block;

    if (new_block->next != NULL) {
        new_block->next->prev = new_block;
    } else {
        heap_tail = new_block;
    }

    block->size = size;
    block->next = new_block;
}

static void coalesce_with_next(block_header_t *block) {
    block_header_t *next = block->next;
    if (next == NULL || !next->is_free) {
        return;
    }

    block->size += sizeof(block_header_t) + next->size;
    block->next = next->next;

    if (block->next != NULL) {
        block->next->prev = block;
    } else {
        heap_tail = block;
    }
}

static block_header_t *coalesce_neighbors(block_header_t *block) {
    if (block == NULL) {
        return NULL;
    }

    while (block->next != NULL && block->next->is_free) {
        coalesce_with_next(block);
    }

    while (block->prev != NULL && block->prev->is_free) {
        block = block->prev;
        while (block->next != NULL && block->next->is_free) {
            coalesce_with_next(block);
        }
    }

    return block;
}

void *my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t aligned_size = align_size(size);
    block_header_t *block = find_free_block(aligned_size);

    if (block != NULL) {
        block->is_free = 0;
        split_block(block, aligned_size);
    } else {
        block = request_from_os(aligned_size);
        if (block == NULL) {
            return NULL;
        }
    }

    return (void *)(block + 1);
}

void *my_calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }

    size_t total = nmemb * size;
    void *ptr = my_malloc(total);
    if (ptr == NULL) {
        return NULL;
    }

    memset(ptr, 0, total);
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return my_malloc(size);
    }

    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_header_t *block = ((block_header_t *)ptr) - 1;
    size_t aligned_size = align_size(size);

    if (block->size >= aligned_size) {
        split_block(block, aligned_size);
        return ptr;
    }

    void *new_ptr = my_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);
    return new_ptr;
}

void my_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    block_header_t *block = ((block_header_t *)ptr) - 1;
    block->is_free = 1;
    coalesce_neighbors(block);
}

allocator_stats_t my_allocator_get_stats(void) {
    allocator_stats_t stats = {0};
    block_header_t *current = heap_head;

    while (current != NULL) {
        stats.total_blocks++;
        if (current->is_free) {
            stats.free_blocks++;
            stats.free_bytes += current->size;
            if (current->size > stats.largest_free_block) {
                stats.largest_free_block = current->size;
            }
        } else {
            stats.allocated_blocks++;
            stats.allocated_bytes += current->size;
        }
        current = current->next;
    }

    if (stats.free_bytes > 0) {
        stats.external_fragmentation =
            1.0 - ((double)stats.largest_free_block / (double)stats.free_bytes);
    } else {
        stats.external_fragmentation = 0.0;
    }
    return stats;
}

void my_allocator_dump(void) {
    block_header_t *current = heap_head;
    size_t index = 0;

    printf("---- allocator dump ----\n");
    if (current == NULL) {
        printf("(empty heap)\n");
    }

    while (current != NULL) {
        void *user_ptr = (void *)(current + 1);
        printf(
            "[%zu] block=%p user=%p size=%zu free=%d prev=%p next=%p\n",
            index,
            (void *)current,
            user_ptr,
            current->size,
            current->is_free,
            (void *)current->prev,
            (void *)current->next
        );
        current = current->next;
        index++;
    }
    printf("------------------------\n");
}

void my_allocator_print_heap(void) {
    block_header_t *current = heap_head;
    size_t total_payload = 0;
    size_t index = 0;

    while (current != NULL) {
        total_payload += current->size;
        current = current->next;
    }

    printf("==== heap visualization ====\n");
    if (heap_head == NULL) {
        printf("(empty heap)\n");
        printf("============================\n");
        return;
    }

    current = heap_head;
    while (current != NULL) {
        size_t width = 40;
        size_t filled = width;
        if (total_payload > 0) {
            filled = (current->size * width) / total_payload;
            if (filled == 0) {
                filled = 1;
            }
            if (filled > width) {
                filled = width;
            }
        }

        char fill_char = current->is_free ? '-' : '#';
        printf("[%02zu] %s %6zuB |", index, current->is_free ? "FREE " : "USED ", current->size);
        for (size_t i = 0; i < width; i++) {
            putchar(i < filled ? fill_char : ' ');
        }
        printf("| user=%p\n", (void *)(current + 1));

        current = current->next;
        index++;
    }
    printf("total payload bytes: %zu\n", total_payload);
    printf("============================\n");
}
