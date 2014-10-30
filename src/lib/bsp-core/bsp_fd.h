/*
 * bsp_fd.h
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
 * File descriptors' manager, header file
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/07/2012
 * @changelog 
 *      [05/30/2012] - Creation
 *      [06/07/2012] - Fd's tid property added
 */

#ifndef _LIB_BSP_CORE_FD_H

#define _LIB_BSP_CORE_FD_H
/* Headers */

/* Definations */
#define FD_TYPE_ANY                             0
#define FD_TYPE_GENERAL                         1
#define FD_TYPE_PIPE                            2
#define FD_TYPE_EPOLL                           3
#define FD_TYPE_EVENT                           4
#define FD_TYPE_SIGNAL                          5
#define FD_TYPE_TIMER                           6
#define FD_TYPE_LOG                             7
#define FD_TYPE_SOCKET_SERVER                   11
#define FD_TYPE_SOCKET_CONNECTOR                12
#define FD_TYPE_SOCKET_CLIENT                   13
#define FD_TYPE_SOCKET_MYSQL                    21
#define FD_TYPE_SOCKET_REDIS                    31
#define FD_TYPE_SHM                             253
#define FD_TYPE_EXIT                            254
#define FD_TYPE_UNKNOWN                         255

#define HARD_LIMIT_FDS                          1048576
#define SAFE_LIMIT_FDS                          1024

/* Macros */

/* Structs */
typedef struct bsp_fd_t
{
    int                 fd;
    int                 type;
    int                 tid;
    void                *ptr;
    BSP_ONLINE          *online;
} BSP_FD;

/* Functions */
// Init fd list
int fd_init(size_t nfds);

// Register to list
int reg_fd(const int fd, const int type, void *ptr);

// Unregister from list
int unreg_fd(const int fd);

// Get fd info
// If a fd registered in list, its ptr will returned.
// If type set as FD_TYPE_ANY, the fd's type will be set back to the pointer, or just find the fd only matching the type argument.
void * get_fd(const int fd, int *type);

// Set current worker thread id
void set_fd_thread(const int fd, const int tid);

// Get worker thread id
int get_fd_thread(const int fd);

// Set fd online info
void set_fd_online(const int fd, BSP_ONLINE *online);

// Get fd online info
BSP_ONLINE * get_fd_online(const int fd);

// Set fd non-blocking
int set_fd_nonblock(const int fd);

// Get max fd
int get_max_fd(void);

// Maxnium fds
int maxnium_fds(void);

#endif  /* _LIB_BSP-CORE_FD_H */
