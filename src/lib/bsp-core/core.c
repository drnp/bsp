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
char **modules;
size_t modules_total;

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
    core_settings.trace_recorder = NULL;
    core_settings.trace_printer = NULL;
    core_settings.on_proc_exit = NULL;
    core_settings.on_proc_tstp = NULL;
    core_settings.on_proc_usr1 = NULL;
    core_settings.on_proc_usr2 = NULL;
    core_settings.server_callback = NULL;
    core_settings.main_timer = NULL;
    
    core_settings.base_dir = NULL;
    core_settings.mod_dir = NULL;
    core_settings.log_dir = NULL;
    core_settings.runtime_dir = NULL;
    core_settings.script_dir = NULL;
    core_settings.script_callback_entry = NULL;
    
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
    
    BSP_OBJECT *runtime_setting = new_object();
    json_nd_decode(STR_STR(rs), STR_LEN(rs), runtime_setting);
    trace_msg(TRACE_LEVEL_NOTICE, "Core   : Runtime setting parsed");
    
    BSP_OBJECT *vobj, *vsrv;
    BSP_OBJECT_ITEM **varr, *vitem;
    BSP_VAL val;
    BSP_SERVER *s;
    BSP_SERVER_CALLBACK *sc;
    size_t i, varr_size;
    int fd_type, srv_ct, nfds, srv_fds[MAX_SERVER_PER_CREATION];
    //debug_object(runtime_setting);
    // Global settings
    object_item_value(runtime_setting, &val, "global", -1);
    vobj = val.v_obj;
    if (vobj)
    {
        object_item_value(vobj, &val, "instance_id", -1);
        if (val.v_int)
        {
            core_settings.instance_id = (int) val.v_int;
        }
        object_item_value(vobj, &val, "static_workers", -1);
        if (val.v_int)
        {
            core_settings.static_workers = val.v_int;
        }
        object_item_value(vobj, &val, "debug_output", -1);
        core_settings.debug_hex_output = (int) val.v_int;
        object_item_value(vobj, &val, "debug_connector_input", -1);
        core_settings.debug_hex_connector_input = (int) val.v_int;
        object_item_value(vobj, &val, "log_dir", -1);
        core_settings.log_dir = (val.v_str) ? strdup(val.v_str) : NULL;
        object_item_value(vobj, &val, "enable_log", -1);
        if (val.v_int && core_settings.is_daemonize)
        {
            log_open();
            core_settings.trace_recorder = log_add;
        }
        object_item_value(vobj, &val, "script_dir", -1);
        core_settings.script_dir = (val.v_str) ? strdup(val.v_str) : NULL;
    }
    // Script modules
    object_item_value(runtime_setting, &val, "modules", -1);
    varr = val.v_arr;
    varr_size = val.v_arr_size;
    if (varr_size > MAX_MODULES)
    {
        // We do not like so many modules -_-
        varr_size = MAX_MODULES;
    }
    
    if (varr)
    {
        for (i = 0; i < varr_size; i ++)
        {
            vitem = varr[i];
            if (vitem && BSP_VAL_STRING == vitem->value.type)
            {
                modules[modules_total ++] = bsp_strndup(vitem->value.rval, vitem->value.rval_len);
            }
        }
    }
    // Servers
    object_item_value(runtime_setting, &val, "servers", -1);
    vobj = val.v_obj;
    struct bsp_conf_server_t srv;
    if (vobj)
    {
        reset_object(vobj);
        while ((vitem = curr_item(vobj)))
        {
            if (vitem && BSP_VAL_OBJECT == vitem->value.type && vitem->value.rval)
            {
                vsrv = (BSP_OBJECT *) vitem->value.rval;
                // Default value
                memset(&srv, 0, sizeof(struct bsp_conf_server_t));
                srv.server_name = bsp_strndup(vitem->key, vitem->key_len);
                srv.server_inet = INET_TYPE_ANY;
                srv.server_sock = SOCK_TYPE_ANY;
                srv.def_client_type = CLIENT_TYPE_DATA;
                srv.def_data_type = DATA_TYPE_PACKET;
                object_item_value(vsrv, &val, "inet", -1);
                if (val.v_str)
                {
                    if (0 == strncasecmp(val.v_str, "ipv6", 4))
                    {
                        srv.server_inet = INET_TYPE_IPV6;
                    }
                    else if (0 == strncasecmp(val.v_str, "ipv4", 4))
                    {
                        srv.server_inet = INET_TYPE_IPV4;
                    }
                    else if (0 == strncasecmp(val.v_str, "local", 5))
                    {
                        srv.server_inet = INET_TYPE_LOCAL;
                    }
                }
                object_item_value(vsrv, &val, "sock", -1);
                if (val.v_str)
                {
                    if (0 == strncasecmp(val.v_str, "tcp", 3))
                    {
                        srv.server_sock = SOCK_TYPE_TCP;
                    }
                    else if (0 == strncasecmp(val.v_str, "udp", 3))
                    {
                        srv.server_sock = SOCK_TYPE_UDP;
                    }
                }
                object_item_value(vsrv, &val, "addr", -1);
                srv.server_addr = val.v_str;
                object_item_value(vsrv, &val, "port", -1);
                srv.server_port = (int) val.v_int;
                object_item_value(vsrv, &val, "heartbeat_check", -1);
                srv.heartbeat_check = (int) val.v_int;
                object_item_value(vsrv, &val, "debug_input", -1);
                srv.debug_hex_input = (int) val.v_int;
                object_item_value(vsrv, &val, "debug_output", -1);
                srv.debug_hex_output = (int) val.v_int;
                object_item_value(vsrv, &val, "max_clients", -1);
                srv.max_clients = (size_t) val.v_int;
                object_item_value(vsrv, &val, "max_packet_length", -1);
                srv.max_packet_length = (size_t) val.v_int;
                object_item_value(vsrv, &val, "websocket", -1);
                if (val.v_int)
                {
                    srv.def_client_type = CLIENT_TYPE_WEBSOCKET_HANDSHAKE;
                }
                object_item_value(vobj, &val, "data_type", -1);
                if (val.v_str && 0 == strncasecmp(val.v_str, "stream", 6))
                {
                    srv.def_data_type = DATA_TYPE_STREAM;
                }
                
                // Add servers
                nfds = MAX_SERVER_PER_CREATION;
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
                        
                        // Find callback defination
                        i = 0;
                        while (core_settings.server_callback)
                        {
                            sc = &core_settings.server_callback[i];
                            if (sc && sc->server_name)
                            {
                                if ((srv.server_name && 0 == strcmp(sc->server_name, srv.server_name)) || (!s->on_events && sc->server_name[0] == '*'))
                                s->on_events = sc->on_events;
                                if (sc->on_data)
                                {
                                    s->on_data = sc->on_data;
                                }
                                break;
                            }
                            else
                            {
                                break;
                            }
                            i ++;
                        }
                        
                        add_server(s);
                    }
                    else
                    {
                        bsp_free(srv.server_name);
                    }
                }
            }
            next_item(vobj);
        }
    }
    del_object(runtime_setting);

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
int core_init(BSP_SERVER_CALLBACK sc[])
{
    modules = bsp_calloc(MAX_MODULES, sizeof(char *));
    modules_total = 0;
    if (!modules)
    {
        trigger_exit(BSP_RTN_FATAL, "Cannot initialize module list");
    }
    
    if (!core_settings.initialized)
    {
        init_core_setting();
    }
    
    if (core_settings.is_daemonize)
    {
        proc_daemonize();
    }

    core_settings.server_callback = sc;
    fd_init(0);
    log_init();
    load_runtime_setting();
    status_op_core(STATUS_OP_INSTANCE_ID, (size_t) core_settings.instance_id);
    thread_init();
    save_pid();
    signal_init();
    socket_init();

    // Load modules
    int i;
    for (i = 0; i < modules_total; i ++)
    {
        script_load_module((const char *) modules[i], 1);
    }
    
    load_bootstrap();
    
    return BSP_RTN_SUCCESS;
}

// Start main loop
int core_loop(void (* timer_event)(BSP_TIMER *))
{
    BSP_THREAD *t = get_thread(MAIN_THREAD);
    if (!t)
    {
        trigger_exit(BSP_RTN_FATAL, "Main thread lost!");
    }

    if (timer_event)
    {
        // Create 1 Hz clock
        BSP_TIMER *tmr = new_timer(1, 0, -1);
        tmr->on_timer = timer_event;
        dispatch_to_thread(tmr->fd, MAIN_THREAD);
        start_timer(tmr);
    }
    trace_msg(TRACE_LEVEL_CORE, "Core   : Main thread loop started");
    thread_process((void *) t);

    return BSP_RTN_SUCCESS;
}
