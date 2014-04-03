/*
 * module_standard.h
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
 * Standard core interfaces header
 * 
 * @package modules::standard
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/06/2012
 * @changelog 
 *      [08/06/2012] - Creation
 */

#ifndef _MODULES_STANDARD_H

#define _MODULES_STANDARD_H
/* Headers */

/* Definations */
#define GLOBAL_NAME_LENGTH                      32
#define GLOBAL_HASH_SIZE                        64

/* Macros */

/* Structs */
struct _global_entry_t
{
    char                key[GLOBAL_NAME_LENGTH];
    char                value[8];
    size_t              value_len;
    int                 type;
    struct _global_entry_t
                        *next;
};

/* Functions */
int bsp_module_standard(lua_State *s);

#endif  /* _BSP_MODULES_STANDARD_H */
