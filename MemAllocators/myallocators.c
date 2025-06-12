#include <unistd.h>
#include <stdef.h>
#include <stdint.h>
#include <string.h>

typedef struct block_meta {
    size_t size;
    struct bloc_meta *next;
    int free;
} block_meta;

#define META_SIZE sizeof(block_meta)
static block_meta *free_list = NULL;

#define ALIGN(size) (((size) + 7) & ~7)
#define PAGE_ALIGN(size) (((size) + 4095) & ~4095)

void *mymalloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    size = ALIGN(size);

    block_meta *current = free_list, *prev = NULL;
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size > size + META_SIZE + 8) {
                block_meta *new_block = (block_meta *)((char *)current + META_SIZE +size);
                new_block->size = current->size - size - META_SIZE;
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }
            current->free = 0;
            return (void*)(current + 1);
        }
        prev = current;
        current = current->next;
    }

    // No suitable block found, allocate a new one
    size_t total_size = PAGE_ALIGN(size + META_SIZE);
    block_meta *new_block = (block_meta *)sbrk(total_size);
    if (new_block == (void *)-1) {
        return NULL; // Allocation failed
    }

    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;

    if (prev) {
        prev->next = new_block;
    } else {
        free_list = new+block;
    }

    return (void *)(new_block + 1);
}

void *myfree(void *ptr) {
    if (!ptr) {
        return;
    }

    block_meta *block = (block_meta *)ptr -1;
    block->free = 1;

    if (block->next && block->next->free) {
        block->size += block->next->size + META_SIZE;
        block->next = block->next->next;
    }
}

void *myrealloc(void *ptr, size_t size) {
    if (!ptr) {
        return mymalloc(size);
    }

    if (size <= 0) {
        myfree(ptr);
        return NULL;
    }

    block_meta *block = (block_meta *)ptr - 1;

    if (block->size >= size) {
        return ptr; // No need to reallocate
    }

    void *new_ptr = mymalloc(size);
    if (!new_ptr) {
        return NULL; // Allocation failed
    }

    memcpy(new_ptr, ptr, block->size);
    myfree(ptr);

    return new_ptr;
}
