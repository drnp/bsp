/*
 * bsp_json.h
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
 * Native JSON encoder / decoder header
 * 
 * @package libbsp::core
 * @author Dr.NP <np@bsgroup.org>
 * @update 01/14/2014
 * @chagelog 
 *      [01/14/2014] - Creation
 */

#ifndef _LIB_BSP_CORE_JSON_H

#define _LIB_BSP_CORE_JSON_H
/* Headers */

/* Definations */

/* Macros */

/* Structs */

/* Functions */
int json_nd_encode(BSP_OBJECT *obj, BSP_STRING *str);
int json_nd_decode(const char *data, ssize_t len, BSP_OBJECT *obj);

#endif  /* _LIB_BSP_CORE_JSON_H */
