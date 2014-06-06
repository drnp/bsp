/*
 * bsp_mempool.h
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
 * Memory pool header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/18/2013
 * @changelog
 *      [06/07/2012] - Creation
 *      [08/23/2012] - mempool_strdup added
 *      [07/18/2013] - mempool_alloc_usable_size
 */

#ifndef _LIB_BSP_CORE_MEMPOOL_H

#define _LIB_BSP_CORE_MEMPOOL_H
/* Headers */

/* Definations */
#define SLAB_BLOCK_LIST_SIZE                    131072      // 512GB - 8TB memory space per pool
#define SLAB_MAX                                55

/* Macros */
#define BLOCK_SIZE(i)                           ((slab_size[i] + 8) * slab_nitems[i])

/* Structs */
struct bsp_mempool_slab_t
{
    int                 slab_id;
    size_t              curr_block_alloced;
    void                *curr_block;
    void                *next_free_item;
    BSP_SPINLOCK        slab_lock;
};

typedef struct bsp_mempool_t
{
    struct bsp_mempool_slab_t
                        slab_list[SLAB_MAX];
    BSP_SPINLOCK        lock;
} BSP_MEMPOOL;

/* Functions */
int mempool_init();
void * mempool_alloc(size_t nsize);
void * mempool_calloc(size_t nmemb, size_t size);
void * mempool_realloc(void *addr, size_t nsize);
char * mempool_strdup(const char *input);
char * mempool_strndup(const char *input, ssize_t len);
size_t mempool_alloc_usable_size(void *addr);
int mempool_free(void *addr);

#endif  /* _LIB_BSP_CORE_MEMPOOL_H */
