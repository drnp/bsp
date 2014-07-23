/*
 * module_fcgi.c
 *
 * Copyright (C) 2014 - Dr.NP
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
 * FastCGI implementation
 * 
 * @package modules::fcgi
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/30/2014
 * @changelog
 *      [06/30/2014] - Creation
 */

#include "bsp.h"

#include "module_fcgi.h"

BSP_OBJECT *fcgi_upstreams = NULL;
BSP_SPINLOCK fcgi_upstreams_lock = BSP_SPINLOCK_INITIALIZER;

// Initialization
static void _fcgi_init()
{
    fcgi_upstreams = new_object();
    if (!fcgi_upstreams)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "FCGI memory error");
    }
    trace_msg(TRACE_LEVEL_DEBUG, "FCGI-M : FCGI initialized");
}

static void _finish_fcgi_resp(BSP_CONNECTOR *cnt)
{
    if (!cnt || !cnt->script_stack.stack)
    {
        return;
    }

    BSP_FCGI_RESPONSE *resp = cnt->additional;
    if (!resp || !resp->is_ended)
    {
        return;
    }

    lua_State *caller = cnt->script_stack.stack;
    if (lua_isfunction(caller, -1))
    {
        lua_checkstack(caller, 3);
        lua_newtable(caller);

        lua_pushstring(caller, "app_status");
        lua_pushinteger(caller, resp->app_status);
        lua_settable(caller, -3);

        lua_pushstring(caller, "protocol_status");
        lua_pushinteger(caller, resp->protocol_status);
        lua_settable(caller, -3);

        if (resp->data_stdout)
        {
            lua_pushstring(caller, "stdout");
            // Find {CRLF}{CRLF}
            size_t i;
            int has_header = 0;
            for (i = 0; i < STR_LEN(resp->data_stdout) - 3; i ++)
            {
                unsigned char *t = (unsigned char *) STR_STR(resp->data_stdout) + i;
                if (0xd == t[0] && 0xa == t[1] && 0xd == t[2] && 0xa == t[3])
                {
                    // Split into header & body
                    has_header = 1;
                    break;
                }
            }
            if (has_header)
            {
                lua_newtable(caller);

                lua_pushstring(caller, "header");
                lua_pushlstring(caller, STR_STR(resp->data_stdout), i);
                lua_settable(caller, -3);

                lua_pushstring(caller, "body");
                lua_pushlstring(caller, STR_STR(resp->data_stdout) + i + 4, STR_LEN(resp->data_stdout) - i - 4);
                lua_settable(caller, -3);
            }
            else
            {
                lua_pushlstring(caller, STR_STR(resp->data_stdout), STR_LEN(resp->data_stdout));
            }

            lua_settable(caller, -3);
        }

        if (resp->data_stderr)
        {
            lua_pushstring(caller, "stderr");
            lua_pushlstring(caller, STR_STR(resp->data_stderr), STR_LEN(resp->data_stderr));
            lua_settable(caller, -3);
        }

        lua_pcall(caller, 1, 0, 0);
    }
    else
    {
        lua_settop(caller, 0);
    }

    return;
}

static size_t _fcgi_on_response(BSP_CONNECTOR *cnt, const char *data, ssize_t len)
{
    BSP_FCGI_RESPONSE *resp;
    if (!cnt->additional)
    {
        resp = bsp_calloc(1, sizeof(BSP_FCGI_RESPONSE));
        if (!resp)
        {
            trigger_exit(BSP_RTN_ERROR_MEMORY, "FCGI memory error");
        }
        cnt->additional = (void *) resp;
    }
    else
    {
        resp = (BSP_FCGI_RESPONSE *) cnt->additional;
    }
    
    size_t ret = parse_fcgi_response(resp, data, len);
    if (resp->is_ended)
    {
        trace_msg(TRACE_LEVEL_VERBOSE, "FCGI-M : Request finished");
        _finish_fcgi_resp(cnt);
        del_string(resp->data_stdout);
        del_string(resp->data_stderr);
        bsp_free(resp);
        cnt->additional = NULL;
        free_connector(cnt);
    }
    
    return ret;
}

