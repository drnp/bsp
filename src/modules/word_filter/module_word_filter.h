/*
 * module_word_filter.h
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
 * Keyword filter with finite automation, header file
 * 
 * @package modules::word_filter
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/23/2013
 * @changelog
 *      [05/23/2013] - Creation
 */

#ifndef _MODULE_WORD_FILTER_H
#define _MODULE_WORD_FILTER_H
/* Headers */

/* Definations */
#define BLOCK_NODE_SIZE                         1270
#define BLOCK_LIST_INITIAL                      16
#define DEFAULT_REPLACEMENT                     "*"

/* Macros */

/* Structs */
struct hash_tree_node_t
{
    // 1032 bytes per node
    int                 length;
    int                 value;
    // 256 path (ASCII code) each for crotch
    // We support binary data here
    int                 path[256];
};

struct word_filter_t
{
    int                 node_used;
    BSP_SPINLOCK        node_lock;
    struct hash_tree_node_t
                        **node_list;
    int                 node_list_size;
    int                 root;
};

/* Functions */

#endif  /* _MODULE_WORD_FILTER_H */
