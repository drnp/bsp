/*
 * bsp-manager.c
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
 * BSP controller center
 * 
 * @package bin::bsp-manager
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/23/2012
 * @changelog 
 *      [07/23/2012] - Creation
 *      [10/10/2012] - Rebuild
 */

#define _GNU_SOURCE

#include "bsp-manager.h"

// Status debug
static void _app_on_usr2()
{
    // Debug status
    debug_status();
    return;
}

// Helo topics
static void _usage()
{
    fprintf(stderr, "BSP-Server, core version : %s\n", get_core_version());
    fprintf(stderr, "===============================================================================\n\n");
    fprintf(stderr, "\t-p : Set Management service listening port.      (Default : %d)\n", DEFAULT_MANAGER_PORT);
    fprintf(stderr, "\t-a : Set Management service listening address.   (Default : %s)\n", DEFAULT_MANAGER_ADDR);
    fprintf(stderr, "\t-d : Daemonize server. (background silent mode)\n");
    fprintf(stderr, "\t-v : Verbose mode, display more message for debug.\n");
    fprintf(stderr, "\t-s : Enable Flash sandbox service, listened on port %d.\n", AS3_SANDBOX_PORT);
    fprintf(stderr, "\t-h : Display this topic.\n\n");
    
    return;
}

/* Server callback */
static void _main_server_events(BSP_CLIENT *clt, int callback, int cmd, void *data, ssize_t len)
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

    switch (callback)
    {
        case SERVER_CALLBACK_ON_CONNECT : 
            break;
        case SERVER_CALLBACK_ON_CLOSE : 
            break;
        case SERVER_CALLBACK_ON_ERROR : 
            break;
        case SERVER_CALLBACK_ON_DATA_RAW : 
            break;
        case SERVER_CALLBACK_ON_DATA_OBJ : 
            break;
        case SERVER_CALLBACK_ON_DATA_CMD : 
            break;
        default : 
            break;
    }
    
    return;
}

static void _channel_server_events(BSP_CLIENT *clt, int callback, int cmd, void *data, ssize_t len)
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

    switch (callback)
    {
        case SERVER_CALLBACK_ON_CONNECT : 
            break;
        case SERVER_CALLBACK_ON_CLOSE : 
            break;
        case SERVER_CALLBACK_ON_ERROR : 
            break;
        case SERVER_CALLBACK_ON_DATA_RAW : 
            break;
        case SERVER_CALLBACK_ON_DATA_OBJ : 
            break;
        case SERVER_CALLBACK_ON_DATA_CMD : 
            break;
        default : 
            break;
    }
    
    return;
}

static void _sandbox_server_events(BSP_CLIENT *clt, int callback, int cmd, void *data, ssize_t len)
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

    const char *input = (const char *) data;
    switch (callback)
    {
        case SERVER_CALLBACK_ON_DATA_RAW : 
            if (!input)
            {
                return;
            }
            if (len > 10 && 0 == strncmp("<policy", input, 7))
            {
                // Valid sandbox request
                append_data_socket(&SCK(clt), AS3_SANDBOX_CONTENT, -1);
            }
            else
            {
                // Invalid
                append_data_socket(&SCK(clt), "<cross-domain-policy>INVALID</cross-domain-policy>", -1);
            }

            flush_socket(&SCK(clt));
            free_client(clt);
            break;
        default : 
            // Ignore other events
            break;
    }
    
    return;
}

/* Main portal */
int main(int argc, char **argv)
{
    int c;
    int is_daemonize = 0;
    int is_verbose = 0;
    int enable_sandbox = 0;
    int port = DEFAULT_MANAGER_PORT;
    char *addr = DEFAULT_MANAGER_ADDR;
    char *prefix = NULL;

    while (-1 != (c = getopt(argc, argv, "hdvsp:a:")))
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
                // Enable AS3 sandbox
                enable_sandbox = 1;
                break;
            case 'p' : 
                // Manager port
                port = atoi(optarg);
                break;
            case 'a' : 
                // Manager address
                addr = strdup(optarg);
                break;
            default : 
                break;
        }
    }

#ifdef BSP_PERFIX_DIR
    prefix = BSP_PERFIX_DIR;
#else
    prefix = get_dir();
#endif
    debug_str("BS.Play manager server started");
    BSP_CORE_SETTING *settings = get_core_setting();
    settings->max_fds = maxnium_fds();
    settings->trace_level = is_verbose ? TRACE_LEVEL_ALL : TRACE_LEVEL_CORE | TRACE_LEVEL_FATAL | TRACE_LEVEL_ERROR;
    settings->trace_printer = (is_daemonize) ? NULL : trace_output;
    settings->is_daemonize = is_daemonize;
    settings->on_proc_usr2 = _app_on_usr2;

    // Try Huge-TLB
    status_init();
    enable_large_pages();
    mempool_init();

    core_init(0);
    set_dir((const char *) prefix);
    save_pid(MANAGER_PID_FILE);
    
    /* Add servers */
    int srv_fds[16];
    int nfds, i, fd_type;
    BSP_SERVER *srv = NULL;

    nfds = 16;
    nfds = new_server(addr, port, INET_TYPE_ANY, SOCK_TYPE_TCP, srv_fds, &nfds);
    for (i = 0; i < nfds; i ++)
    {
        fd_type = FD_TYPE_SOCKET_SERVER;
        srv = (BSP_SERVER *) get_fd(srv_fds[i], &fd_type);
        if (srv)
        {
            srv->on_events = _main_server_events;
            srv->def_client_type = CLIENT_TYPE_DATA;
            srv->def_data_type = DATA_TYPE_PACKET;
            add_server(srv_fds[i]);
        }
    }

    if (enable_sandbox)
    {
        nfds = 16;
        nfds = new_server(NULL, AS3_SANDBOX_PORT, INET_TYPE_IPV6, SOCK_TYPE_TCP, srv_fds, &nfds);
        for (i = 0; i < nfds; i ++)
        {
            fd_type = FD_TYPE_SOCKET_SERVER;
            srv = (BSP_SERVER *) get_fd(srv_fds[i], &fd_type);
            if (srv)
            {
                srv->on_events = _sandbox_server_events;
                srv->def_client_type = CLIENT_TYPE_DATA;
                srv->def_data_type = DATA_TYPE_STREAM;
                add_server(srv_fds[i]);
            }
        }
    }

    // Start local sock service
    nfds = 1;
    nfds = new_server(CHANNEL_SOCK_FILE, 0666, INET_TYPE_LOCAL, SOCK_TYPE_TCP, srv_fds, &nfds);
    fd_type = FD_TYPE_SOCKET_SERVER;
    srv = (BSP_SERVER *) get_fd(srv_fds[0], &fd_type);
    if (!srv)
    {
        trace_msg(TRACE_LEVEL_FATAL, "Create local socket %s failed!", CHANNEL_SOCK_FILE);
        _exit(BSP_RTN_FATAL);
    }

    srv->on_events = _channel_server_events;
    srv->def_client_type = CLIENT_TYPE_DATA;
    srv->def_data_type = DATA_TYPE_PACKET;
    add_server(srv_fds[0]);

    start_loop();
    
    return BSP_RTN_SUCCESS;
}
