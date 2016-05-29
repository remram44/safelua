#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"

void *l_simple_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void)ud; (void)osize;  /* not used */
    if(nsize == 0)
    {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}

struct Allocator *new_allocator(void)
{
    struct Allocator *alloc = malloc(sizeof(struct Allocator));
    alloc->blocks = malloc(sizeof(struct Block) * 4);
    alloc->nb_blocks = 0;
    alloc->size_blocks = 4;
    alloc->total_allocated = 0;
    return alloc;
}

void delete_allocator(struct Allocator *alloc)
{
    size_t blk;
    for(blk = 0; blk < alloc->nb_blocks; ++blk)
        free(alloc->blocks[blk].ptr);
    free(alloc->blocks);
    free(alloc);
}

void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    struct Allocator *alloc = ud;
    size_t blk;
    if(ptr == NULL)
    {
        blk = alloc->nb_blocks;
        alloc->nb_blocks++;
        if(alloc->nb_blocks > alloc->size_blocks)
        {
            alloc->size_blocks *= 2;
            alloc->blocks = realloc(alloc->blocks,
                                    sizeof(struct Block) * alloc->size_blocks);
        }
        alloc->blocks[blk].ptr = NULL;
        alloc->blocks[blk].size = 0;
    }
    else
    {
        /* Bisect to the block */
        size_t low = 0, high = alloc->nb_blocks;
        while(low < high)
        {
            size_t mid = (low + high) / 2;
            if(alloc->blocks[mid].ptr < ptr)
                low = mid + 1;
            else
                high = mid;
        }
        blk = low;
    }
    assert(alloc->blocks[blk].ptr == ptr &&
           (!ptr || alloc->blocks[blk].size == osize) );
    if(nsize == 0)
    {
        free(ptr);
        alloc->total_allocated -= osize;
        memmove(&alloc->blocks[blk], &alloc->blocks[blk + 1],
                sizeof(struct Block) * (alloc->nb_blocks - blk - 1));
        alloc->nb_blocks--;
        return NULL;
    }
    else
    {
        ptr = realloc(ptr, nsize);
        alloc->blocks[blk].ptr = ptr;
        alloc->total_allocated += nsize - alloc->blocks[blk].size;
        alloc->blocks[blk].size = nsize;
        while(blk > 0 &&
              alloc->blocks[blk].ptr < alloc->blocks[blk - 1].ptr)
        {
            struct Block tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk - 1];
            alloc->blocks[blk - 1] = tmp;
            blk--;
        }
        while(blk + 1 < alloc->nb_blocks &&
              alloc->blocks[blk].ptr > alloc->blocks[blk + 1].ptr)
        {
            struct Block tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk + 1];
            alloc->blocks[blk + 1] = tmp;
            blk++;
        }
        return ptr;
    }
}
