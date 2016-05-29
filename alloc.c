#include <assert.h>
#include <stdio.h>
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
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Created allocator %p\n", alloc);
#endif
    return alloc;
}

void delete_allocator(struct Allocator *alloc)
{
    size_t blk;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Deleting allocator %p\n", alloc);
#endif
    for(blk = 0; blk < alloc->nb_blocks; ++blk)
        free(alloc->blocks[blk].ptr);
    free(alloc->blocks);
    free(alloc);
}

void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    struct Allocator *alloc = ud;
    size_t blk;
#ifdef _DEBUG_ALLOC
    {
        size_t i;
        for(i = 0; i < alloc->nb_blocks; ++i)
        {
            fprintf(stderr, "%p %u ",
                    alloc->blocks[i].ptr, alloc->blocks[i].size);
            if(i % 4 == 0)
                fprintf(stderr, "\n");
        }
        if(i % 4 != 1)
            fprintf(stderr, "\n\n");
        else
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "Lua request on allocator %p: "
            "ptr=%p, osize=%u, nsize=%u\n", alloc, ptr, osize, nsize);
#endif
    if(ptr == NULL)
    {
        if(nsize == 0)
        {
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "Returning NULL...\n");
#endif
            return NULL;
        }
        blk = alloc->nb_blocks;
        alloc->nb_blocks++;
        if(alloc->nb_blocks > alloc->size_blocks)
        {
            alloc->size_blocks *= 2;
            alloc->blocks = realloc(alloc->blocks,
                                    sizeof(struct Block) * alloc->size_blocks);
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "Growing block table to %u blocks\n",
                    alloc->size_blocks);
#endif
        }
        alloc->blocks[blk].ptr = NULL;
        alloc->blocks[blk].size = 0;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Allocated new block %u\n", blk);
#endif
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
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Found block %u\n", blk);
#endif
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
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Removed block, now have %u blocks, %u bytes\n\n",
                alloc->nb_blocks, alloc->total_allocated);
#endif
        return NULL;
    }
    else
    {
        ptr = realloc(ptr, nsize);
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "ptr=%p ", ptr);
#endif
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
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "< ");
#endif
        }
        while(blk + 1 < alloc->nb_blocks &&
              alloc->blocks[blk].ptr > alloc->blocks[blk + 1].ptr)
        {
            struct Block tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk + 1];
            alloc->blocks[blk + 1] = tmp;
            blk++;
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "> ");
#endif
        }
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "\n");
        fprintf(stderr, "Moved block to %u, now have %u bytes\n\n",
                blk, alloc->total_allocated);
#endif
        return ptr;
    }
}