static void _fcgi_on_close(BSP_CONNECTOR *cnt)
{
    if (!cnt)
    {
        return;
    }
    
    if (!cnt->additional)
    {
        return;
    }

    trace_msg(TRACE_LEVEL_DEBUG, "FCGI-M : Connection peer closed by remote FCGI server");
    BSP_FCGI_RESPONSE *resp = (BSP_FCGI_RESPONSE *) cnt->additional;
    if (resp && resp->is_ended)
    {
        _finish_fcgi_resp(cnt);
        del_string(resp->data_stdout);
        del_string(resp->data_stderr);
        bsp_free(resp);
    }
    
    return;
}

// Set upstream server group
static int fcgi_set_upstream(lua_State *s)
{
    if (!s || !lua_isstring(s, -2) || !lua_istable(s, -1) || !fcgi_upstreams)
    {
        return 0;
    }

    BSP_OBJECT_ITEM *item;
    struct fcgi_upstream_entry_t *entry;
    struct fcgi_upstream_t *upstream;
    const char *name = lua_tostring(s, -2);
    const char *host;
    int port, weight;
    int n = 0;

    bsp_spin_lock(&fcgi_upstreams_lock);
    upstream = bsp_calloc(1, sizeof(struct fcgi_upstream_t));
    lua_checkstack(s, 1);
    lua_pushnil(s);
    while (lua_next(s, -2))
    {
        if (lua_istable(s, -1))
        {
            lua_getfield(s, -1, "host");
            host = lua_tostring(s, -1);
            lua_pop(s, 1);
            lua_getfield(s, -1, "port");
            port = lua_tointeger(s, -1);
            lua_pop(s, 1);
            lua_getfield(s, -1, "weight");
            weight = lua_tointeger(s, -1);
            lua_pop(s, 1);

            if (host && port)
            {
                // A new entry
                entry = bsp_calloc(1, sizeof(struct fcgi_upstream_entry_t));
                if (!entry || !upstream)
                {
                    trigger_exit(BSP_RTN_ERROR_MEMORY, "FCGI memory error");
                }
                entry->host = bsp_strdup(host);
                entry->port = port;
                entry->weight = weight ? weight : 1;
                entry->next = upstream->entries;
                upstream->entries = entry;
                upstream->total_weight += weight;
                n ++;
            }
        }
        lua_pop(s, 1);
    }
    item = object_get_item(fcgi_upstreams, name, -1);
    if (item)
    {
        // Overwrite
        if (BSP_VAL_POINTER == item->value.type)
        {
            // Clear old upstream
            struct fcgi_upstream_t *old_upstream = (struct fcgi_upstream_t *) item->value.rval;
            struct fcgi_upstream_entry_t *old_entry;
            if (old_upstream)
            {
                entry = old_upstream->entries;
                while (entry)
                {
                    if (entry->host)
                    {
                        bsp_free(entry->host);
                    }
                    old_entry = entry;
                    entry = entry->next;
                    bsp_free(old_entry);
                }
            }
        }
        object_remove_item(fcgi_upstreams, item);
        del_object_item(item);
    }
    
    if (upstream->entries)
    {
        // Set new upstream
        item = new_object_item(name, -1);
        set_item_pointer(item, (const void *) upstream);
        object_insert_item(fcgi_upstreams, item);
    }
    else
    {
        // Nothing to set
        bsp_free(upstream);
    }
    bsp_spin_unlock(&fcgi_upstreams_lock);
    trace_msg(TRACE_LEVEL_DEBUG, "FCGI-M : FCGI upstream %s added with %d entries", name, n);

    return 0;
}

