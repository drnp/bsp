/*
 * fcgi.c
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
 * @packet libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/08/2014
 * @chagelog 
 *      [05/08/2014] - Creation
 */

#include "bsp.h"

static void _build_header(BSP_STRING *req, int type, size_t length)
{
    static unsigned char hdr[8];
    if (req)
    {
        hdr[0] = 1;
        hdr[1] = (unsigned char) type;
        hdr[2] = 0;
        hdr[3] = 1;
        hdr[4] = (length >> 8) & 0xFF;
        hdr[5] = length & 0xFF;
        hdr[6] = (8 - (length & 7)) & 7;
        hdr[7] = 0;

        string_append(req, (const char *) hdr, 8);
    }

    return;
}

static void _build_role(BSP_STRING *req, int role, int flag)
{
    unsigned char r[8];
    if (req)
    {
        r[0] = (role >> 8) & 0XFF;
        r[1] = role & 0xFF;
        r[2] = (unsigned char) flag;
        r[3] = 0;
        r[4] = 0;
        r[5] = 0;
        r[6] = 0;
        r[7] = 0;

        string_append(req, (const char *) r, 8);
    }

    return;
}

static void _build_nv_pair(BSP_STRING *str, const char *name, const char *value)
{
    static unsigned char len[4];
    if (str && name)
    {
        size_t len_name = strlen(name);
        size_t len_value = (value) ? strlen(value) : 0;

        if (len_name > 127)
        {
            set_int32(len_name, (char *) len);
            string_append(str, (const char *) len, 4);
        }
        else
        {
            set_int8(len_name, (char *) len);
            string_append(str, (const char *) len, 1);
        }

        if (len_value > 127)
        {
            set_int32(len_value, (char *) len);
            string_append(str, (const char *) len, 4);
        }
        else
        {
            set_int8(len_value, (char *) len);
            string_append(str, (const char *) len, 1);
        }
        string_append(str, name, len_name);
        string_append(str, value, len_value);
    }

    return;
}

// Build FastCGI request
BSP_STRING * build_fcgi_request(BSP_FCGI_PARAMS *params, BSP_STRING *post_data)
{
    BSP_STRING *req = new_string(NULL, 0);

    // Create begin
    _build_header(req, FCGI_BEGIN_REQUEST, 8);
    _build_role(req, FCGI_RESPONDER, 0);

    // Create params
    BSP_STRING *p = new_string(NULL, 0);
    _build_nv_pair(p, "QUERY_STRING", params->query_string);
    _build_nv_pair(p, "REQUEST_METHOD", (params->request_method) ? (params->request_method) : (post_data ? "POST" : "GET"));
    _build_nv_pair(p, "CONTENT_TYPE", params->content_type);
    _build_nv_pair(p, "CONTENT_LENGTH", params->content_length);
    _build_nv_pair(p, "SCRIPT_FILENAME", params->script_filename);
    _build_nv_pair(p, "SCRIPT_NAME", params->script_name);
    _build_nv_pair(p, "REQUEST_URI", params->request_uri);
    _build_nv_pair(p, "DOCUMENT_URI", params->document_uri);
    _build_nv_pair(p, "DOCUMENT_ROOT", params->document_root);
    _build_nv_pair(p, "SERVER_PROTOCOL", (params->server_protocol) ? (params->server_protocol) : FCGI_PARAMS_DEFAULT_SERVER_PROTOCOL);
    _build_nv_pair(p, "GATEWAY_INTERFACE", (params->gateway_interface) ? (params->gateway_interface) : FCGI_PARAMS_DEFAULT_GATEWAY_INTERFACE);
    _build_nv_pair(p, "SERVER_SOFTWARE", (params->server_software) ? (params->server_software) : FCGI_PARAMS_DEFAULT_SERVER_SOFTWARE);
    _build_nv_pair(p, "REMOTE_ADDR", params->remote_addr);
    _build_nv_pair(p, "REMOTE_PORT", params->remote_port);
    _build_nv_pair(p, "SERVER_ADDR", params->server_addr);
    _build_nv_pair(p, "SERVER_PORT", params->server_port);
    _build_nv_pair(p, "SERVER_NAME", params->server_name);

    // Additional HTTP params
    // NONE now ...

    size_t len = STR_LEN(p);
    size_t curr = 0;
    size_t size = 0;
    for (curr = 0; curr < len; curr += 65536)
    {
        if (len - curr > 65536)
        {
            size = 65536;
        }
        else
        {
            size = len - curr;
        }

        _build_header(req, FCGI_PARAMS, size);
        string_append(req, STR_STR(p) + curr, size);
    }
    size = (8 - (len & 7)) & 7;
    if (size > 0)
    {
        string_fill(req, 0, size);
    }
    _build_header(req, FCGI_PARAMS, 0);
    del_string(p);

    // Create stdin
    if (post_data)
    {
        for (curr = 0; curr < STR_LEN(post_data); curr += 65536)
        {
            if (STR_LEN(post_data) - curr > 65536)
            {
                size = 65536;
            }
            else
            {
                size = STR_LEN(post_data) - curr % 65536;
            }

            _build_header(req, FCGI_STDIN, size);
            string_append(req, STR_STR(post_data) + curr, size);
        }
        size = (8 - (STR_LEN(post_data) & 7)) & 7;
        if (size > 0)
        {
            string_fill(req, 0, size);
        }
    }
    _build_header(req, FCGI_STDIN, 0);

    return req;
}

