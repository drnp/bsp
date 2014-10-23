/*
 * online.c
 *
 * Copyright (C) 2014 - Dr.NP
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
 * Online pool services
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/22/2014
 * @changelog
 *      [10/22/2014[ - Creation
 */

#include "bsp.h"

// Hash table
BSP_ONLINE **online_hash = NULL;
BSP_SPINLOCK hash_lock;

// Initialize online hash table
int online_init()
{
    //BSP_CORE_SETTING *settings = get_core_setting();
    online_hash = bsp_calloc(ONLINE_HASH_SIZE, sizeof(BSP_ONLINE *));
    if (!online_hash)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "Cannot create online list");
    }
    bsp_spin_init(&hash_lock);

    return BSP_RTN_SUCCESS;
}

/* Hash operations */
static BSP_ONLINE * _hash_find(const char *key)
{
    if (!key || !online_hash)
    {
        return NULL;
    }

    uint32_t h = bsp_hash(key, -1);
    int idx = (h % ONLINE_HASH_SIZE);
    bsp_spin_lock(&hash_lock);
    BSP_ONLINE *try = online_hash[idx];
    BSP_ONLINE *ret = NULL;

    while (try)
    {
        if (0 == strcmp(key, try->key))
        {
            ret = try;
            break;
        }
        try = try->next;
    }
    bsp_spin_unlock(&hash_lock);

    return ret;
}

static void _hash_insert(BSP_ONLINE *entry)
{
    if (!entry || !online_hash)
    {
        return;
    }

    uint32_t h = bsp_hash(entry->key, -1);
    int idx = (h % ONLINE_HASH_SIZE);
    bsp_spin_lock(&hash_lock);
    entry->next = online_hash[idx];
    online_hash[idx] = entry;
    bsp_spin_unlock(&hash_lock);

    return;
}

static BSP_ONLINE * _hash_remove(const char *key)
{
    if (!key || !online_hash)
    {
        return NULL;
    }

    uint32_t h = bsp_hash(key, -1);
    int idx = (h % ONLINE_HASH_SIZE);
    bsp_spin_lock(&hash_lock);
    BSP_ONLINE *try = online_hash[idx];
    BSP_ONLINE *old = NULL, *ret = NULL;
    while (try)
    {
        if (0 == strcmp(key, try->key))
        {
            // Same ID, remove
            if (!old)
            {
                // Head
                online_hash[idx] = try->next;
            }
            else
            {
                old->next = try->next;
            }

            ret = try;
            break;
        }
        old = try;
        try = try->next;
    }
    bsp_spin_unlock(&hash_lock);

    return ret;
}

// Create new online entry
BSP_ONLINE * new_online(const char *key)
{
    if (!key)
    {
        return NULL;
    }

    BSP_ONLINE *entry = bsp_calloc(1, sizeof(BSP_ONLINE));
    if (entry)
    {
        entry->key = key;
        // Insert into hash
    }

    return entry;
}
