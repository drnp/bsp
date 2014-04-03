/*
 * module_online.h
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
 * Online list header. Thread-safe for multi LUA states
 * 
 * @package module::online
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/15/2013
 * @changelog
 *      [08/15/2013] - Creation
 */

#ifndef _MODULES_ONLINE_H

#define _MODULES_ONLINE_H
/* Headers */

/* Definations */
#define ONLINE_MAP_KEY_LENGTH                   32
#define DEFAULT_ONLINE_INFO_STRING_LENGTH       32
#define ONLINE_INFO_HASH_SIZE                   64

#define ONLINE_STATUS_UNINITIALIZED             0
#define ONLINE_STATUS_ONLINE                    1
#define ONLINE_STATUS_BINDED                    2
#define ONLINE_STATUS_BLOCKED                   3
#define ONLINE_STATUS_INVALID                   4
#define ONLINE_STATUS_OFFLINE                   5

/* Macros */

/* Structs */
struct _online_entry_t
{
    void                *info;
    int                 status;
    BSP_SPINLOCK        lock;
};

struct _online_info_map_t
{
    char                key[ONLINE_MAP_KEY_LENGTH];
    int                 type;
    int                 index;
    int                 offset;
    int                 length;
    struct _online_info_map_t
                        *next;
    struct _online_info_map_t
                        *lnext;
};

/* Functions */
int bsp_module_online(lua_State *s);

#endif
