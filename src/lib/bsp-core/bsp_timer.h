/*
 * bsp_timer.h
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
 * Timer task header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/07/2012
 * @changelog 
 *      [06/07/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_TIMER_H

#define _LIB_BSP_CORE_TIMER_H
/* Headers */
//#include <sys/timerfd.h>

/* Definations */

/* Macros */

/* Structs */
typedef struct bsp_timer_t
{
    int                 fd;
    struct itimerspec   tm;
    struct epoll_event  ev;
    void                (* on_timer) (unsigned long long);
    void                (* on_stop) (unsigned long long);
    uint64_t            timer;
    uint64_t            loop;

    // Script runner
    BSP_SCRIPT_STACK    script_stack;
} BSP_TIMER;

/* Functions */
// Create a new timer
// sec for second and nsec for microsecond.
// If loop is negative, the timer will be a infinite loop, if greater than 0, after [loop] times, the timer will self-deleted.
// cb for the ON-TIMER callback function and arg for the callback argument.
BSP_TIMER * new_timer(time_t it_sec, long it_nsec, uint64_t loop);
int free_timer(BSP_TIMER *tmr);
void start_timer(BSP_TIMER *tmr);
void stop_timer(BSP_TIMER *tmr);

#endif  /* _LIB_BSP_CORE_TIMER_H */
