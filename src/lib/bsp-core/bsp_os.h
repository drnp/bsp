/*
 * bsp_os.h
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
 * OS-related functions‘ header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/06/2012
 * @changelog 
 *      [06/06/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_OS_H

#define _LIB_BSP_CORE_OS_H
/* Headers */

/* Definations */

/* Macros */

/* Structs */

/* Functions */
// Set common signals with default behavior
void signal_init(void);

// Regist signal behavior
void set_signal(const int sig, void (cb)(int));

// Make application as daemon process
int proc_daemonize(void);

// Try to enable large page
// Reduce TLB-misses by using large memory page
int enable_large_pages(void);

#endif  /* _LIB_BSP_CORE_OS_H */
