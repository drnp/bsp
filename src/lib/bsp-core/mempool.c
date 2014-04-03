/*
 * mempool.c
 *
 * Copyright (C) 2012 - Dr.NP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Memory pool
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/23/2012
 * @changelog
 *      [06/07/2012] - Creation
 *      [08/23/2012] - mempool_strdup added
 *      [07/09/2013] - Dissociative bug fixed / free_item_link re-generated
 */

#include "bsp.h"

#ifdef ENABLE_MEMPOOL

BSP_MEMPOOL *pool = NULL;
size_t slab_size[SLAB_MAX] = {
    32,     40,     50,     64,     80,     100, 
    128,    160,    200,    256,    320,    400, 
    512,    640,    800,    1024,   1280,   1600, 
    2048,   2560,   3200,   4096,   5120,   6400, 
    8192,   10240,  12800,  16384,  20480,  25600, 
    32768,  40960,  51200,  65536,  81920,  102400, 
    131072, 163840, 204800, 262144, 327680, 409600, 
    524288, 655360, 819200, 1048576,1310720,1638400, 
    2097152,2621440,3276800,4194304,5242880,6553600, 
    8388608
};

size_t slab_nitems[SLAB_MAX] = {
    131072, 131072, 131072, 131072, 131072, 131072, 
    65536,  65536,  65536,  32768,  32768,  32768, 
    16384,  16384,  16384,  8192,   8192,   8192, 
    4096,   4096,   4096,   2048,   2048,   2048, 
    1024,   1024,   1024,   512,    512,    512, 
    256,    256,    256,    128,    128,    128, 
    64,     64,     64,     32,     32,     32, 
    16,     16,     16,     8,      8,      8, 
    8,      8,      8,      8,      8,      8, 
    8
};

// Initialize memory pool
int mempool_init()
{
    pool = calloc(1, sizeof(BSP_MEMPOOL));
    if (!pool)
    {
        _exit(BSP_RTN_ERROR_MEMORY);
    }
    bsp_spin_init(&pool->lock);

    int i;
    for (i = 0; i < SLAB_MAX; i ++)
    {
        pool->slab_list[i].slab_id = i;
        bsp_spin_init(&pool->slab_list[i].slab_lock);
    }

    trace_msg(TRACE_LEVEL_CORE, "Memory : Memory pool initialized as %d slabs", SLAB_MAX);
    
    return BSP_RTN_SUCCESS;
}

static void _mempool_free_item(void *addr, struct bsp_mempool_slab_t *slab)
{
    if (!addr || !pool || !slab)
    {
        return;
    }
    
    // Recycle to free link
    set_pointer(slab->next_free_item, addr);
    slab->next_free_item = addr;

    return;
}

static void * _pop_next_free_item(struct bsp_mempool_slab_t *slab)
{
    void *ret = slab->next_free_item;
    if (ret)
    {
        slab->next_free_item = get_pointer(ret);
    }

    return ret;
}

// Calculate the slab id by given size
static int _get_slab_id(int nsize)
{
    int slab_id = 0;
    if (nsize > slab_size[SLAB_MAX - 1])
    {
        return -1;
    }

    // Slab item size = 1.25 * 1.25 * 1.28 * 1.25 * 1.25 * 1.28 ...
    for (slab_id = 0; slab_id < SLAB_MAX; slab_id ++)
    {
        if (slab_size[slab_id] >= nsize)
        {
            break;
        }
    }

    return slab_id;
}

// Get Item size by given slab ID
static size_t _get_slab_item_size(int slab_id)
{
    return (slab_id < SLAB_MAX && slab_id >= 0) ? slab_size[slab_id] : 0;
}

