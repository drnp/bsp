/*
 * conf_parser.c
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
 * Parse config file (linux ini-style) into object
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/02/2012
 * @changelog 
 *      [07/02/2012] - Creation
 *      [03/28/2013] - Moved to core
 *      [07/31/2013] - Rebuild
 */

#include "bsp.h"

struct bsp_conf_param_t conf_list[CONF_HASH_SIZE];
struct bsp_conf_param_t **conf_index;
BSP_SPINLOCK conf_lock;
size_t conf_total, conf_index_size;

// Initialization
int conf_init(const char *conf_file)
{
    //memset(&conf, 0, sizeof(BSP_CONF));
    // Conf hash
    bsp_spin_init(&conf_lock);
    memset(conf_list, 0, CONF_HASH_SIZE * sizeof(struct bsp_conf_param_t));
    conf_index = bsp_calloc(CONF_INDEX_INITIAL, sizeof(struct bsp_conf_param_t *));

    if (!conf_index)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "Configure table alloc error");
    }

    conf_total = 0;
    conf_index_size = CONF_INDEX_INITIAL;
    trace_msg(TRACE_LEVEL_VERBOSE, "Conf   : Configure table initialized");

    return BSP_RTN_SUCCESS;
}

// Set a configure item
void conf_set(const char *key, const char *value, int level)
{
    char modified = 0x0;
    int hash_key = (int) bsp_hash(key, strlen(key)) % CONF_HASH_SIZE;
    struct bsp_conf_param_t *last = &conf_list[hash_key];
    struct bsp_conf_param_t *head = last->next, **tmp;

    if (!key || !conf_index)
    {
        return;
    }

    if (!value)
    {
        value = "";
    }

    bsp_spin_lock(&conf_lock);
    while (head)
    {
        if (0 == strcmp(head->key, key))
        {
            if (level >= head->level)
            {
                if (head->value)
                {
                    bsp_free(head->value);
                }

                head->value = bsp_strdup(value);
                head->level = level;
            }

            modified = 0x1;
            break;
        }

        last = head;
        head = head->next;
    }

    if (!modified)
    {
        // Add new node
        head = bsp_malloc(sizeof(struct bsp_conf_param_t));
        if (!head)
        {
            return;
        }

        head->key = bsp_strdup(key);
        head->value = bsp_strdup(value);
        head->level = level;
        head->next = NULL;
        last->next = head;

        while (conf_total >= conf_index_size)
        {
            tmp = bsp_realloc(conf_index, sizeof(struct bsp_conf_param_t *) * conf_index_size * 2);
            if (!tmp)
            {
                continue;
            }

            conf_index = tmp;
            conf_index_size *= 2;
        }

        conf_index[conf_total ++] = head;
    }

    bsp_spin_unlock(&conf_lock);

    return;
}

// Get configure
char * conf_get(const char *key)
{
    char *value = "";
    int hash_key;
    struct bsp_conf_param_t *curr = NULL;

    if (key && conf_index)
    {
        hash_key = (int) bsp_hash(key, strlen(key)) % CONF_HASH_SIZE;
        curr = conf_list[hash_key].next;

        while (curr)
        {
            if (0 == strcmp(key, curr->key))
            {
                value = curr->value;
                break;
            }

            curr = curr->next;
        }
    }

    return value;
}
