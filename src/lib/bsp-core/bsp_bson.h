/*
 * bsp_bson.h
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
 * Native BSON operator header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/31/2014
 * @changelog 
 *      [10/31/2014] - Creation
 */

#ifndef _LIB_BSP_CORE_BSON_H
#define _LIB_BSP_CORE_BSON_H

/* Headers */

/* Definations */
// Bson types
#define BSON_ELEM_NONE                          0x00
#define BSON_ELEM_DOUBLE                        0x01
#define BSON_ELEM_STRING                        0x02
#define BSON_ELEM_DOCUMENT                      0x03
#define BSON_ELEM_ARRAY                         0x04
#define BSON_ELEM_BINARY                        0x05
#define BSON_ELEM_UNDEFINED                     0x06
#define BSON_ELEM_OID                           0x07
#define BSON_ELEM_BOOLEAN                       0x08
#define BSON_ELEM_UTC_DATETIME                  0x09
#define BSON_ELEM_NULL                          0x0A
#define BSON_ELEM_REGEXP                        0x0B
#define BSON_ELEM_DBPOINTER                     0x0C
#define BSON_ELEM_JS_CODE                       0x0D
#define BSON_ELEM_SYMBOL                        0x0E
#define BSON_ELEM_JS_CODE_WS                    0x0F
#define BSON_ELEM_INT32                         0x10
#define BSON_ELEM_TIMESTAMP                     0x11
#define BSON_ELEM_INT64                         0x12
#define BSON_ELEM_MIN                           0xFF
#define BSON_ELEM_MAX                           0x7F

// Subtypes of binary (0x05)
#define BSON_ELEM_BINARY_GENERIC                0x00
#define BSON_ELEM_BINARY_FUNCTION               0x01
#define BSON_ELEM_BINARY_BINARY                 0x02
#define BSON_ELEM_BINARY_UUID_OLD               0x03
#define BSON_ELEM_BINARY_UUID                   0x04
#define BSON_ELEM_BINARY_MD5                    0x05
#define BSON_ELEM_BINARY_USER                   0x80

// Subtypes of boolean (0x08)
#define BSON_ELEM_BOOLEAN_FALSE                 0x00
#define BSON_ELEM_BOOLEAN_TRUE                  0x01

/* Macros */

/* Structs */

/* Functions */
BSP_STRING * bson_nd_encode(BSP_OBJECT *obj);
BSP_OBJECT * bson_nd_decode(BSP_STRING *str);

#endif  /* _LIB_BSP_CORE_BSON_H */
