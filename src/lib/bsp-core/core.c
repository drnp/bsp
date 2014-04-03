/*
 * core.c
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
 * BSP core runtime
 * 
 * @package libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 03/29/2013
 * @changelog
 *      [03/29/2013] - Creation
 */
#include "bsp.h"

BSP_CORE_SETTING core_settings = {.initialized = 0};

// Set default values
static void init_core_setting()
{
    int np = get_nprocs();
    int nw = np ? np * 2 : DEFAULT_STATIC_WORKERS;

    core_settings.initialized = 1;
    core_settings.app_id = -1;
    core_settings.max_fds = SAFE_LIMIT_FDS;
    core_settings.is_daemonize = 0;
    core_settings.tcp_listen_backlog = 1024;
    core_settings.epoll_wait_conns = 1024;
    core_settings.static_workers = nw;
    core_settings.trace_level = TRACE_LEVEL_NONE;
    core_settings.udp_proto_main = 0;
    core_settings.udp_proto_status = 0;
    core_settings.trace_recorder = NULL;
    core_settings.trace_printer = NULL;
    core_settings.on_proc_exit = NULL;
    core_settings.on_proc_tstp = NULL;
    core_settings.on_proc_usr1 = NULL;
    core_settings.on_proc_usr2 = NULL;
    core_settings.main_timer = NULL;

    core_settings.base_dir = NULL;
    core_settings.mod_dir = NULL;
    core_settings.log_dir = NULL;
    core_settings.runtime_dir = NULL;

    core_settings.debug_hex_connector_input = 0;
    core_settings.debug_hex_output = 0;

    return;
}

// Get a complete core configuration sets
BSP_CORE_SETTING * get_core_setting()
{
    if (!core_settings.initialized)
    {
        init_core_setting();
    }
    
    return &core_settings;
}

// Get core version
char * get_core_version()
{
    return BSP_CORE_VERSION;
}

// Init BSP-core
int core_init(int app_id)
{
    if (!core_settings.initialized)
    {
        init_core_setting();
    }

    core_settings.app_id = app_id;
    if (core_settings.is_daemonize)
    {
        proc_daemonize();
    }

    status_op_core(STATUS_OP_APPID, (size_t) app_id);
    signal_init();
    socket_init();
    fd_init(0);
    thread_init();
    log_init();
    
    return BSP_RTN_SUCCESS;
}
