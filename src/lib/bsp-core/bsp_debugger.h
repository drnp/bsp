/*
 * bsp_debugger.h
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
 * Data tracer header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/14/2012
 * @changelog 
 *      [06/14/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_DEBUGGER_H

#define _LIB_BSP_CORE_DEBUGGER_H
/* Headers */

/* Definations */

/* Macros */

/* Structs */

/* Functions */
// Trace_MSG callback. Output trace info with color
void trace_output(time_t now, int level, const char *msg);
void trigger_exit(int level, const char *fmt, ...);

// Output a formatted string
void debug_str(const char *fmt, ...);
void debug_hex(const char *data, ssize_t len);
void debug_object(BSP_OBJECT *obj);

#endif  /* _LIB_BSP_CORE_DEBUGGER_H */
