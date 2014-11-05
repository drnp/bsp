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
BSP_OBJECT *runtime_settings = NULL;

// Base timer event
static void base_timer(BSP_TIMER *tmr)
{
    if (!tmr)
    {
        return;
    }

    // Script garbage collection
    if (core_settings.script_gc_interval)
    {
        if (0 == (tmr->timer + 1) % (core_settings.script_gc_interval * 10))
        {
            trigger_gc(MAIN_THREAD);
        }

        if (0 == (tmr->timer % core_settings.script_gc_interval))
        {
            trigger_gc((tmr->timer / core_settings.script_gc_interval) % core_settings.static_workers);
        }
    }

    // Online autosave
    if (core_settings.online_autosave_interval)
    {
        if (0 == tmr->timer % core_settings.online_autosave_interval)
        {
            // Auto save
        }
    }

    // External callback
    if (core_settings.ext_timer_callback)
    {
        core_settings.ext_timer_callback(tmr);
    }

    return;
}

// Set default values
static void init_core_setting()
{
    int np = get_nprocs();
    int nw = np ? np * 2 : DEFAULT_STATIC_WORKERS;

    core_settings.initialized = 1;
    core_settings.instance_id = INSTANCE_UNKNOWN;
    core_settings.max_fds = SAFE_LIMIT_FDS;
    core_settings.is_daemonize = 0;
    core_settings.tcp_listen_backlog = 1024;
    core_settings.epoll_wait_conns = 1024;
    core_settings.static_workers = nw;
    core_settings.trace_level = TRACE_LEVEL_NONE;
    core_settings.udp_proto_main = 0;
    core_settings.udp_proto_status = 0;

    core_settings.on_srv_data = NULL;
    core_settings.on_srv_events = NULL;
    core_settings.trace_recorder = NULL;
    core_settings.trace_printer = NULL;
    core_settings.on_proc_exit = NULL;
    core_settings.on_proc_tstp = NULL;
    core_settings.on_proc_usr1 = NULL;
    core_settings.on_proc_usr2 = NULL;
    core_settings.main_timer = NULL;
    core_settings.ext_timer_callback = NULL;
    core_settings.script_gc_interval = DEFAULT_SCRIPT_GC_INTERVAL;
    core_settings.online_autosave_interval = DEFAULT_ONLINE_AUTOSAVE_INTERVAL;

    core_settings.base_dir = NULL;
    core_settings.mod_dir = NULL;
    core_settings.log_dir = NULL;
    core_settings.runtime_dir = NULL;
    core_settings.script_dir = NULL;

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

// Parse runtime setting
int _parse_runtime_setting(BSP_STRING *rs)
{
    if (!rs)
    {
        return BSP_RTN_FATAL;
    }

    if (!core_settings.initialized)
    {
        init_core_setting();
    }

    runtime_settings = json_nd_decode(rs);
    if (!runtime_settings || OBJECT_TYPE_HASH != runtime_settings->type)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Core   : Runtime setting parse error");
        return BSP_RTN_ERROR_GENERAL;
    }

    trace_msg(TRACE_LEVEL_NOTICE, "Core   : Runtime setting parsed");
    BSP_OBJECT *vobj;
    BSP_STRING *vstr;
    BSP_VALUE *val;
    // Global settings
    val = object_get_hash_str(runtime_settings, "global");
    vobj = value_get_object(val);
    if (vobj && OBJECT_TYPE_HASH == vobj->type)
    {
        val = object_get_hash_str(vobj, "instance_id");
        if (val && BSP_VAL_INT == val->type)
        {
            core_settings.instance_id = (int) value_get_int(val);
        }
        val = object_get_hash_str(vobj, "static_workers");
        if (val && BSP_VAL_INT == val->type)
        {
            core_settings.static_workers = value_get_int(val);
            if (core_settings.static_workers <= 0)
            {
                core_settings.static_workers = DEFAULT_STATIC_WORKERS;
            }
        }
        val = object_get_hash_str(vobj, "debug_output");
        core_settings.debug_hex_output = value_get_boolean(val);
        val = object_get_hash_str(vobj, "debug_connector_input");
        core_settings.debug_hex_connector_input = value_get_boolean(val);
        val = object_get_hash_str(vobj, "log_dir");
        vstr = value_get_string(val);
        core_settings.log_dir = (vstr) ? bsp_strndup(STR_STR(vstr), STR_LEN(vstr)) : NULL;
        val = object_get_hash_str(vobj, "enable_log");
        if (value_get_boolean(val) && core_settings.is_daemonize)
        {
            log_open();
            core_settings.trace_recorder = log_add;
        }
        val = object_get_hash_str(vobj, "script_dir");
        vstr = value_get_string(val);
        core_settings.script_dir = (vstr) ? bsp_strndup(STR_STR(vstr), STR_LEN(vstr)) : NULL;
        val = object_get_hash_str(vobj, "script_gc_interval");
        if (val && BSP_VAL_INT == val->type)
        {
            core_settings.script_gc_interval = value_get_int(val);
        }
    }

    return BSP_RTN_SUCCESS;
}

