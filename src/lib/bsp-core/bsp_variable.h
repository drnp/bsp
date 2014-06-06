/*
 * bsp_variable.h
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
 * Varible operator
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/11/2012
 * @changelog 
 *      [06/11/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_VARIBLE_H

#define _LIB_BSP_CORE_VARIBLE_H
/* Headers */

/* Definations */
#define BSP_VAL_INT8                            0x1
#define BSP_VAL_INT16                           0x2
#define BSP_VAL_INT32                           0x3
#define BSP_VAL_INT64                           0x4
#define BSP_VAL_BOOLEAN                         0x5
#define BSP_VAL_FLOAT                           0x11
#define BSP_VAL_DOUBLE                          0x12
#define BSP_VAL_STRING                          0x21
#define BSP_VAL_POINTER                         0x31
#define BSP_VAL_OBJECT                          0x51
#define BSP_VAL_OBJECT_END                      0x52
#define BSP_VAL_ARRAY                           0x61
#define BSP_VAL_ARRAY_END                       0x62
#define BSP_VAL_NULL                            0x7F

#define BSP_OP_EQUAL                            0x1
#define BSP_OP_NE                               0x2
#define BSP_OP_GT                               0x11
#define BSP_OP_GE                               0x12
#define BSP_OP_LT                               0x21
#define BSP_OP_LE                               0x22

#define BSP_BOOLEAN_TRUE                        1
#define BSP_BOOLEAN_FALSE                       0

/* Macros */

/* Structs */

/* Functions */
// Set values
inline void set_bit(int bit, char *addr, size_t idx);
inline void set_int8(int8_t data, char *addr);
inline void set_int16(int16_t data, char *addr);
inline void set_int32(int32_t data, char *addr);
inline void set_int64(int64_t data, char *addr);
inline void set_float(float data, char *addr);
inline void set_double(double data, char *addr);
inline void set_string(const char *data, ssize_t len, char *addr);
inline void set_pointer(const void *p, char *addr);

// Get values
inline int get_bit(const char *addr, size_t idx);
inline int8_t get_int8(const char *addr);
inline int16_t get_int16(const char *addr);
inline int32_t get_int32(const char *addr);
inline int64_t get_int64(const char *addr);
inline float get_float(const char *addr);
inline double get_double(const char *addr);
inline char * get_string(const char *addr);
inline void * get_pointer(const char *addr);

void get_rand(char *data, size_t len);
ssize_t trimmed_strlen(const char *input);

const char * escape_char(unsigned char c);
int32_t utf8_to_value(const char *data, ssize_t len, int *size);

#endif  /* _LIB_BSP_CORE_VARIBLE_H */