// Parse FCGI response
size_t parse_fcgi_response(BSP_FCGI_RESPONSE *resp, BSP_STRING *data)
{
    if (!data || !resp)
    {
        return 0;
    }
    size_t curr = 0;
    unsigned char hdr[8];
    unsigned char d[8];
    int content_length;
    int padding_length;
    //debug_hex(data, len);
    while ((STR_LEN(data) - curr) >= 8)
    {
        // First read header
        memcpy(hdr, STR_STR(data) + curr, 8);
        content_length = (hdr[4] << 8) + hdr[5];
        padding_length = hdr[6];

        if (FCGI_END_REQUEST == hdr[1] && (STR_LEN(data) - curr) >= 8)
        {
            // Response complete
            // Read body
            memcpy(d, STR_STR(data) + curr + 8, 8);
            resp->request_id = (hdr[2] << 8) + hdr[3];
            resp->app_status = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];
            resp->protocol_status = d[4];
            resp->is_ended = 1;
            curr += 16;
            break;
        }
        else if (FCGI_STDOUT == hdr[1] && (STR_LEN(data) - curr) >=  (content_length + padding_length + 8))
        {
            // Read data
            if (!resp->data_stdout)
            {
                resp->data_stdout = new_string(NULL, 0);
            }

            if (content_length > 0)
            {
                string_append(resp->data_stdout, STR_STR(data) + curr + 8, content_length);
            }
            curr += (content_length + padding_length + 8);
        }
        else if (FCGI_STDERR == hdr[1] && (STR_LEN(data) - curr) >= (content_length + padding_length + 8))
        {
            // Read data
            if (!resp->data_stderr)
            {
                resp->data_stderr = new_string(NULL, 0);
            }

            if (content_length > 0)
            {
                string_append(resp->data_stderr, STR_STR(data) + curr + 8, content_length);
            }
            curr += (content_length + padding_length + 8);
        }
        else
        {
            // Unsupported request type from FCGI server
            break;
        }
    }

    return curr;
}

// New upstream (Server group)
BSP_FCGI_UPSTREAM * new_fcgi_upstream(const char *name)
{
    BSP_FCGI_UPSTREAM * upstream = bsp_calloc(1, sizeof(BSP_FCGI_UPSTREAM));
    if (upstream)
    {
        upstream->name = bsp_strdup(name);
    }

    return upstream;
}

