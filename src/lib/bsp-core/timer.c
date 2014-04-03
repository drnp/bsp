/*
 * timer.c
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
 * Timer task
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/07/2012
 * @changelog 
 *      [06/07/2012] - Creation
 */

#include "bsp.h"

BSP_TIMER **free_timer_list = NULL;
size_t free_timer_list_size = 0;
size_t free_timer_total = 0;

// Create a new timer with event callback
BSP_TIMER * new_timer(time_t it_sec, long it_nsec, uint64_t loop)
{
    BSP_TIMER *tmr = NULL;
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    if (-1 == fd)
    {
        return NULL;
    }
    // Create new
    tmr = mempool_alloc(sizeof(BSP_TIMER));
    if (!tmr)
    {
        _exit(BSP_RTN_ERROR_MEMORY);
    }

    memset(tmr, 0, sizeof(BSP_TIMER));
    tmr->tm.it_value.tv_sec = it_sec;
    tmr->tm.it_value.tv_nsec = it_nsec;
    tmr->tm.it_interval.tv_sec = it_sec;
    tmr->tm.it_interval.tv_nsec = it_nsec;

    tmr->ev.events = EPOLLIN;
    tmr->ev.data.fd = fd;
    tmr->fd = fd;

    tmr->on_timer = NULL;
    tmr->on_stop = NULL;
    tmr->cb_arg = NULL;
    tmr->loop = loop;

    reg_fd(fd, FD_TYPE_TIMER, (void *) tmr);
    status_op_timer(STATUS_OP_TIMER_ADD);

    dispatch_to_worker(fd, -1);

    return tmr;
}

// Delete a timer
int free_timer(BSP_TIMER *tmr)
{
    if (!tmr)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    stop_timer(tmr);
    remove_from_worker(tmr->fd);
    unreg_fd(tmr->fd);
    status_op_timer(STATUS_OP_TIMER_DEL);
    mempool_free(tmr);
    
    return BSP_RTN_SUCCESS;
}

// Start timer
void start_timer(BSP_TIMER *tmr)
{
    if (!tmr)
    {
        return;
    }
    
    // Timerfd_Settime
    timerfd_settime(tmr->fd, 0, &tmr->tm, NULL);
    trace_msg(TRACE_LEVEL_DEBUG, "Timer  : Timer %d started", tmr->fd);
    
    return;
}

// Stop timer
void stop_timer(BSP_TIMER *tmr)
{
    if (!tmr)
    {
        return;
    }

    tmr->tm.it_value.tv_sec = 0;
    tmr->tm.it_value.tv_nsec = 0;
    tmr->tm.it_interval.tv_sec = 0;
    tmr->tm.it_interval.tv_nsec = 0;

    timerfd_settime(tmr->fd, 0, &tmr->tm, NULL);
    trace_msg(TRACE_LEVEL_DEBUG, "Timer  : Timer %d stoped", tmr->fd);

    return;
}