// Load server setting
int load_runtime_setting()
{
#ifdef ENABLE_STANDALONE
    // On disk IO
    BSP_STRING *rs = new_string_from_file(RUNTIME_SETTING_FILE);
    if (!rs)
    {
        trigger_exit(BSP_RTN_FATAL, "Cannot load runtime setting file");
    }
    else
    {
        _parse_runtime_setting(rs);
    }

    return BSP_RTN_SUCCESS;
#else
    // Read from manager
    return BSP_RTN_SUCCESS;
#endif
}

// Load bootstrap
int load_bootstrap()
{
#ifdef ENABLE_STANDALONE
    // On disk IO
    BSP_STRING *bs = new_string_from_file(BOOTSTRAP_FILE);
    if (!bs)
    {
        trigger_exit(BSP_RTN_FATAL, "Cannot load bootstrap file");
    }
    else
    {
        start_bootstrap(bs);
    }

    return BSP_RTN_SUCCESS;
#else
    // Read from manager
    return BSP_RTN_SUCCESS;
#endif
}

// Init BSP-core
int core_init()
{
    if (!core_settings.initialized)
    {
        init_core_setting();
    }

    if (core_settings.is_daemonize)
    {
        proc_daemonize();
    }

    fd_init(0);
    log_init();
    load_runtime_setting();
    status_op_core(STATUS_OP_INSTANCE_ID, (size_t) core_settings.instance_id);
    thread_init();
    save_pid();
    signal_init();
    socket_init();
    memdb_init();
    online_init();

    // Load modules
    BSP_VALUE *val = object_get_hash_str(runtime_settings, "modules");
    BSP_OBJECT *vobj = value_get_object(val);
    BSP_STRING *vstr = NULL;
    if (vobj && OBJECT_TYPE_ARRAY == vobj->type)
    {
        size_t varr_size = object_size(vobj), i;
        if (varr_size > MAX_MODULES)
        {
            varr_size = MAX_MODULES;
        }

        reset_object(vobj);
        for (i = 0; i < varr_size; i ++)
        {
            val = object_get_array(vobj, i);
            vstr = value_get_string(val);
            if (vstr)
            {
                script_load_module(vstr, 1);
            }
        }
    }

    return BSP_RTN_SUCCESS;
}

