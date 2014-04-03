/*
 * bsp-core.h
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
 * libbsp-core header
 * 
 * @package bsp::libbsp-core
 * @Author Dr.NP <np@bsgroup.org>
 * @update 03/29/2013
 * @changelog 
 *      [06/04/2012] - Creation
 *      [03/29/2013] - Move ext into core
 */

#ifndef _LIB_BSP_CORE_H

#define _LIB_BSP_CORE_H
/* Headers */
#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#ifdef ENABLE_JEMALLOC
    #include <jemalloc/jemalloc.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#if HAVE_MALLOC_H
    #include <malloc.h>
#endif
#if HAVE_MATH_H
    #include <math.h>
#endif
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/time.h>

#include "bsp_spinlock.h"
#include "bsp_variable.h"
#include "bsp_string.h"
#include "bsp_object.h"
#include "bsp_json.h"
#include "bsp_msgpack.h"
#include "bsp_amf.h"
#include "bsp_debugger.h"
#include "bsp_script.h"
#include "bsp_timer.h"
#include "bsp_logger.h"
#include "bsp_conf.h"
#include "bsp_fd.h"
#include "bsp_hash.h"
#include "bsp_mempool.h"
#include "bsp_misc.h"
#include "bsp_os.h"
#include "bsp_socket.h"
#include "bsp_thread.h"
#include "bsp_db_sqlite.h"
#include "bsp_db_mysql.h"
#include "bsp_ip_list.h"
#include "bsp_http.h"
#include "bsp_server.h"
#include "bsp_core.h"
#include "bsp_status.h"

/* Structs */

/* Definations */
// General
#define BSP_CORE_VERSION                        "BS.Play(SickyCat)-20130812-dev"
#define BSP_PACKAGE_NAME                        "bsp"

// Return values
#define BSP_RTN_SUCCESS                         0
#define BSP_RTN_OS                              0xFFFE
#define BSP_RTN_FATAL                           0xFFFF
#define BSP_RTN_ERROR_GENERAL                   1
#define BSP_RTN_ERROR_UNKNOWN                   2
#define BSP_RTN_ERROR_SIGNAL                    3
#define BSP_RTN_ERROR_IO                        11
#define BSP_RTN_ERROR_MEMORY                    21
#define BSP_RTN_ERROR_PTHREAD                   31
#define BSP_RTN_ERROR_RESOURCE                  41

#define BSP_RTN_ERROR_EPOLL                     1001
#define BSP_RTN_ERROR_EVENTFD                   1002
#define BSP_RTN_ERROR_TIMERFD                   1003
#define BSP_RTN_ERROR_SIGNALFD                  1004

#define BSP_RTN_ERROR_NETWORK                   2001
#define BSP_RTN_ERROR_SCRIPT                    3001

#ifndef _POSIX_PATH_MAX
    #define _POSIX_PATH_MAX                     1024
#endif

#ifndef _SYMBOL_NAME_MAX
    #define _SYMBOL_NAME_MAX                    64
#endif

/* Macros */

/* Functions */

#endif  /* _LIB_BSP_CORE_H */
