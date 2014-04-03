/*
 * bsp-manager.h
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
 * BSP stat client header
 * 
 * @package bin::bsp-manager
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/23/2012
 * @changelog 
 *      [07/23/2012] - Creation
 */

#ifndef _BIN_BSP_MANAGER_H

#define _BIN_BSP_MANAGER_H
/* Headers */
#include "bsp.h"

/* Definations */
#define DEFAULT_MANAGER_PORT                    65530
#define DEFAULT_MANAGER_ADDR                    "0.0.0.0"
#define MANAGER_PID_FILE                        "/var/run/bsp-manager.pid"
#define CHANNEL_SOCK_FILE                       "/var/run/bsp-manager.sock"

#define AS3_SANDBOX_PORT                        843
#define AS3_SANDBOX_CONTENT                     "<cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>"

// Commands

/* Macros */

/* Structs */

/* Functions */

#endif  /* _BIN_BSP_MANAGER_H */