// Start main loop
int core_loop(void (* server_event)(BSP_CALLBACK *))
{
    BSP_THREAD *t = get_thread(MAIN_THREAD);
    if (!t)
    {
        trigger_exit(BSP_RTN_FATAL, "Main thread lost!");
    }

    // Servers
    BSP_VALUE *val = object_get_hash_str(runtime_settings, "servers");
    BSP_OBJECT *vobj = value_get_object(val);
    BSP_STRING *vstr = NULL;
    if (vobj && OBJECT_TYPE_ARRAY == vobj->type)
    {
        struct bsp_conf_server_t srv;
        BSP_OBJECT *vsrv = NULL;
        size_t varr_size = object_size(vobj), i;
        reset_object(vobj);
        for (i = 0; i < varr_size; i ++)
        {
            val = object_get_array(vobj, i);
            vsrv = value_get_object(val);
            if (vsrv && OBJECT_TYPE_HASH == vsrv->type)
            {
                // Default value
                memset(&srv, 0, sizeof(struct bsp_conf_server_t));
                srv.server_inet = INET_TYPE_ANY;
                srv.server_sock = SOCK_TYPE_ANY;
                srv.def_client_type = CLIENT_TYPE_DATA;
                srv.def_data_type = DATA_TYPE_PACKET;
                val = object_get_hash_str(vsrv, "name");
                vstr = value_get_string(val);
                srv.server_name = bsp_strndup(STR_STR(vstr), STR_LEN(vstr));
                val = object_get_hash_str(vsrv, "inet");
                vstr = value_get_string(val);
                if (vstr)
                {
                    if (0 == strncasecmp(STR_STR(vstr), "ipv6", 4))
                    {
                        srv.server_inet = INET_TYPE_IPV6;
                    }
                    else if (0 == strncasecmp(STR_STR(vstr), "ipv4", 4))
                    {
                        srv.server_inet = INET_TYPE_IPV4;
                    }
                    else if (0 == strncasecmp(STR_STR(vstr), "local", 5))
                    {
                        srv.server_inet = INET_TYPE_LOCAL;
                    }
                }
                val = object_get_hash_str(vsrv, "sock");
                vstr = value_get_string(val);
                if (vstr)
                {
                    if (0 == strncasecmp(STR_STR(vstr), "tcp", 3))
                    {
                        srv.server_sock = SOCK_TYPE_TCP;
                    }
                    else if (0 == strncasecmp(STR_STR(vstr), "udp", 3))
                    {
                        srv.server_sock = SOCK_TYPE_UDP;
                    }
                }
                val = object_get_hash_str(vsrv, "addr");
                vstr = value_get_string(val);
                srv.server_addr = bsp_strndup(STR_STR(vstr), STR_LEN(vstr));
                val = object_get_hash_str(vsrv, "port");
                srv.server_port = (int) value_get_int(val);
                val = object_get_hash_str(vsrv, "heartbeat_check");
                srv.heartbeat_check = (int) value_get_int(val);
                val = object_get_hash_str(vsrv, "debug_input");
                srv.debug_hex_input = value_get_boolean(val);
                val = object_get_hash_str(vsrv, "debug_output");
                srv.debug_hex_input = value_get_boolean(val);
                val = object_get_hash_str(vsrv, "max_clients");
                srv.max_clients = (int) value_get_int(val);
                val = object_get_hash_str(vsrv, "max_packet_length");
                srv.max_packet_length = (size_t) value_get_int(val);
                val = object_get_hash_str(vsrv, "websocket");
                if (value_get_boolean(val))
                {
                    srv.def_client_type = CLIENT_TYPE_WEBSOCKET_HANDSHAKE;
                }
                val = object_get_hash_str(vsrv, "data_type");
                vstr = value_get_string(val);
                if (vstr && 0 == strncasecmp(STR_STR(vstr), "stream", 6))
                {
                    srv.def_data_type = DATA_TYPE_STREAM;
                }

                // Add server
                BSP_SERVER *s;
                int srv_fds[MAX_SERVER_PER_CREATION], srv_ct, fd_type;
                int nfds = MAX_SERVER_PER_CREATION;
                nfds = new_server(srv.server_addr, srv.server_port, srv.server_inet, srv.server_sock, srv_fds, &nfds);
                for (srv_ct = 0; srv_ct < nfds; srv_ct ++)
                {
                    fd_type = FD_TYPE_SOCKET_SERVER;
                    s = (BSP_SERVER *) get_fd(srv_fds[srv_ct], &fd_type);
                    if (s)
                    {
                        s->name = srv.server_name;
                        s->heartbeat_check = srv.heartbeat_check;
                        s->def_client_type = srv.def_client_type;
                        s->def_data_type = srv.def_data_type;
                        s->max_packet_length = srv.max_packet_length;
                        s->max_clients = srv.max_clients;
                        s->debug_hex_input = srv.debug_hex_input;
                        s->debug_hex_output = srv.debug_hex_output;

                        add_server(s);
                    }
                    else
                    {
                        bsp_free(srv.server_name);
                    }
                }
            }
        }
    }

    // Server event
    if (server_event)
    {
        core_settings.on_srv_events = server_event;
    }

    // Create 1 Hz clock
    BSP_TIMER *tmr = new_timer(BASE_CLOCK_SEC, BASE_CLOCK_USEC, -1);
    tmr->on_timer = base_timer;
    core_settings.main_timer = tmr;
    dispatch_to_thread(tmr->fd, MAIN_THREAD);
    start_timer(tmr);

    // Let's go
    load_bootstrap();
    trace_msg(TRACE_LEVEL_CORE, "Core   : Main thread loop started");
/*
    debug_object(runtime_settings);
    BSP_VALUE *r;
    r = object_get_value(runtime_settings, "global");
    debug_value(r);
    r = object_get_value(runtime_settings, "modules");
    debug_value(r);
    r = object_get_value(runtime_settings, "global.script_dir");
    debug_value(r);
    r = object_get_value(runtime_settings, "modules.2");
    debug_value(r);
    r = object_get_value(runtime_settings, "servers.0");
    debug_value(r);
    r = object_get_value(runtime_settings, "servers.0.name");
    debug_value(r);
    r = object_get_value(runtime_settings, "servers.test");
    debug_value(r);
    r = object_get_value(runtime_settings, "global..script_dir");
    debug_value(r);
    r = object_get_value(runtime_settings, ".global.script_dir");
    debug_value(r);
*/
    thread_process((void *) t);

    return BSP_RTN_SUCCESS;
}
