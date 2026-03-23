#include "allocator.h"

#include <stdio.h>
#include <string.h>

enum { MAX_SLOTS = 32 };

typedef struct {
    void *ptr;
    size_t size;
    int in_use;
} allocation_slot_t;

static void print_help(void) {
    printf("Commands:\n");
    printf("  malloc <slot> <bytes>   allocate memory in slot\n");
    printf("  calloc <slot> <n> <sz>  allocate n items of size sz\n");
    printf("  realloc <slot> <bytes>  resize allocation in slot\n");
    printf("  free <slot>             free allocation in slot\n");
    printf("  write <slot> <text>     copy text into slot memory\n");
    printf("  read <slot>             print slot bytes as string\n");
    printf("  list                    list active slot allocations\n");
    printf("  dump                    print allocator internal blocks\n");
    printf("  heap                    visualize heap block layout\n");
    printf("  stats                   print fragmentation statistics\n");
    printf("  help                    show this help message\n");
    printf("  quit                    exit the test interface\n");
}

static int parse_slot(int slot) {
    return slot >= 0 && slot < MAX_SLOTS;
}

static void list_slots(const allocation_slot_t *slots) {
    printf("---- slots ----\n");
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (slots[i].in_use) {
            printf("[%d] ptr=%p size=%zu\n", i, slots[i].ptr, slots[i].size);
        }
    }
    printf("--------------\n");
}

static void print_stats(void) {
    allocator_stats_t stats = my_allocator_get_stats();
    printf("---- allocator stats ----\n");
    printf("total_blocks:            %zu\n", stats.total_blocks);
    printf("allocated_blocks:        %zu\n", stats.allocated_blocks);
    printf("free_blocks:             %zu\n", stats.free_blocks);
    printf("allocated_bytes:         %zu\n", stats.allocated_bytes);
    printf("free_bytes:              %zu\n", stats.free_bytes);
    printf("largest_free_block:      %zu\n", stats.largest_free_block);
    printf("external_fragmentation:  %.2f%%\n", stats.external_fragmentation * 100.0);
    printf("-------------------------\n");
}

int main(void) {
    allocation_slot_t slots[MAX_SLOTS] = {0};
    char line[512];

    printf("Allocator test interface\n");
    print_help();

    while (1) {
        char command[32];
        int slot = -1;
        size_t size = 0;
        size_t nmemb = 0;

        printf("\nallocator> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        if (sscanf(line, "%31s", command) != 1) {
            continue;
        }

        if (strcmp(command, "malloc") == 0 || strcmp(command, "alloc") == 0) {
            if (sscanf(line, "%*s %d %zu", &slot, &size) != 2) {
                printf("usage: malloc <slot> <bytes>\n");
                continue;
            }
            if (!parse_slot(slot)) {
                printf("slot must be 0..%d\n", MAX_SLOTS - 1);
                continue;
            }
            if (slots[slot].in_use) {
                printf("slot %d already in use; free it first\n", slot);
                continue;
            }

            slots[slot].ptr = my_malloc(size);
            if (slots[slot].ptr == NULL) {
                printf("allocation failed\n");
                continue;
            }
            slots[slot].size = size;
            slots[slot].in_use = 1;
            printf("allocated %zu bytes in slot %d at %p\n", size, slot, slots[slot].ptr);
            continue;
        }

        if (strcmp(command, "calloc") == 0) {
            if (sscanf(line, "%*s %d %zu %zu", &slot, &nmemb, &size) != 3) {
                printf("usage: calloc <slot> <n> <sz>\n");
                continue;
            }
            if (!parse_slot(slot)) {
                printf("slot must be 0..%d\n", MAX_SLOTS - 1);
                continue;
            }
            if (slots[slot].in_use) {
                printf("slot %d already in use; free it first\n", slot);
                continue;
            }

            slots[slot].ptr = my_calloc(nmemb, size);
            if (slots[slot].ptr == NULL) {
                printf("allocation failed\n");
                continue;
            }
            slots[slot].size = nmemb * size;
            slots[slot].in_use = 1;
            printf(
                "calloc allocated %zu bytes in slot %d at %p\n",
                slots[slot].size,
                slot,
                slots[slot].ptr
            );
            continue;
        }

        if (strcmp(command, "realloc") == 0) {
            if (sscanf(line, "%*s %d %zu", &slot, &size) != 2) {
                printf("usage: realloc <slot> <bytes>\n");
                continue;
            }
            if (!parse_slot(slot) || !slots[slot].in_use) {
                printf("invalid or unused slot\n");
                continue;
            }

            void *new_ptr = my_realloc(slots[slot].ptr, size);
            if (new_ptr == NULL && size != 0) {
                printf("realloc failed; original allocation unchanged\n");
                continue;
            }

            slots[slot].ptr = new_ptr;
            slots[slot].size = size;
            slots[slot].in_use = (size != 0);
            if (size == 0) {
                printf("realloc to 0 bytes freed slot %d\n", slot);
            } else {
                printf("reallocated slot %d to %zu bytes at %p\n", slot, size, new_ptr);
            }
            continue;
        }

        if (strcmp(command, "free") == 0) {
            if (sscanf(line, "%*s %d", &slot) != 1) {
                printf("usage: free <slot>\n");
                continue;
            }
            if (!parse_slot(slot) || !slots[slot].in_use) {
                printf("invalid or unused slot\n");
                continue;
            }

            my_free(slots[slot].ptr);
            slots[slot].ptr = NULL;
            slots[slot].size = 0;
            slots[slot].in_use = 0;
            printf("freed slot %d\n", slot);
            continue;
        }

        if (strcmp(command, "write") == 0) {
            char text[400];
            if (sscanf(line, "%*s %d %399[^\n]", &slot, text) != 2) {
                printf("usage: write <slot> <text>\n");
                continue;
            }
            if (!parse_slot(slot) || !slots[slot].in_use) {
                printf("invalid or unused slot\n");
                continue;
            }

            size_t max_copy = slots[slot].size > 0 ? slots[slot].size - 1 : 0;
            size_t copy_len = strlen(text);
            if (copy_len > max_copy) {
                copy_len = max_copy;
            }
            memcpy(slots[slot].ptr, text, copy_len);
            ((char *)slots[slot].ptr)[copy_len] = '\0';
            printf("wrote %zu bytes to slot %d\n", copy_len, slot);
            continue;
        }

        if (strcmp(command, "read") == 0) {
            if (sscanf(line, "%*s %d", &slot) != 1) {
                printf("usage: read <slot>\n");
                continue;
            }
            if (!parse_slot(slot) || !slots[slot].in_use) {
                printf("invalid or unused slot\n");
                continue;
            }
            printf("slot %d: %s\n", slot, (char *)slots[slot].ptr);
            continue;
        }

        if (strcmp(command, "list") == 0) {
            list_slots(slots);
            continue;
        }

        if (strcmp(command, "dump") == 0) {
            my_allocator_dump();
            continue;
        }

        if (strcmp(command, "heap") == 0) {
            my_allocator_print_heap();
            continue;
        }

        if (strcmp(command, "stats") == 0) {
            print_stats();
            continue;
        }

        if (strcmp(command, "help") == 0) {
            print_help();
            continue;
        }

        if (strcmp(command, "quit") == 0) {
            break;
        }

        printf("unknown command: %s\n", command);
        print_help();
    }

    for (int i = 0; i < MAX_SLOTS; i++) {
        if (slots[i].in_use) {
            my_free(slots[i].ptr);
        }
    }
    return 0;
}
