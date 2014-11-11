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

#ifdef ENABLE_STANDALONE
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
#endif

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
static void server_callback(BSP_CALLBACK *cb)
{
    BSP_OBJECT *p = new_object(OBJECT_TYPE_HASH);
    if (!cb || !cb->server || !cb->client || !cb->client->script_stack.stack || !p)
    {
        return;
    }

    if (SERVER_CALLBACK_ON_HEARTBEAT == cb->event)
    {
        // Ignore heartbeat
        return;
    }

    BSP_STRING *key;
    BSP_VALUE *val;
    BSP_SCRIPT_SYMBOL sym = {NULL, NULL, 0};

    key = new_string_const("server_name", -1);
    val = new_value();
    BSP_STRING *srv_name = new_string_const(cb->server->name, -1);
    value_set_string(val, srv_name);
    object_set_hash(p, key, val);

    key = new_string_const("client", -1);
    val = new_value();
    value_set_int(val, SFD(cb->client));
    object_set_hash(p, key, val);

    switch (cb->event)
    {
        case SERVER_CALLBACK_ON_ACCEPT : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("accept", -1));
            object_set_hash(p, key, val);
        case SERVER_CALLBACK_ON_CONNECT : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("connect", -1));
            object_set_hash(p, key, val);
            break;
        case SERVER_CALLBACK_ON_CLOSE : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("close", -1));
            object_set_hash(p, key, val);
            break;
        case SERVER_CALLBACK_ON_ERROR : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("error", -1));
            object_set_hash(p, key, val);
            break;
        case SERVER_CALLBACK_ON_DATA_RAW : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("raw", -1));
            object_set_hash(p, key, val);
            key = new_string_const("raw", -1);
            val = new_value();
            value_set_string(val, cb->stream);
            object_set_hash(p, key, val);
            break;
        case SERVER_CALLBACK_ON_DATA_OBJ : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("obj", -1));
            object_set_hash(p, key, val);
            key = new_string_const("obj", -1);
            val = new_value();
            value_set_object(val, cb->obj);
            object_set_hash(p, key, val);
            break;
        case SERVER_CALLBACK_ON_DATA_CMD : 
            key = new_string_const("event", -1);
            val = new_value();
            value_set_string(val, new_string_const("cmd", -1));
            object_set_hash(p, key, val);
            key = new_string_const("cmd", -1);
            val = new_value();
            value_set_int(val, cb->cmd);
            object_set_hash(p, key, val);
            key = new_string_const("params", -1);
            val = new_value();
            value_set_object(val, cb->obj);
            object_set_hash(p, key, val);
            break;
        default : 
            break;
    }

    if (cb->server->lua_entry && cb->client->script_stack.stack)
    {
        sym.func = cb->server->lua_entry;
        script_call(&cb->client->script_stack, &sym, p);
    }

    if (cb->server->fcgi_upstream)
    {
        debug_object(p);
        fcgi_call(cb->server->fcgi_upstream, p, &cb->client->sck.saddr);
    }

    del_object(p);

    return;
}

// Main timer event
static void _dida(BSP_TIMER * tmr)
{
    // Clean fds
    int fd;
    int fd_type;
    time_t now = time(NULL);
    void *ptr = NULL;
    BSP_CLIENT *clt = NULL;
    BSP_SERVER *srv = NULL;
    // Kick corpses
    for (fd = tmr->timer % HEARTBEAT_CHECK_RATE; fd <= get_max_fd(); fd += HEARTBEAT_CHECK_RATE)
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
    settings->ext_timer_callback = _dida;
#endif

    // Try Huge-TLB
    status_init();
    enable_large_pages();
#ifdef ENABLE_MEMPOOL
    mempool_init();
#endif

    core_init();
    core_loop(server_callback);

    return BSP_RTN_SUCCESS;
}
