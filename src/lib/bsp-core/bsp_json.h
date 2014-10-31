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
#define JSON_DECODE_STATUS_STR                  0b00000001
#define JSON_DECODE_STATUS_STRIP                0b00000010
#define JSON_DECODE_STATUS_UTF                  0b00000100
#define JSON_DECODE_STATUS_DIGIT                0b00001000
#define JSON_DECODE_STATUS_BOOLEAN              0b00010000

/* Macros */

/* Structs */

/* Functions */
BSP_STRING * json_nd_encode(BSP_OBJECT *obj);
BSP_OBJECT * json_nd_decode(BSP_STRING *str);

#endif  /* _LIB_BSP_CORE_JSON_H */
