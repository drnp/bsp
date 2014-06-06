/*
 * fd.c
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
 * File descriptors' manager
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/07/2012
 * @changelog 
 *      [05/30/2012] - Creation
 *      [06/07/2012] - Fd's tid property added
 */

#include "bsp.h"

#include <sys/resource.h>

BSP_FD *fd_list = NULL;
size_t fd_list_size = 0;
int max_fd = -1;
BSP_SPINLOCK fd_lock;

// Init fd list
int fd_init(size_t nfds)
{
    BSP_CORE_SETTING *settings = get_core_setting();
    
    if (0 == nfds)
    {
        nfds = settings->max_fds;
    }
    
    if (fd_list)
    {
        bsp_free(fd_list);
    }
    
    BSP_FD *tmp = bsp_malloc(sizeof(BSP_FD) * nfds);
    if (tmp)
    {
        memset(tmp, 0, sizeof(BSP_FD) * nfds);
        fd_list = tmp;
        fd_list_size = nfds;
        trace_msg(TRACE_LEVEL_DEBUG, "FileDs : File descriptor list inited as size %d", nfds);
        status_op_fd(STATUS_OP_FD_TOTAL, fd_list_size);
        bsp_spin_init(&fd_lock);
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "FileDs : File descriptor list alloc error");
        exit(BSP_RTN_ERROR_MEMORY);
    }
    
    return BSP_RTN_SUCCESS;
}

// Register to list
int reg_fd(const int fd, const int type, void *ptr)
{
    if (0 == fd_list_size || NULL == fd_list)
    {
        // List uninitialized
        fd_init(0);
    }
    
    if (fd >= 0 && fd < fd_list_size)
    {
        bsp_spin_lock(&fd_lock);
        fd_list[fd].fd = fd;
        fd_list[fd].type = type;
        fd_list[fd].tid = UNBOUNDED_THREAD;
        fd_list[fd].ptr = ptr;
        status_op_fd(STATUS_OP_FD_REG, 0);
        trace_msg(TRACE_LEVEL_VERBOSE, "FileDs : FD %d registed as type %d", fd, type);
        
        if (fd > max_fd)
        {
            status_op_fd(STATUS_OP_FD_MAX, (size_t) fd);
            max_fd = fd;
        }
        bsp_spin_unlock(&fd_lock);
    }
    
    return BSP_RTN_SUCCESS;
}

// Unregister from list
int unreg_fd(const int fd)
{
    if (fd >= 0 && fd < fd_list_size)
    {
        bsp_spin_lock(&fd_lock);
        fd_list[fd].fd = 0;
        fd_list[fd].type = 0;
        fd_list[fd].tid = UNBOUNDED_THREAD;
        fd_list[fd].ptr = NULL;
        status_op_fd(STATUS_OP_FD_UNREG, 0);
        trace_msg(TRACE_LEVEL_VERBOSE, "FileDs : FD %d unregisted from list", fd);
        
        if (fd >= max_fd)
        {
            int i;
            for (i = fd - 1; i >= 0; i --)
            {
                if (fd_list[i].fd == i)
                {
                    max_fd = i;
                    break;
                }
            }
        }
        bsp_spin_unlock(&fd_lock);
    }
    
    // Try close
    close(fd);
    
    return BSP_RTN_SUCCESS;
}

// Get fd info
void * get_fd(const int fd, int *type)
{
    void *ptr = NULL;
    if (fd >= 0 && fd < fd_list_size && fd_list[fd].fd == fd)
    {
        if (*type != FD_TYPE_ANY)
        {
            // Appointed fd type
            if (fd_list[fd].type == *type)
            {
                ptr = fd_list[fd].ptr;
                trace_msg(TRACE_LEVEL_VERBOSE, "FileDs : Get appointed FD %d, type %d", fd, *type);
            }
        }
        else
        {
            *type = fd_list[fd].type;
            ptr = fd_list[fd].ptr;
        }
    }
    else
    {
        // Not in list
        *type = FD_TYPE_UNKNOWN;
    }
    
    return ptr;
}

// Set current worker thread id
void set_fd_thread(const int fd, const int tid)
{
    if (fd >= 0 && fd < fd_list_size && fd_list[fd].fd == fd)
    {
        fd_list[fd].tid = tid;
        trace_msg(TRACE_LEVEL_VERBOSE, "FileDs : Set FD %d's thread to %d", fd, tid);
    }
    
    return;
}

