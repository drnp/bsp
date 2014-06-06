/*
 * spinlock.c
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
 * A simple spinlock, operate on a 8-bit variable
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/06/2012
 * @changelog 
 *      [06/06/2012] - Creation
 *      [12/18/2012] - Two-Bytes spinlock
 *      [05/21/2014] - Combined into bsp.h
 */

#include "bsp.h"
#ifdef ENABLE_SPIN
struct timespec ts = {0, 500000};
static inline uint8_t _spin_cas(uint8_t compare, uint8_t val, uint8_t *lock)
{
    uint8_t ret;
    __asm__ __volatile__("lock; cmpxchgb %2, (%3);"
                         "setz %0;"
                         : "=r" (ret)
                         : "a" (compare), 
                           "q" (val), 
                           "r" (lock)
                         : "memory", "flags");
    return ret;
}

static inline void _spin_sleep(BSP_SPINLOCK *lock)
{
    if (!lock)
    {
        return;
    }
    
    if ((lock->_loop_times & 0xF) == 0)
    {
        nanosleep(&ts, NULL);
        lock->_loop_times = 0;
    }

    else
    {
        __asm__ __volatile__("pause");
    }

    lock->_loop_times ++;

    return;
}

static inline uint8_t _spin_trylock(BSP_SPINLOCK *lock)
{
    if (!lock)
    {
        return 0;
    }
    
    return _spin_cas(SPIN_FREE, SPIN_LOCKED, &lock->_lock);
}

// Initialize a spinlock
void spin_init(BSP_SPINLOCK *lock)
{
    if (lock)
    {
        lock->_lock = SPIN_FREE;
        lock->_loop_times = 0;
    }
}

// Try lock a spin
void spin_lock(BSP_SPINLOCK *lock)
{
    if (!lock)
    {
        return;
    }
    
    do
    {
        while (lock->_lock != SPIN_FREE)
        {
            __asm__ __volatile__("" : : : "memory");
            // Sleep
            _spin_sleep(lock);
        }
    } while (!_spin_trylock(lock));

    lock->_loop_times = 0;
    
    return;
}

// Try unlock a spin
void spin_unlock(BSP_SPINLOCK *lock)
{
    if (!lock)
    {
        return;
    }

    __asm__ __volatile__("" : : : "memory");
    lock->_lock = SPIN_FREE;
    
    return;
}

// We do nothing here, just a interface corresponding to pthread_spinlock
void spin_destroy(BSP_SPINLOCK *lock)
{
    // Do nothing

    return;
}
#endif
