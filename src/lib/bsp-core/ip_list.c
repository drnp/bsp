/*
 * ip_list.c
 *
 * Copyright (C) 2013 - Dr.NP
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
 * IPv4 / IPv6 list, used for whitelist / blacklist
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 04/07/2013
 * @changelog
 *      [04/07/2013] - Creation
 */

#include <netinet/in.h>

#include "bsp.h"

// New IPv4 table
BSP_IPV4_LIST * new_ipv4_list()
{
    BSP_IPV4_LIST *ret = mempool_alloc(sizeof(BSP_IPV4_LIST));
    if (!ret)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Create IP list error");
        return NULL;
    }

    memset(ret, 0, sizeof(BSP_IPV4_LIST));

    return ret;
}

// New IPv6 table
BSP_IPV6_LIST * new_ipv6_list()
{
    BSP_IPV6_LIST *ret = mempool_alloc(sizeof(BSP_IPV6_LIST));
    if (!ret)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Create IP list error");
        return NULL;
    }

    memset(ret, 0, sizeof(BSP_IPV6_LIST));

    return ret;
}

// Search a IPv4 address(unsigned integer format) in table
int search_ipv4(BSP_IPV4_LIST *list, uint32_t addr)
{
    if (!list)
    {
        return 0;
    }

    int slab_id = ipv4_hash(addr, IPLIST_SIZE);
    struct bsp_ipv4_item_t *item = list->slab[slab_id][0];
    while (item)
    {
        if (item->addr == addr)
        {
            return 1;
        }
        
        item = item->next;
    }

    return 0;
}

// Add a IPv4 address to table
void add_ipv4(BSP_IPV4_LIST *list, uint32_t addr)
{
    if (search_ipv4(list, addr))
    {
        // Already in list
        return;
    }
    
    if (!list)
    {
        return;
    }

    struct bsp_ipv4_item_t *item = mempool_alloc(sizeof(struct bsp_ipv4_item_t));
    item->addr = addr;
    item->next = NULL;
    int slab_id = ipv4_hash(addr, IPLIST_SIZE);
    if (!list->slab[slab_id][0] || !list->slab[slab_id][1])
    {
        // Header
        list->slab[slab_id][0] = item;
    }

    else
    {
        // Add link
        list->slab[slab_id][1]->next = item;
    }

    list->slab[slab_id][1] = item;

    return;
}

// Remove a IPv4 address from table
void del_ipv4(BSP_IPV4_LIST *list, uint32_t addr)
{
    if (!list)
    {
        return;
    }

    int slab_id = ipv4_hash(addr, IPLIST_SIZE);
    struct bsp_ipv4_item_t *item = list->slab[slab_id][0];
    while (item)
    {
        if (item->addr == addr)
        {
            // Recover with zero
            item->addr = 0;
        }
        
        item = item->next;
    }

    return;
}

// Search a IPv5 address(unsigned byte array format) from table
int search_ipv6(BSP_IPV6_LIST *list, uint8_t *addr)
{
    if (!list)
    {
        return 0;
    }

    int slab_id = ipv6_hash(addr, IPLIST_SIZE);
    struct bsp_ipv6_item_t *item = list->slab[slab_id][0];
    while (item)
    {
        if (0 == memcmp(item->addr, addr, 16))
        {
            return 1;
        }
        
        item = item->next;
    }

    return 0;
}

// Add a IPv6 address to table
void add_ipv6(BSP_IPV6_LIST *list, uint8_t *addr)
{
    if (search_ipv6(list, addr))
    {
        // Already in list
        return;
    }
    
    if (!list)
    {
        return;
    }

    struct bsp_ipv6_item_t *item = mempool_alloc(sizeof(struct bsp_ipv6_item_t));
    memcpy(item->addr, addr, 16);
    item->next = NULL;
    int slab_id = ipv6_hash(addr, IPLIST_SIZE);
    if (!list->slab[slab_id][0] || !list->slab[slab_id][1])
    {
        // Header
        list->slab[slab_id][0] = item;
    }

    else
    {
        // Add link
        list->slab[slab_id][1]->next = item;
    }

    list->slab[slab_id][1] = item;

    return;
}

// Remove a IPv6 address from table
void del_ipv6(BSP_IPV6_LIST *list, uint8_t *addr)
{
    if (!list)
    {
        return;
    }

    int slab_id = ipv6_hash(addr, IPLIST_SIZE);
    struct bsp_ipv6_item_t *item = list->slab[slab_id][0];
    while (item)
    {
        if (0 == memcmp(item->addr, addr, 16))
        {
            // Recover with zero
            memset(item->addr, 0, 16);
        }
        
        item = item->next;
    }

    return;
}