// Get worker thread id
int get_fd_thread(const int fd)
{
    if (fd >= 0 && fd < fd_list_size && fd_list[fd].fd == fd)
    {
        return fd_list[fd].tid;
    }
    
    return UNBOUNDED_THREAD;
}

// Set fd non-blocking
int set_fd_nonblock(const int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        trace_msg(TRACE_LEVEL_ERROR, "FileDs : Get FD %d optional error", fd);
        return BSP_RTN_ERROR_IO;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        trace_msg(TRACE_LEVEL_ERROR, "FileDs : Set FD %d optional error", fd);
        return BSP_RTN_ERROR_IO;
    }
    
    trace_msg(TRACE_LEVEL_DEBUG, "FileDs : Set FD %d as non-blocking mode", fd);
    
    return BSP_RTN_SUCCESS;
}

int get_max_fd()
{
    return max_fd;
}

// Maxnium fds
int maxnium_fds()
{
    struct rlimit rlim, rlim_new;
    int old_maxfiles = 0;
    
    if (0 == getrlimit(RLIMIT_CORE, &rlim))
    {
        // Increase RLIMIT_CORE to infinity if possible
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        
        if (0 != setrlimit(RLIMIT_CORE, &rlim_new))
        {
            // Set rlimit error
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            (void) setrlimit(RLIMIT_CORE, &rlim_new);
        }
    }
    
    if (0 != getrlimit(RLIMIT_CORE, &rlim) || rlim.rlim_cur == 0)
    {
        trace_msg(TRACE_LEVEL_ERROR, "FileDs : GetRLimit CORE error");
        
        return -1;
    }
    
    // Read current RLIMIT_NOFILE
    if (0 != getrlimit(RLIMIT_NOFILE, &rlim))
    {
        trace_msg(TRACE_LEVEL_ERROR, "FileDs : GetRLimit NOFILE error");
        
        return -1;
    }
    else
    {
        // Enlarge RLIMIT_NOFILE to allow as many connections as we need
        old_maxfiles = rlim.rlim_max;
        
        if (rlim.rlim_cur < HARD_LIMIT_FDS)
        {
            rlim.rlim_cur = HARD_LIMIT_FDS;
        }
        
        if (rlim.rlim_max < rlim.rlim_cur)
        {
            if (0 == getuid() || 0 == geteuid())
            {
                // You are root?
                rlim.rlim_max = rlim.rlim_cur;
                trace_msg(TRACE_LEVEL_DEBUG, "FileDs : ROOT privilege, try to set RLimit.max to %d", rlim.rlim_cur);
            }
            else
            {
                // You are not root?
                rlim.rlim_cur = rlim.rlim_max;
                trace_msg(TRACE_LEVEL_DEBUG, "FileDs : NON-ROOT privilege, try to set RLimit.cur to %d", rlim.rlim_max);
            }
        }
        
        if (0 != setrlimit(RLIMIT_NOFILE, &rlim))
        {
            // Rlimit set error
            trace_msg(TRACE_LEVEL_DEBUG, "FileDs : Try to set RLimit NOFILE to %d error, you can decrease HARD_LIMIT_FDS, now try to set NOFILE to SAFE_LIMIT_FDS (%d)", rlim.rlim_max, SAFE_LIMIT_FDS);
            rlim.rlim_cur = SAFE_LIMIT_FDS;
            rlim.rlim_max = old_maxfiles;
            if (rlim.rlim_max < rlim.rlim_cur)
            {
                if (0 == getuid() || 0 == geteuid())
                {
                    // You are root?
                    rlim.rlim_max = rlim.rlim_cur;
                    trace_msg(TRACE_LEVEL_DEBUG, "FileDs : ROOT privilege, try to set RLimit.max to %d", rlim.rlim_cur);
                }
                else
                {
                    // You are not root?
                    rlim.rlim_cur = rlim.rlim_max;
                    trace_msg(TRACE_LEVEL_DEBUG, "FileDs : NON-ROOT privilege, try to set RLimit.cur to %d", rlim.rlim_max);
                }
            }
            
            if (0 != setrlimit(RLIMIT_NOFILE, &rlim))
            {
                trace_msg(TRACE_LEVEL_ERROR, "FileDs : Try to set RLimit NOFILE to %d failed", rlim.rlim_max);
                rlim.rlim_max = old_maxfiles;
            }
        }
    }
    
    trace_msg(TRACE_LEVEL_CORE, "FileDs : Set RLimit NOFILE to %d", rlim.rlim_max);
    
    return rlim.rlim_max;
}