// Generate and do a FastCGI request
static int fcgi_send_request(lua_State *s)
{
    if (!s || !lua_isstring(s, -3) || !lua_istable(s, -2) || !fcgi_upstreams)
    {
        return 0;
    }

    const char *name = lua_tostring(s, -3);
    BSP_OBJECT_ITEM *item = object_get_item(fcgi_upstreams, name, -1);
    if (!item || item->value.type != BSP_VAL_POINTER || !item->value.rval)
    {
        lua_settop(s, 0);
        return 0;
    }
    struct fcgi_upstream_t *upstream = (struct fcgi_upstream_t *) item->value.rval;
    struct fcgi_upstream_entry_t *entry = NULL;
    BSP_FCGI_PARAMS p;
    memset(&p, 0, sizeof(BSP_FCGI_PARAMS));
    int n = 0;
    
    lua_getfield(s, -2, "QUERY_STRING");
    p.query_string = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "REQUEST_METHOD");
    p.request_method = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "CONTENT_TYPE");
    p.content_type = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "CONTENT_LENGTH");
    p.content_length = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SCRIPT_FILENAME");
    p.script_filename = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "REQUEST_URI");
    p.request_uri = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "DOCUMENT_URI");
    p.document_uri = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "DOCUMENT_ROOT");
    p.document_root = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SERVER_PROTOCOL");
    p.server_protocol = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "GATEWAY_INTERFACE");
    p.gateway_interface = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SERVER_SOFTWARE");
    p.server_software = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "REMOTE_ADDR");
    p.remote_addr = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "REMOTE_PORT");
    p.remote_port = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SERVER_ADDR");
    p.server_addr = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SERVER_PORT");
    p.server_port = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "SERVER_NAME");
    p.server_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -2, "POST_DATA");
    size_t post_len = 0;
    const char *post_data = lua_tolstring(s, -1, &post_len);
    lua_pop(s, 1);
    
    BSP_STRING *req = build_fcgi_request(&p, post_data, post_len);
    // Find a server;
    bsp_spin_lock(&fcgi_upstreams_lock);
    entry = upstream->entries;
    while (entry)
    {
        n += entry->weight;
        if (upstream->counter < n)
        {
            // Selected
            break;
        }
        entry = entry->next;
    }
    upstream->counter ++;
    if (upstream->counter >= upstream->total_weight)
    {
        upstream->counter = 0;
    }
    bsp_spin_unlock(&fcgi_upstreams_lock);
    // Real send
    if (entry && entry->host && entry->port)
    {
        BSP_CONNECTOR *cnt = new_connector(entry->host, entry->port, INET_TYPE_ANY, SOCK_TYPE_TCP);
        if (cnt)
        {
            cnt->on_data = _fcgi_on_response;
            cnt->on_close = _fcgi_on_close;
            cnt->additional = NULL;
            dispatch_to_thread(SFD(cnt), curr_thread_id());
            
            if (lua_isfunction(s, -1))
            {
                // Callback function
                lua_xmove(s, cnt->script_stack.stack, 1);
            }
            else if (lua_isstring(s, -1))
            {
                // Global function
                lua_getglobal(cnt->script_stack.stack, lua_tostring(s, -1));
            }
            else
            {
                // Nothing to call
                lua_pushnil(cnt->script_stack.stack);
            }
            append_data_socket(&SCK(cnt), (const char *) STR_STR(req), STR_LEN(req));
            flush_socket(&SCK(cnt));
        }
    }
    del_string(req);
    
    return 0;
}

int bsp_module_fcgi(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    bsp_spin_lock(&fcgi_upstreams_lock);
    if (!fcgi_upstreams)
    {
        _fcgi_init();
    }
    bsp_spin_unlock(&fcgi_upstreams_lock);

    lua_pushcfunction(s, fcgi_set_upstream);
    lua_setglobal(s, "bsp_fcgi_set_upstream");
    lua_pushcfunction(s, fcgi_send_request);
    lua_setglobal(s, "bsp_fcgi_send_request");
    
    lua_settop(s, 0);
    
    return 0;
}
