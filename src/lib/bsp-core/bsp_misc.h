/*
 * bsp_misc.h
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
 * Miscellaneous functions header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/31/2012
 * @changelog 
 *      [05/31/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_MISC_H

#define _LIB_BSP_CORE_MISC_H
/* Headers */

/* Definations */
#define MAX_TRACE_LENGTH                        32000

#define TRACE_LEVEL_CORE                        0b00000001
#define TRACE_LEVEL_FATAL                       0b00000010
#define TRACE_LEVEL_ERROR                       0b00000100
#define TRACE_LEVEL_NOTICE                      0b00001000
#define TRACE_LEVEL_DEBUG                       0b00010000
#define TRACE_LEVEL_VERBOSE                     0b00100000

#define TRACE_LEVEL_NONE                        0b00000000
#define TRACE_LEVEL_ALL                         0b11111111

/* Macros */

/* Structs */

/* Functions */
// Get current running process' location
char * get_dir(void);

// Set current working dir
void set_dir(const char *dir);

// Save PID file (a text file whose content is process ID)
void save_pid(void);

// Trace information, if handle function registered in global setting, message will be proceed by them.
size_t trace_msg(int level, const char *fmt, ...);

// Return message level in string format, if with_color set non-zero, terminal color information will be included
char * get_trace_level_str(int level, int with_color);

// Filtering non-printable characters, replace them by given substitute
int filter_non_pritable_char(char *input, ssize_t len, char r);

#endif  /* _LIB_BSP_CORE_MISC_H */
