/*
 * bsp-server.c
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
 * Main server program.
 * 
 * @package bsp::bsp-server
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/23/2012
 * @changelog 
 *      [06/04/2012] - Creation
 *      [08/14/2012] - BSP.Packet protocol
 *      [10/23/2012] - Terminal reload action
 *      [07/15/2013] - Recode
 *      [12/25/2013] - Trace level rearranged
 */

#define _GNU_SOURCE

#include "bsp-server.h"
#include "sched.h"

// Signals
static void _app_on_exit()
{
    // Call script
    //script_close();
    // Close log
    log_close();
    // Close status
    status_close();
    
    return;
}

static void _app_on_tstp()
{
    // Reload script
    //script_try_load();
    return;
}

static void _app_on_usr1()
{
    // Reopen log file
    log_open();
    return;
}

static void _app_on_usr2()
{
    // Debug status
    debug_status();
    return;
}

static void _usage()
{
    fprintf(stderr, "BSP-Server, core version : %s\n", get_core_version());
    fprintf(stderr, "===============================================================================\n\n");
    fprintf(stderr, "USAGE : bsp-server [-c Configure filename][-p PID filename][-a AppID][-d][-v][-h]\n");
    fprintf(stderr, "\t-d : Daemonize server. (background silent mode)\n");
    fprintf(stderr, "\t-v : Verbose mode, (display or record more message for debug)\n");
	fprintf(stderr, "\t-s : Silent mode. (display or record no debug message)\n");
    fprintf(stderr, "\t-h : Display this topic.\n\n");
    
    return;
}

/* Server callback */
static void main_server(BSP_CLIENT *clt, int callback, int cmd, void *data, ssize_t len)
{
    if (!clt)
    {
        return;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    BSP_SERVER *srv = get_client_connected_server(clt);
    if (!srv || !settings->script_callback_entry)
    {
        return;
    }
    
    switch (callback)
    {
        case SERVER_CALLBACK_ON_CONNECT : 
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_OSTRING, -1, srv->name}, 
                    {CALL_PTYPE_OSTRING, -1, "connect"}, 
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                script_call(clt->script_stack.stack, settings->script_callback_entry, p);
            }
            break;
        case SERVER_CALLBACK_ON_CLOSE : 
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_OSTRING, -1, srv->name}, 
                    {CALL_PTYPE_OSTRING, -1, "close"}, 
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                script_call(clt->script_stack.stack, settings->script_callback_entry, p);
            }
            break;
        case SERVER_CALLBACK_ON_DATA_RAW : 
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_OSTRING, -1, srv->name}, 
                    {CALL_PTYPE_OSTRING, -1, "raw"}, 
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_OSTRING, len, data}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                script_call(clt->script_stack.stack, settings->script_callback_entry, p);
            }
            break;
        case SERVER_CALLBACK_ON_DATA_OBJ : 
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_OSTRING, -1, srv->name}, 
                    {CALL_PTYPE_OSTRING, -1, "obj"}, 
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_OBJECT, 0, data}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                script_call(clt->script_stack.stack, settings->script_callback_entry, p);
            }
            break;
        case SERVER_CALLBACK_ON_DATA_CMD : 
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_OSTRING, -1, srv->name}, 
                    {CALL_PTYPE_OSTRING, -1, "cmd"}, 
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_INTEGER, 0, &cmd}, 
                    {CALL_PTYPE_OBJECT, 0, data}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                script_call(clt->script_stack.stack, settings->script_callback_entry, p);
            }
            break;
        default : 
            break;
    }
    
    return;
}

// Main timer event
static void _server_dida(unsigned long long timer)
{
    // Clean fds
    int fd;
    int fd_type;
    time_t now = time(NULL);
    void *ptr = NULL;
    BSP_CLIENT *clt = NULL;
    BSP_SERVER *srv = NULL;
    // Kick corpses
    for (fd = timer % HEARTBEAT_CHECK_RATE; fd <= get_max_fd(); fd += HEARTBEAT_CHECK_RATE)
    {
        fd_type = FD_TYPE_SOCKET_CLIENT;
        ptr = get_fd(fd, &fd_type);
        if (ptr)
        {
            clt = (BSP_CLIENT *) ptr;
            srv = get_client_connected_server(clt);
            if (srv->heartbeat_check > 0 && (now - clt->last_hb_time > srv->heartbeat_check))
            {
                // Remove client
                free_client(clt);
                flush_socket(&SCK(clt));
            }
        }
    }

    // Garbage-Collection
    if (!SCRIPT_GC_RATE)
    {
        return;
    }
    
    BSP_CORE_SETTING *settings = get_core_setting();
    if (0 == (timer + 1) % (SCRIPT_GC_RATE * 10))
    {
        trigger_gc(MAIN_THREAD);
    }
    
    if (0 == (timer % SCRIPT_GC_RATE))
    {
        trigger_gc((timer / SCRIPT_GC_RATE) % settings->static_workers);
    }
    
    return;
}

/* Main portal */
int main(int argc, char **argv)
{
    int c;
    int is_daemonize = 0;
    int is_verbose = 0;
	int is_silent = 0;
    int instance_id = INSTANCE_UNKNOWN;
    char *prefix = NULL;
    
    while (-1 != (c = getopt(argc, argv, "hdvsi:")))
    {
        switch (c)
        {
            case 'h' : 
                // Help topic
                _usage();
                return BSP_RTN_SUCCESS;
            case 'd' :
                // Daemonize
                is_daemonize = 1;
                break;
            case 'v' : 
                // Output more debug information
                is_verbose = 1;
                break;
            case 's' : 
                // Output no debug information (slient mode)
                is_silent = 1;
                break;
            default : 
                // Do nothing
                break;
        }
    }
#ifdef BSP_PERFIX_DIR
    prefix = BSP_PERFIX_DIR;
#else
    prefix = get_dir();
#endif
#ifndef ENABLE_STANDALONE
    is_daemonize = 1;
#endif
    debug_str("BS.Play generic server started");
    
    // Current working directory
    set_dir((const char *) prefix);
    BSP_CORE_SETTING *settings = get_core_setting();
    settings->max_fds = maxnium_fds();
    settings->instance_id = instance_id;
    settings->trace_level = is_verbose ? TRACE_LEVEL_ALL : TRACE_LEVEL_CORE | TRACE_LEVEL_FATAL | TRACE_LEVEL_ERROR;
    if (is_silent)
    {
        settings->trace_level = TRACE_LEVEL_NONE;
    }
    settings->trace_printer = (is_daemonize) ? NULL : trace_output;
    settings->is_daemonize = is_daemonize;
#ifdef ENABLE_STANDALONE
    settings->on_proc_exit = _app_on_exit;
    settings->on_proc_tstp = _app_on_tstp;
    settings->on_proc_usr1 = _app_on_usr1;
    settings->on_proc_usr2 = _app_on_usr2;
#endif
    
    // Try Huge-TLB
    status_init();
    enable_large_pages();
#ifdef ENABLE_MEMPOOL
    mempool_init();
#endif
    BSP_SERVER_CALLBACK sc[] = {
        {"main_server", NULL, main_server}, 
        {NULL, NULL, NULL}
    };
    core_init(sc);
    core_loop(_server_dida);
    
    return BSP_RTN_SUCCESS;
}
