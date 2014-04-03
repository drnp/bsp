/*
 * bsp-server.h
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
 * Main server header.
 * 
 * @package bsp::bsp-server
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/04/2012
 * @changelog 
 *      [06/04/2012] - Creation
 *      [10/09/2012] - PreInstall
 *      [01/15/2013] - UDP Protocol ID
 */

#ifndef _BIN_BSP_SERVER_H

#define _BIN_BSP_SERVER_H
/* Headers */
#include "bsp.h"

/* Definations */
#define DEFAULT_PID_FILE                        "/var/run/bsp-server.pid"
#define DEFAULT_CONF_FILE                       "etc/bsp-server.conf"
#define DEFAULT_LOG_FILE                        "log/bsp-server.log"
#define DEFAULT_SCRIPT_IDENTIFIER               "script/main.lua"
#define HEARTBEAT_CHECK_RATE                    60

#ifndef CHANNEL_SOCK_FILE
    #define CHANNEL_SOCK_FILE                       "/var/run/bsp-manager.sock"
#endif

/* Macros */

/* Structs */

/* Functions */

#endif  /* _BIN_BSP_SERVER_H */
