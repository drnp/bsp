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
    script_close();
    // Close log
    log_close();
    // Close status
    status_close();
    
    return;
}

static void _app_on_tstp()
{
    // Reload script
    script_try_load();
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
    fprintf(stderr, "\t-c : Set configure filename.         (Default : %s)\n", DEFAULT_CONF_FILE);
    fprintf(stderr, "\t-p : Set process's PID filename.     (Default : %s)\n", DEFAULT_PID_FILE);
    fprintf(stderr, "\t-a : Set AppID, or bsp will read this value from configure file.\n");
    fprintf(stderr, "\t-d : Daemonize server. (background silent mode)\n");
    fprintf(stderr, "\t-v : Verbose mode, (display or record more message for debug)\n");
	fprintf(stderr, "\t-s : Silent mode. (display or record no debug message)\n");
    fprintf(stderr, "\t-i : Independence mode. (do not connect to manager server)\n");
    fprintf(stderr, "\t-h : Display this topic.\n\n");
    
    return;
}

/* Server callback */
static void _server_events(BSP_CLIENT *clt, int callback, int cmd, void *data, ssize_t len)
{
    if (!clt)
    {
        return;
    }

    BSP_SERVER *srv = get_client_connected_server(clt);
    if (!srv)
    {
        return;
    }

    struct bsp_conf_server_t *conf_srv = (struct bsp_conf_server_t *) srv->additional;
    if (!conf_srv)
    {
        return;
    }

    struct bsp_script_stack_t *ts = NULL;
    switch (callback)
    {
        case SERVER_CALLBACK_ON_CONNECT : 
            // Bind client to a new thread
            ts = script_new_stack(find_thread()->runner);
            if (ts)
            {
                clt->additional = (void *) ts;
            }
            if (conf_srv->script_func_on_connect)
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                (ts) ? script_caller_call_func(ts->stack, conf_srv->script_func_on_connect, p) : script_call_func(conf_srv->script_func_on_connect, p);
            }
            break;
        case SERVER_CALLBACK_ON_CLOSE : 
            ts = (struct bsp_script_stack_t *) clt->additional;
            if (conf_srv->script_func_on_close)
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                (ts) ? script_caller_call_func(ts->stack, conf_srv->script_func_on_close, p) : script_call_func(conf_srv->script_func_on_close, p);
            }
            // Remove stack
            if (ts)
            {
                script_remove_stack(ts);
                clt->additional = NULL;
            }
            break;
        case SERVER_CALLBACK_ON_DATA_RAW : 
            ts = (struct bsp_script_stack_t *) clt->additional;
            if (conf_srv->script_func_on_data)
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_OSTRING, len, data}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                (ts) ? script_caller_call_func(ts->stack, conf_srv->script_func_on_data, p) : script_call_func(conf_srv->script_func_on_data, p);
            }
            break;
        case SERVER_CALLBACK_ON_DATA_OBJ : 
            ts = (struct bsp_script_stack_t *) clt->additional;
            if (conf_srv->script_func_on_data)
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_OBJECT, 0, data}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                (ts) ? script_caller_call_func(ts->stack, conf_srv->script_func_on_data, p) : script_call_func(conf_srv->script_func_on_data, p);
            }
            break;
        case SERVER_CALLBACK_ON_DATA_CMD : 
            ts = (struct bsp_script_stack_t *) clt->additional;
            if (conf_srv->script_func_on_data)
            {
                BSP_SCRIPT_CALL_PARAM p[] = {
                    {CALL_PTYPE_INTEGER, 0, &SFD(clt)}, 
                    {CALL_PTYPE_INTEGER, 0, &cmd}, 
                    {CALL_PTYPE_OBJECT, 0, data}, 
                    {CALL_PTYPE_END, 0, NULL}
                };
                (ts) ? script_caller_call_func(ts->stack, conf_srv->script_func_on_data, p) : script_call_func(conf_srv->script_func_on_data, p);
            }
            break;
        default : 
            break;
    }
    
    return;
}

// Main timer event
void _server_dida(unsigned long long timer)
{
    // Clean fds
    int fd;
    int fd_type;
    time_t now = time(NULL);
    void *ptr = NULL;
    BSP_CLIENT *clt = NULL;
    BSP_SERVER *srv = NULL;
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
    return;
}

/* Main portal */
int main(int argc, char **argv)
{
    int c;
    int is_daemonize = 0;
    int is_verbose = 0;
	int is_silent = 0;
    int is_independence = 0;
    int app_id = 0;
    char *prefix = NULL;
    char *pid_file = NULL;
    char *conf_file = NULL;
    char *script_identifier = NULL;

    while (-1 != (c = getopt(argc, argv, "hdvsp:c:a:i")))
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
            case 'p' : 
                // PID file
                pid_file = strdup(optarg);
                break;
            case 'c' : 
                // Configure file
                conf_file = strdup(optarg);
                break;
            case 'a' : 
                // APP ID
                app_id = atoi(optarg);
                break;
            case 'i' : 
                // Independence mode
                is_independence = 1;
                break;
            default : 
                // Do nothing
                break;
        }
    }
    
    if (!pid_file)
    {
        pid_file = DEFAULT_PID_FILE;
    }
    if (!conf_file)
    {
        conf_file = DEFAULT_CONF_FILE;
    }
    
