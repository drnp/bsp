/*
 * bsp_string.h
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
 * String operator header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/14/2012
 * @changelog 
 *      [06/12/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_STRING_H

#define _LIB_BSP_CORE_STRING_H
/* Headers */

/* Definations */
#define FREE_STRING_LIST_INITIAL                1024
#define STRING_INITIAL                          60

#define COMPRESS_TYPE_NONE                      0x0
#define COMPRESS_TYPE_DEFLATE                   0x1
#define COMPRESS_TYPE_LZO                       0x2
#define COMPRESS_TYPE_SNAPPY                    0x3

#define COMPRESS_ZLIB_CHUNK_SIZE                16384

/* Macros */

/* Structs */
typedef struct bsp_string_t
{
    char                *str;
    size_t              str_len;
    size_t              ori_len;
    char                compressed;
} BSP_STRING;

/* Functions */
// Create a new string by given data

// If data is NULL or len is zero, a empty string will be created
BSP_STRING * new_string(const char *data, ssize_t len);

// Delete a string
void del_string(BSP_STRING *str);

// Clean string, data will be empty
void clean_string(BSP_STRING *str);

// Duplicate a string
BSP_STRING * clone_string(BSP_STRING *str);

// Append data to an exists string
ssize_t string_append(BSP_STRING *str, const char *data, ssize_t len);

// Fill (enlarge) string to new size. If code >= 0, fill the enlarged space with the value of code (ASCII)
// After enlarge, str_len will be set as zero.
ssize_t string_fill(BSP_STRING *str, int code, size_t len);

// Concat string to the end of an exists string
size_t string_concat(BSP_STRING *str, const char *data, ssize_t len);

// Print formatted data to the end of a string
size_t string_printf(BSP_STRING *str, const char *fmt, ...);

// Find a sub-string from a string and replace them with new value
void string_replace(BSP_STRING *str, const char *search, const char *replace);

// Compress data
int string_compress_deflate(BSP_STRING *str);
int string_compress_lzo(BSP_STRING *str);
int string_compress_snappy(BSP_STRING *str);

// Uncompress data
int string_uncompress_deflate(BSP_STRING *str);
int string_uncompress_lzo(BSP_STRING *str);
int string_uncompress_snappy(BSP_STRING *str);

// Base64 encode
int string_base64_encode(BSP_STRING *str, const char *data, ssize_t len);

// Base64_decode
int string_base64_decode(BSP_STRING *str, const char *data, ssize_t len);

#endif  /* _LIB_BSP_CORE_STRING_H */
