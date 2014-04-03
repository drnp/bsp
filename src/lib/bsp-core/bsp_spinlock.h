/*
 * bsp_spinlock.h
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
 * Spinlock header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 09/03/2012
 * @changelog 
 *      [06/06/2012] - Creation
 *      [09/03/2012] - Initializer added
 *      [12/18/2012] - Two-Bytes spinlock
 */

#ifndef _LIB_BSP_CORE_SPINLOCK_H

#define _LIB_BSP_CORE_SPINLOCK_H
/* Headers */
#include <pthread.h>

/* Definations */
#define SPIN_FREE                               0
#define SPIN_LOCKED                             1
#ifdef ENABLE_BSP_SPIN
    #define BSP_SPINLOCK_INITIALIZER                {._lock = SPIN_FREE, ._loop_times = 0}
#else
    #define BSP_SPINLOCK_INITIALIZER                1
#endif

/* Macros */

/* Structs */
#ifdef ENABLE_BSP_SPIN
typedef struct bsp_spinlock_t
{
    uint8_t             _lock;
    uint8_t             _loop_times;
} BSP_SPINLOCK;
#else
typedef pthread_spinlock_t BSP_SPINLOCK;
#endif

/* Functions */
#ifdef ENABLE_BSP_SPIN
    void bsp_spin_init(BSP_SPINLOCK *lock);
    void bsp_spin_lock(BSP_SPINLOCK *lock);
    void bsp_spin_unlock(BSP_SPINLOCK *lock);
    void bsp_spin_destroy(BSP_SPINLOCK *lock);
#else
    #define bsp_spin_init(lock)                     pthread_spin_init(lock, 0)
    #define bsp_spin_lock(lock)                     pthread_spin_lock(lock)
    #define bsp_spin_unlock(lock)                   pthread_spin_unlock(lock)
    #define bsp_spin_destroy(lock)                  pthread_spin_destroy(lock)
#endif

#endif  /* _LIB_BSP_CORE_SPINLOCK_H */