#ifdef BSP_PERFIX_DIR
    prefix = BSP_PERFIX_DIR;
#else
    prefix = get_dir();
#endif
    debug_str("BS.Play generic server started");

    // Current working directory
    set_dir((const char *) prefix);
    BSP_CORE_SETTING *settings = get_core_setting();
    settings->max_fds = maxnium_fds();
    settings->trace_level = is_verbose ? TRACE_LEVEL_ALL : TRACE_LEVEL_CORE | TRACE_LEVEL_FATAL | TRACE_LEVEL_ERROR;
    if (is_silent)
    {
        settings->trace_level = TRACE_LEVEL_NONE;
    }
    settings->trace_printer = (is_daemonize) ? NULL : trace_output;
    settings->is_daemonize = is_daemonize;
    settings->on_proc_exit = _app_on_exit;
    settings->on_proc_tstp = _app_on_tstp;
    settings->on_proc_usr1 = _app_on_usr1;
    settings->on_proc_usr2 = _app_on_usr2;

    // Try Huge-TLB
    status_init();
    enable_large_pages();
    mempool_init();
    
    BSP_CONF *conf = conf_init(conf_file);
    if (!app_id)
    {
        app_id = conf->conf_core.app_id;
    }

    script_identifier = conf->conf_core.script_identifier;

    // Core settings
    if (!script_identifier)
    {
        script_identifier = DEFAULT_SCRIPT_IDENTIFIER;
    }

    if (conf->conf_core.static_workers > 0)
    {
        settings->static_workers = conf->conf_core.static_workers;
    }

    settings->debug_hex_connector_input = conf->conf_core.debug_hex_connector_input;
    settings->debug_hex_output = conf->conf_core.debug_hex_output;
    if (conf->conf_core.log_dir && strlen(conf->conf_core.log_dir))
    {
        settings->log_dir = conf->conf_core.log_dir;
    }
    
    core_init(app_id);
    if (is_daemonize && !conf->conf_core.disable_log)
    {
        log_open();
        settings->trace_recorder = log_add;
    }

    // We should write PID file after core_init because daemonize will change current process ID
    save_pid(pid_file);

    // Connect to manager
    if (!is_independence)
    {
        BSP_CONNECTOR *cnt = new_connector(CHANNEL_SOCK_FILE, 0, INET_TYPE_LOCAL, SOCK_TYPE_TCP);
        if (!cnt)
        {
            trace_msg(TRACE_LEVEL_FATAL, "BSP    : Cannot connect to manager server");
            _exit(BSP_RTN_FATAL);
        }
    }

    /* Add servers */
    int srv_fds[16];
    int nfds, i, fd_type;
    BSP_SERVER *srv = NULL;

    struct bsp_conf_server_t *curr_srv = conf->conf_servers;
    while (curr_srv)
    {
        nfds = 16;
        nfds = new_server(curr_srv->server_addr, curr_srv->server_port, curr_srv->server_inet, curr_srv->server_sock, srv_fds, &nfds);
        for (i = 0; i < nfds; i ++)
        {
            fd_type = FD_TYPE_SOCKET_SERVER;
            srv = (BSP_SERVER *) get_fd(srv_fds[i], &fd_type);
            if (srv)
            {
                srv->additional = (void *) curr_srv;
                srv->on_events = _server_events;
                srv->heartbeat_check = curr_srv->heartbeat_check;
                srv->def_client_type = curr_srv->default_client_type;
                srv->def_data_type = curr_srv->default_data_type;
                srv->max_packet_length = curr_srv->max_packet_length;
                srv->max_clients = curr_srv->max_clients;
                add_server(srv_fds[i]);
            }
        }
        curr_srv = curr_srv->next;
    }
    script_init(NULL);
    script_set_identifier(script_identifier);
    if (conf->conf_core.script_func_on_load)
    {
        script_set_hook(SCRIPT_HOOK_LOAD, conf->conf_core.script_func_on_load);
    }
    if (conf->conf_core.script_func_on_reload)
    {
        script_set_hook(SCRIPT_HOOK_RELOAD, conf->conf_core.script_func_on_reload);
    }
    if (conf->conf_core.script_func_on_exit)
    {
        script_set_hook(SCRIPT_HOOK_EXIT, conf->conf_core.script_func_on_exit);
    }
    if (conf->conf_core.script_func_on_sub_load)
    {
        script_set_hook(SCRIPT_HOOK_SUB_LOAD, conf->conf_core.script_func_on_sub_load);
    }
    if (conf->conf_core.script_func_on_sub_reload)
    {
        script_set_hook(SCRIPT_HOOK_SUB_RELOAD, conf->conf_core.script_func_on_sub_reload);
    }
    if (conf->conf_core.script_func_on_sub_exit)
    {
        script_set_hook(SCRIPT_HOOK_SUB_EXIT, conf->conf_core.script_func_on_sub_exit);
    }

    struct bsp_conf_module_t *curr_mod = conf->conf_modules;
    while (curr_mod)
    {
        if (curr_mod->module_name)
        {
            script_load_module(curr_mod->module_name);
        }
        curr_mod = curr_mod->next;
    }
    script_try_load();

    // Let's GO!!!
    set_loop_event(SERVER_CALLBACK_ON_LOOP_TIMER, _server_dida);
    start_loop();
    
    return BSP_RTN_SUCCESS;
}
