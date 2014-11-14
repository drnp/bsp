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
#include <pthread.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define HAVE_MSG_NOSIGNAL                       1

#include <glib.h>
#include <mongo.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "bsp_spinlock.h"
#include "bsp_variable.h"
#include "bsp_mempool.h"
#include "bsp_string.h"
#include "bsp_object.h"
#include "bsp_json.h"
#include "bsp_msgpack.h"
#include "bsp_bson.h"
#include "bsp_amf.h"
#include "bsp_debugger.h"
#include "bsp_script.h"
#include "bsp_timer.h"
#include "bsp_logger.h"
#include "bsp_conf.h"
#include "bsp_online.h"
#include "bsp_fd.h"
#include "bsp_hash.h"
#include "bsp_memdb.h"
#include "bsp_misc.h"
#include "bsp_os.h"
#include "bsp_http.h"
#include "bsp_fcgi.h"
#include "bsp_socket.h"
#include "bsp_thread.h"
#include "bsp_db_sqlite.h"
#include "bsp_db_mysql.h"
#include "bsp_db_mongodb.h"
#include "bsp_ip_list.h"
#include "bsp_server.h"
#include "bsp_bootstrap.h"
#include "bsp_core.h"
#include "bsp_status.h"

/* Structs */

/* Definations */
// General
#define BSP_CORE_VERSION                        "BS.Play(SickyCat)-20141103-dev"
#define BSP_PACKAGE_NAME                        "bsp"

// Instances
#define INSTANCE_MANAGER                        0
#define INSTANCE_UNKNOWN                        -1

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
#define BSP_RTN_ERROR_DB                        4001

// Compatible
#ifndef _POSIX_PATH_MAX
    #define _POSIX_PATH_MAX                     1024
#endif

#ifndef _SYMBOL_NAME_MAX
    #define _SYMBOL_NAME_MAX                    64
#endif

// Runtime
#define DEFAULT_DB_FILE                         "db/bsp.db"
#ifdef ENABLE_STANDALONE
    #define RUNTIME_SETTING_FILE                "etc/runtime-setting.conf"
    #define BOOTSTRAP_FILE                      "etc/bootstrap.lua"
#else
    #define MGR_CMD_SHUTDOWN                    0xFFF0
    #define MGR_CMD_LOG                         0x3001
    #define MGR_CMD_LOG_LEVEL                   0x3002
    #define MGR_CMD_BAN_IP                      0x3003
    #define MGR_CMD_LIFT_IP                     0x3004
    #define MGR_I_RUNTIME_SETTING               0x4001
    #define MGR_M_RUNTIME_SETTING               0x4002
    #define MGR_I_BOOTSTRAP                     0x4011
    #define MGR_M_BOOTSTRAP                     0x4012
    #define MGR_I_STATUS                        0x5001
    #define MGR_M_STATUS                        0x5002
    #define MGR_DATA_LOG                        0xA011
#endif

#ifdef ENABLE_BSP_SPIN
    #define BSP_SPINLOCK_INITIALIZER            {._lock = SPIN_FREE, ._loop_times = 0}
#else
    #define BSP_SPINLOCK_INITIALIZER            1
#endif
#define MAX_SERVER_PER_CREATION                 128

/* Macros */

/* Functions */
#ifdef ENABLE_MEMPOOL
// With BSP.Mempool
    #define bsp_malloc(size)                    mempool_alloc(size)
    #define bsp_calloc(nmemb, size)             mempool_calloc(nmemb, size)
    #define bsp_realloc(ptr, size)              mempool_realloc(ptr, size)
    #define bsp_strdup(input)                   mempool_strdup(input)
    #define bsp_strndup(input, len)             mempool_strndup(input, len)
    #define bsp_malloc_usable_size(ptr)         mempool_alloc_usable_size(ptr)
    #define bsp_free(ptr)                       mempool_free(ptr)
#else
// General memory allocator
    #define bsp_malloc(size)                    malloc(size)
    #define bsp_calloc(nmemb, size)             calloc(nmemb, size)
    #define bsp_realloc(ptr, size)              realloc(ptr, size)
    #define bsp_strdup(input)                   strdup(input)
    #define bsp_strndup(input, len)             strndup(input, len)
    #define bsp_malloc_usable_size(ptr)         malloc_usable_size(ptr)
    #define bsp_free(ptr)                       free(ptr)
#endif

#ifdef ENABLE_BSP_SPIN
// With BSP.Spinlock
    #define bsp_spin_init(lock)                 spin_init(lock)
    #define bsp_spin_lock(lock)                 spin_lock(lock)
    #define bsp_spin_unlock(lock)               spin_unlock(lock)
    #define bsp_spin_destroy(lock)              spin_destroy(lock)
#else
// Pthread spinlock
    #define bsp_spin_init(lock)                 pthread_spin_init(lock, 0)
    #define bsp_spin_lock(lock)                 pthread_spin_lock(lock)
    #define bsp_spin_unlock(lock)               pthread_spin_unlock(lock)
    #define bsp_spin_destroy(lock)              pthread_spin_destroy(lock)
#endif

#endif  /* _LIB_BSP_CORE_H */
