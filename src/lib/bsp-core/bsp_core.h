/*
 * bsp_core.h
 *
 * Copyright (C) 2013 - Dr.NP
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
 * BSP core runtime header
 * 
 * @package libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 03/29/2013
 * @changelog
 *      [03/29/2013] - Creation
 */

#ifndef _LIB_BSP_CORE_CORE_H

#define _LIB_BSP_CORE_CORE_H
/* Headers */

/* Definations */
#define MAX_MODULES                             1024

/* Macros */

/* Structs */
typedef struct bsp_core_setting_t
{
    int                 initialized;
    int                 instance_id;
    
    // Process
    int                 max_fds;
    int                 is_daemonize;
    int                 tcp_listen_backlog;
    int                 epoll_wait_conns;
    int                 static_workers;
    int                 trace_level;
    int                 udp_proto_main;
    int                 udp_proto_status;

    // Debug callback
    void                (* trace_recorder) (time_t timestamp, int level, const char *msg);
    void                (* trace_printer) (time_t timestamp, int level, const char *msg);

    // Process callback
    void                (* on_proc_exit) (int signo);
    void                (* on_proc_tstp) (int signo);
    void                (* on_proc_usr1) (int signo);
    void                (* on_proc_usr2) (int signo);

    // Server callback
    BSP_SERVER_CALLBACK *server_callback;

    // Main base (1Hz) timer callback
    BSP_TIMER           *main_timer;

    // Application base dir (prefix)
    const char          *base_dir;
    char                *mod_dir;
    char                *log_dir;
    char                *runtime_dir;
    char                *script_dir;
    char                *script_callback_entry;

    // Debug
    int                 debug_hex_connector_input;
    int                 debug_hex_output;
} BSP_CORE_SETTING;

/* Functions */
// Get a complete core configuration sets
// You can modify all the values anyway.
BSP_CORE_SETTING * get_core_setting();

// Get core version
char * get_core_version(void);

// Init
int core_init(BSP_SERVER_CALLBACK sc[]);

// Start main loop
int core_loop(void (* timer_event)(unsigned long long));

#endif  /* _LIB_BSP_CORE_CORE_H */
