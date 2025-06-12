#include <unistd.h>
#include <stdio.h>
#include <stdef.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

typedef struct block_meta {
    size_t size;
    struct bloc_meta *next;
    int free;
    pthread_mutex_t lock; // Mutex for thread safety
} block_meta;

#define META_SIZE sizeof(block_meta)
static block_meta *free_list = NULL;

static pthread_mutex_t free_list_mutex = PTHREAD_MUTEX_INITIALIZER;

#define ALIGN(size) (((size) + 7) & ~7)
#define PAGE_ALIGN(size) (((size) + 4095) & ~4095)

static void init_block_lock(block_meta *block) {
    pthread_mutexattr_t = attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&block->lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

void *MtSafeMalloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    size = ALIGN(size);
    block_meta *best_fit = NULL;
    block_meta *prev = NULL;
    block_meta *current = NULL;

    pthread_mutex_lock(&free_list_mutex);
    current = free_list;

    while (current) {
        if (current->free && current->size >= size) {
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
                if (best_fit->size == size) break; // Exact fit found
            }
        }
        current = current->next;
    }

    if (best_fit) {
        pthread_mutex_lock(&best_fit->block_lock);

        if (best_fit->free) {
            if (best_fit->size > size + META_SIZE + 8) {
                block_meta *new_block = (block_meta *)((char *)best_fit + META_SIZE + size);
                new_block->size = best_fit->size - size - META_SIZE;
                new_block->free = 1;
                init_block_lock(new_block);
                
                pthread_mutex_lock(&free_list_mutex);
                new_block->next = best_fit->next;
                best_fit->next = new_block;
                pthread_mutex_unlock(&free_list_mutex);

                best_fit->size = size;
            }
            best_fit->free = 0;
            pthread_mutex_unlock(&best_fit->block_lock);
            pthread_mutex_unlock(&free_list_mutex);
            return (void *)(best_fit + 1);
        }
        pthread_mutex_unlock(&best_fit->block_lock);
    }
    pthread_mutex_unlock(&free_list_mutex);

    // No suitable block found, allocate a new one
    size_t total_size = PAGE_ALIGN(size + META_SIZE);
    block_meta *new_block = (block_meta *)sbrk(total_size);
    if (new_block == (void *)-1) {
        return NULL; // Allocation failed
    }

    new_block->size = size;
    new_block->free = 0;
    init_block_lock(new_block);
    
    pthread_mutex_lock(&free_list_mutex);
    new_block->next = free_list;
    free_list = new_block;
    pthread_mutex_unlock(&free_list_mutex);

    return (void *)(new_block + 1);
}

void *MtSafeFree(void *ptr) {
    if (!ptr) {
        return;
    }

    block_meta *block = (block_meta *)ptr -1;
    pthread_mutex_lock(&block->lock);
    block->free = 1;

    if (block->next) {
        if (pthread_mutex_trylock(&block->next->block_lock) == 0) {
            if (block->next->free) {
                block->size += META_SIZE + block->next->size;

                pthread_mutex_lock(&free_list_mutex);
                block->next = block->next->next;
                pthread_mutex_unlock(&free_list_mutex);
            }
            pthread_mutex_unlock(&block->next->block_lock);
        }
    }
    pthread_mutex_unlock(&block->block_lock);
}

void *MtSafeRealloc(void *ptr, size_t size) {
    if (!ptr) {
        return MtSafeMalloc(size);
    }

    if (size <= 0) {
        MtSafeFree(ptr);
        return NULL;
    }

    size = ALIGN(size);
    block_meta *block = (block_meta *)ptr - 1;

    pthread_mutex_lock(&block->block_lock);

    if (block->size >= size) {
        if (block->size > size + META_SIZE + 8) {
            block_meta *new_block = (block_meta *)((char *)block + META_SIZE + size);
            new_block->size = block->size - size - META_SIZE;
            new_block->free = 1;
            init_block_lock(new_block);

            pthread_mutex_lock(&free_list_mutex);
            new_block->next = block->next;
            block->next = new_block;
            pthread_mutex_unlock(&free_list_mutex);
            block->size = size;
        }
        pthread_mutex_unlock(&block->block_lock);
        return ptr; // No need to reallocate
    }

    pthread_mutex_unlock(&block->block_lock);
    // Need to reallocate
    void *new_ptr = MtSafemalloc(size);
    if (!new_ptr) {
        return NULL; // Allocation failed
    }

    pthread_mutex_lock(&block->block_lock);
    memcpy(new_ptr, ptr, block->size);
    pthread_mutex_unlock(&block->block_lock);

    MtSafeFree(ptr);

    return new_ptr;
}
