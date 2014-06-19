/*
 * bsp_thread.h
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
 * Static worker pool and dynamic task threads header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/15/2012
 * @changelog 
 *      [06/04/2012] - Creation
 *      [06/15/2012] - modify_fd_events method added
 */

#ifndef _LIB_BSP_CORE_THREAD_H

#define _LIB_BSP_CORE_THREAD_H
/* Headers */

/* Definations */
#define DEFAULT_STATIC_WORKERS                  8
#define FREE_TASK_LIST_INITIAL                  1024
#define MAIN_THREAD                             -1
#define UNBOUNDED_THREAD                        -2
#define STATIC_WORKER                           -3
#define MAIN_THREAD_PID                         -1

/* Macros */

/* Structs */
struct bsp_thread_task_t
{
    int                 idx;
    int                 keep;
    BSP_SCRIPT_CALL_PARAM
                        *params;
    struct bsp_thread_task_t
                        *next;
};

typedef struct bsp_thread_t
{
    int                 id;
    pthread_t           pid;
    int                 loop_fd;
    int                 notify_fd;
    int                 exit_fd;
    size_t              nfds;
    BSP_SPINLOCK        fd_lock;

    // Critical
    BSP_SCRIPT_STATE    script_runner;
    char                read_block[READ_ONCE];
} BSP_THREAD;

struct bsp_dispatch_task_t
{
    int                 fd;
    struct bsp_dispatch_task_t
                        *next;
};

typedef struct bsp_dispacher_t
{
    struct bsp_dispatch_task_t
                        *head;
    struct bsp_dispatch_task_t
                        *tail;
} BSP_DISPATCHER;

/* Functions */
// Initialization
int thread_init();

// Static thread loop
void * thread_process(void *arg);

// Add a fd to thread, if tid < 0, the most idle (has the least file descriptor) worker will be selected
int dispatch_to_thread(const int fd, int tid);

// Remove a fd from thread
int remove_from_thread(const int fd);

// Modify fd's listening event in epoll
int modify_fd_events(const int fd, struct epoll_event *ev);

// Set main server looper
//void set_main_loop_fd(int fd);

// Trigger script garbage-collection
int trigger_gc(int tid);

// Stop all static worker
void stop_workers(void);

// Get thread by givven index
BSP_THREAD * get_thread(int i);

// Current thread
BSP_THREAD * curr_thread();
int curr_thread_id();

#endif  /* _LIB_BSP_CORE_THREAD_H */