// Del upstream
int del_fcgi_upstream(BSP_FCGI_UPSTREAM *upstream)
{
    if (!upstream)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    size_t i;
    struct bsp_fcgi_upstream_entry_t *entry = NULL;
    if (upstream->pool)
    {
        for (i = 0; i < upstream->pool_size; i ++)
        {
            if (upstream->pool[i] != entry)
            {
                if (entry)
                {
                    if (entry->host)
                    {
                        bsp_free((void *) entry->host);
                    }
                    if (entry->sock)
                    {
                        bsp_free((void *) entry->sock);
                    }
                    if (entry->script_filename)
                    {
                        bsp_free((void *) entry->script_filename);
                    }
                    bsp_free(entry);
                }
                entry = upstream->pool[i];
            }
        }
        bsp_free(entry);
    }

    bsp_free(upstream);

    return BSP_RTN_SUCCESS;
}

// Add fastcgi server entry to upstream
void add_fcgi_server_entry(BSP_FCGI_UPSTREAM *upstream, struct bsp_fcgi_upstream_entry_t *entry)
{
    if (!upstream || !entry || (!entry->host && !entry->sock))
    {
        return;
    }

    if (entry->weight <= 0)
    {
        entry->weight = 1;
    }

    size_t i, new_pool_size = upstream->pool_size + entry->weight;
    struct bsp_fcgi_upstream_entry_t **new_pool = bsp_realloc(upstream->pool, (upstream->pool_size + entry->weight) * sizeof(struct bsp_fcgi_upstream_entry_t *));
    if (!new_pool)
    {
        return;
    }
    for (i = upstream->pool_size; i < new_pool_size; i ++)
    {
        new_pool[i] = entry;
    }
    upstream->pool = new_pool;
    upstream->pool_size = new_pool_size;

    return;
}

// Select an entry from upstream (Rate calculated by weight)
struct bsp_fcgi_upstream_entry_t * get_fcgi_upstream_entry(BSP_FCGI_UPSTREAM *upstream)
{
    if (!upstream || !upstream->pool_size || !upstream->pool)
    {
        return NULL;
    }

    static char str[4];
    size_t i;
    if (1 == upstream->pool_size)
    {
        // Single entry upstream
        i = 0;
    }
    else
    {
        // Get rand from system
        get_rand(str, 4);
        int val = get_int32(str);
        i = val % upstream->pool_size;
    }

    return upstream->pool[i];
}

// Send FCGI request
static void _fcgi_finish(BSP_CONNECTOR *cnt)
{
    if (!cnt || !cnt->additional)
    {
        return;
    }

    BSP_FCGI_RESPONSE *resp = (BSP_FCGI_RESPONSE *) cnt->additional;
    if (!cnt->script_stack.stack || !lua_isfunction(cnt->script_stack.stack, -1) || !resp->is_ended)
    {
        return;
    }

    trace_msg(TRACE_LEVEL_VERBOSE, "FCGI   : FCGI request finished");
    status_op_fcgi(STATUS_OP_FCGI_RESPONSE);
    lua_State *caller = cnt->script_stack.stack;
    bsp_spin_lock(&cnt->script_stack.lock);
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
        lua_newtable(caller);
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
            lua_pushstring(caller, "header");
            lua_pushlstring(caller, STR_STR(resp->data_stdout), i);
            lua_settable(caller, -3);

            lua_pushstring(caller, "body");
            lua_pushlstring(caller, STR_STR(resp->data_stdout) + i + 4, STR_LEN(resp->data_stdout) - i - 4);
            lua_settable(caller, -3);
        }
        else
        {
            lua_pushstring(caller, "body");
            lua_pushlstring(caller, STR_STR(resp->data_stdout), STR_LEN(resp->data_stdout));
            lua_settable(caller, -3);
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
    bsp_spin_unlock(&cnt->script_stack.lock);

    return;
}

