/*
 * bsp_logger.h
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
 * Text message logger header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/14/2012
 * @chagelog 
 *      [06/14/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_LOGGER_H

#define _LIB_BSP_CORE_LOGGER_H
/* Headers */

/* Definations */
#define DEFAULT_LOG_DIR                         "log/"
#define LOG_FILENAME_PREFIX                     "bsp-"
#define MAX_LOG_LINE_LENGTH                     32768

/* Macros */

/* Structs */

/* Functions */
void log_init();
void log_add(time_t now, int level, const char *msg);
void log_open();
void log_close();

#endif  /* _LIB_BSP_CORE_LOGGER_H */
