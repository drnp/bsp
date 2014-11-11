/*
 * bsp_online.h
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
 * Online pool services header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/22/2014
 * @changelog
 *      [10/22/2014[ - Creation
 */

#ifndef _LIB_BSP_CORE_ONLINE_H

#define _LIB_BSP_CORE_ONLINE_H
/* Headers */

/* Definations */
#define ONLINE_HASH_SIZE                        4096
#define ONLINE_HANDLER_NAME_SAVE                "_bsp_online_handler_save_"
#define ONLINE_HANDLER_NAME_LOAD                "_bsp_online_handler_load_"
#define DEFAULT_ONLINE_AUTOSAVE_INTERVAL        300

/* Macros */

/* Structs */
typedef struct bsp_online_entry_t
{
    int                 bind;
    time_t              last_save;
    char                *key;
    BSP_OBJECT          *data;
    struct bsp_online_entry_t
                        *next;
} BSP_ONLINE;

/* Functions */
// Initialize online hash table
int online_init();

// Create new online entry
void new_online(int fd, const char *key);

// Remove online entry
void del_online_by_bind(int fd);
void del_online_by_key(const char *key);

// Load and save
int load_online_data_by_bind(int fd);
int load_online_data_by_key(const char *key);
int save_online_data_by_bind(int fd);
int save_online_data_by_key(const char *key);

// Get data
BSP_OBJECT * get_online_data_by_key(const char *key);
BSP_OBJECT * get_online_data_by_bind(int fd);

// Online list
BSP_OBJECT * get_online_list(const char *condition);

#endif  /* _LIB_BSP_CORE_ONLINE_H */
