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

/* Macros */

/* Structs */
typedef struct bsp_online_entry_t
{
    int                 bind;
    const char          *key;
    BSP_OBJECT          *data;
    struct bsp_online_entry_t
                        *next;
} BSP_ONLINE;

/* Functions */

#endif  /* _LIB_BSP_CORE_ONLINE_H */
