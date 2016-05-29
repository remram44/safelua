#ifndef ALLOC_H
#define ALLOC_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"

struct Block {
    void *ptr;
    size_t size;
};

struct Allocator {
    struct Block *blocks;
    size_t nb_blocks;
    size_t size_blocks;
    size_t total_allocated;
};

struct Allocator *new_allocator(void);
void delete_allocator(struct Allocator *alloc);

void *l_simple_alloc(void *ud, void *ptr, size_t osize, size_t nsize);
void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize);

#endif