static size_t _fcgi_on_data(BSP_CONNECTOR *cnt, const char *data, ssize_t len)
{
    BSP_FCGI_RESPONSE *resp = NULL;
    if (!cnt->additional)
    {
        resp = bsp_calloc(1, sizeof(BSP_FCGI_RESPONSE));
        if (!resp)
        {
            trigger_exit(BSP_RTN_ERROR_MEMORY, "Cannot alloc FCGI response");
        }
        cnt->additional = (void *) resp;
    }
    else
    {
        resp = (BSP_FCGI_RESPONSE *) cnt->additional;
    }

    size_t ret = parse_fcgi_response(resp, new_string_const(data, len));
    if (resp->is_ended)
    {
        _fcgi_finish(cnt);
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
    if (!cnt || !cnt->additional)
    {
        return;
    }

    trace_msg(TRACE_LEVEL_NOTICE, "FCGI   : Conenction peer closed by remote FCGI server");
    BSP_FCGI_RESPONSE *resp = (BSP_FCGI_RESPONSE *) cnt->additional;
    if (resp->is_ended)
    {
        _fcgi_finish(cnt);
        del_string(resp->data_stdout);
        del_string(resp->data_stderr);
        bsp_free(resp);
        cnt->additional = NULL;
    }

    return;
}
int fcgi_call(BSP_FCGI_UPSTREAM *upstream, BSP_OBJECT *p, struct sockaddr_storage *addr)
{
    if (!upstream || !p)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_STRING *data = json_nd_encode(p);
    if (!data)
    {
        return BSP_RTN_ERROR_GENERAL;
    }
    struct bsp_fcgi_upstream_entry_t *entry = get_fcgi_upstream_entry(upstream);
    if (!entry)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    char len_str[16] = {0};
    snprintf(len_str, 15, "%llu", (long long unsigned int) STR_LEN(data));
    BSP_FCGI_PARAMS fp;
    memset(&fp, 0, sizeof(BSP_FCGI_PARAMS));

    fp.script_filename = entry->script_filename;
    fp.content_type = "text/html";
    fp.content_length = len_str;
    if (addr)
    {
        // Add remote addr and port into params
        char ipaddr[64];
        char port[8];
        memset(ipaddr, 0, 64);
        memset(port, 0, 8);
        if (AF_INET6 == addr->ss_family)
        {
            // IPv6
            struct sockaddr_in6 *clt_sin6 = (struct sockaddr_in6 *) addr;
            inet_ntop(AF_INET6, &clt_sin6->sin6_addr.s6_addr, ipaddr, 63);
            fp.remote_addr = ipaddr;
            snprintf(port, 7, "%d", ntohs(clt_sin6->sin6_port));
            fp.remote_port = port;
        }
        else
        {
            // IPv4
            struct sockaddr_in *clt_sin4 = (struct sockaddr_in *) addr;
            inet_ntop(AF_INET, &clt_sin4->sin_addr.s_addr, ipaddr, 63);
            fp.remote_addr = ipaddr;
            snprintf(port, 7, "%d", ntohs(clt_sin4->sin_port));
            fp.remote_port = port;
        }
    }
    trace_msg(TRACE_LEVEL_NOTICE, "Try to make a FCGI request to %s", upstream->name);
    BSP_STRING *request = build_fcgi_request(&fp, data);
    BSP_CONNECTOR *cnt = NULL;
    if (entry->host && entry->port)
    {
        cnt = new_connector(entry->host, entry->port, INET_TYPE_ANY, SOCK_TYPE_TCP);
    }
    else if (entry->sock)
    {
        cnt = new_connector(entry->sock, 0, INET_TYPE_LOCAL, SOCK_TYPE_TCP);
    }
    if (cnt)
    {
        cnt->on_close = _fcgi_on_close;
        cnt->on_data = _fcgi_on_data;
        dispatch_to_thread(SFD(cnt), curr_thread_id());
        if (cnt->script_stack.stack && upstream->callback_key)
        {
            lua_getfield(cnt->script_stack.stack, LUA_REGISTRYINDEX, upstream->callback_key);
            if (!lua_isfunction(cnt->script_stack.stack, -1))
            {
                trace_msg(TRACE_LEVEL_NOTICE, "FCGI   : Cannot find registered callback function %s", upstream->callback_key);
            }
        }
        append_data_socket(&SCK(cnt), request);
        flush_socket(&SCK(cnt));
        status_op_fcgi(STATUS_OP_FCGI_REQUEST);
    }
    del_string(data);
    del_string(request);

    return BSP_RTN_SUCCESS;
}