// Fetch space from pool
void * mempool_alloc(size_t nsize)
{
    void *ret = NULL;
    size_t osize = nsize + 8;
    struct bsp_mempool_slab_t *slab = NULL;
    if (!pool)
    {
        // Not initialized
        trigger_exit(BSP_RTN_FATAL, "Memory : Mempool not initialized!!!");
    }

    if (!nsize)
    {
        return NULL;
    }
    
    int slab_id = _get_slab_id(nsize);
    if (slab_id < 0)
    {
        // A huge item, Simply alloc one block for this
        ret = malloc(osize);
        bsp_spin_lock(&pool->lock);
        status_op_mempool(slab_id, STATUS_OP_MEMPOOL_HUGE_ALLOC, osize);
        bsp_spin_unlock(&pool->lock);
        trace_msg(TRACE_LEVEL_VERBOSE, "Memory : Alloc a big memory block, size %lld", (long long int) nsize);
    }

    else
    {
        slab = &pool->slab_list[slab_id];
        bsp_spin_lock(&slab->slab_lock);
        if (slab->next_free_item)
        {
            // Recycled item
            ret = _pop_next_free_item(slab);
        }

        else
        {
            if (!slab->curr_block || slab->curr_block_alloced >= slab_nitems[slab_id])
            {
                // Need new block
                void *block = malloc(BLOCK_SIZE(slab_id));
                if (!block)
                {
                    bsp_spin_unlock(&slab->slab_lock);
                    trigger_exit(BSP_RTN_FATAL, "Memory : Allocate memory block error!!!");
                    return NULL;
                }
                //slab->used_blocks ++;
                slab->curr_block = block;
                slab->curr_block_alloced = 0;
                status_op_mempool(slab_id, STATUS_OP_MEMPOOL_BLOCK, BLOCK_SIZE(slab_id));
                status_op_mempool(slab_id, STATUS_OP_MEMPOOL_ITEM, slab_nitems[slab_id]);
                trace_msg(TRACE_LEVEL_VERBOSE, "Memory : Allocated a new memory block to slab %d.", slab_id);
            }

            ret = slab->curr_block + (slab->curr_block_alloced ++) * (slab_size[slab_id] + 8);
        }
        status_op_mempool(slab_id, STATUS_OP_MEMPOOL_ALLOC, 0);
        bsp_spin_unlock(&slab->slab_lock);
    }

    if (ret)
    {
        set_pointer((void *) slab, ret);
        return ret + 8;
    }
    return NULL;
}

void * mempool_calloc(size_t nmemb, size_t size)
{
    void *ptr = mempool_alloc(nmemb * size);
    if (ptr)
    {
        memset(ptr, 0, nmemb * size);
    }

    return ptr;
}

// Realloc a memory space
void * mempool_realloc(void *addr, size_t nsize)
{
    if (!pool)
    {
        // Not initialized
        trigger_exit(BSP_RTN_FATAL, "Memory : Mempool not initialized!!!");
    }
    
    if (!addr)
    {
        return mempool_alloc(nsize);
    }

    void *old_addr = addr - 8;
    void *new_addr = NULL;
    struct bsp_mempool_slab_t *slab = get_pointer(old_addr);
    if (!slab)
    {
        // Huge block
        new_addr = realloc(old_addr, nsize + 8);
        return (new_addr) ? new_addr + 8 : NULL;
    }
    size_t slab_item_size = _get_slab_item_size(slab->slab_id);
    if (nsize <= slab_item_size)
    {
        // Needn't move data, just return the same addr
        return addr;
    }
    
    else
    {
        // Move data to a new slab
        void *new_addr = mempool_alloc(nsize);
        memcpy(new_addr, addr, slab_item_size);
        mempool_free(addr);
        
        return new_addr;
    }
    
    return NULL;
}

// Duplicate a string to mempool
char * mempool_strdup(const char *input)
{
    if (!input)
    {
        return NULL;
    }
    size_t len = strlen(input) + 1;
    char *new = mempool_alloc(len);
    if (new)
    {
        memcpy(new, input, len);
        new[len - 1] = 0x0;
    }

    return new;
}

// Duplicate a lengthed-string to mempool
char * mempool_strndup(const char *input, ssize_t len)
{
    if (!input)
    {
        return NULL;
    }
    if (len < 0)
    {
        len = strlen(input);
    }
    
    char *new = mempool_alloc(len + 1);
    if (new)
    {
        memcpy(new, input, len);
        new[len] = 0x0;
    }

    return new;
}

// Check usable space with an alloced pointer
size_t mempool_alloc_usable_size(void *addr)
{
    if (!pool || !addr)
    {
        return 0;
    }
    
    void *oaddr = addr - 8;
    struct bsp_mempool_slab_t *slab = get_pointer(oaddr);
    if (!slab)
    {
        // Huge block
        return malloc_usable_size(oaddr);
    }

    return _get_slab_item_size(slab->slab_id);
}

// Return space to free list
int mempool_free(void *addr)
{
    if (!addr || !pool)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    void *ori_addr = addr - 8;
    struct bsp_mempool_slab_t *slab = get_pointer(ori_addr);
    if (!slab)
    {
        // Huge block
        bsp_spin_lock(&pool->lock);
        status_op_mempool(0, STATUS_OP_MEMPOOL_HUGE_FREE, malloc_usable_size(ori_addr));
        bsp_spin_unlock(&pool->lock);
        free(ori_addr);
    }

    else
    {
        // Recycle
        if (slab->slab_id * sizeof(struct bsp_mempool_slab_t) + (void *) pool != (void *) slab)
        {
            // Double free
            trace_msg(TRACE_LEVEL_DEBUG, "Memory : Double free occurred");
        }

        else
        {
            bsp_spin_lock(&slab->slab_lock);
            _mempool_free_item(ori_addr, slab);
            status_op_mempool(slab->slab_id, STATUS_OP_MEMPOOL_FREE, 0);
            bsp_spin_unlock(&slab->slab_lock);
        }
    }
    
    return BSP_RTN_SUCCESS;
}

#endif
