/*
 * module_http.c
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
 * HTTP request producer
 * 
 * @package modules::http
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/27/2012
 * @changelog 
 *      [08/23/2012] - Creation
 *      [08/27/2012] - set_callback() added
 *      [12/17/2013] - chunk lenght bug fixed
 */

#include "bsp.h"

#include "module_http.h"

// Response callback
char *func_response_callback = NULL;

// Finish HTTP request and call LUA runner
static void _finish_http_resp(BSP_CONNECTOR *cnt)
{
    if (!cnt || !cnt->script_stack.stack)
    {
        return;
    }

    BSP_HTTP_RESPONSE *resp = (BSP_HTTP_RESPONSE *) cnt->additional;
    if (!resp)
    {
        return;
    }

    lua_State *caller = cnt->script_stack.stack;
    if (lua_isfunction(caller, -1))
    {
        lua_checkstack(caller, 3);
        lua_newtable(caller);

        lua_pushstring(caller, "version");
        lua_pushstring(caller, resp->version);
        lua_settable(caller, -3);

        lua_pushstring(caller, "status_code");
        lua_pushinteger(caller, resp->status_code);
        lua_settable(caller, -3);

        lua_pushstring(caller, "content_type");
        lua_pushstring(caller, resp->content_type);
        lua_settable(caller, -3);

        lua_pushstring(caller, "content_length");
        lua_pushinteger(caller, resp->content_length);
        lua_settable(caller, -3);

        lua_pushstring(caller, "transfer_encoding");
        lua_pushstring(caller, resp->transfer_encoding);
        lua_settable(caller, -3);

        lua_pushstring(caller, "content");
        lua_pushlstring(caller, resp->content, resp->content_length);
        lua_settable(caller, -3);

        lua_pcall(caller, 1, 0, 0);
    }
    else
    {
        lua_settop(caller, 0);
    }

    return;
}

static size_t _http_on_response(BSP_CONNECTOR *cnt, const char *data, ssize_t len)
{
    BSP_HTTP_RESPONSE * resp = NULL;
    size_t head_len = 0, i, ret = 0;
    int finished = 0;
    if (!cnt)
    {
        return len;
    }

    resp = (BSP_HTTP_RESPONSE *) cnt->additional;
    if (!resp)
    {
        // New response
        resp = new_http_response();
        if (!resp)
        {
            // Alloc response error
            free_connector(cnt);
            return len;
        }
        // Save response to socket additional for next read
        cnt->additional = (void *) resp;
    }

    // header
    if (!resp->version)
    {
        // Parse header
        head_len = parse_http_response(data, len, resp);
        if (head_len > 0)
        {
            ret += head_len;
        }
        else
        {
            // Header not enough
            return 0;
        }
    }

    // Body
    if (resp->transfer_encoding && 0 == strcasecmp("chunked", resp->transfer_encoding))
    {
        long int chunk_len = -1;
        int chunkstr_len = 0;
        while (ret < len)
        {
            // Chunked encoding
            if (chunk_len < 0)
            {
                for (i = ret + 1; i < len; i ++)
                {
                    // Read CRLF after chunk-size and chunk-ext
                    if (data[i - 1] == 0xd && data[i] == 0xa)
                    {
                        // First ? broken chunk or chunk ending
                        if (i == ret + 1)
                        {
                            ret += 2;
                        }
                        else
                        {
                            chunk_len = strtol(data + ret, NULL, 16);
                            chunkstr_len = i + 1 - ret;
                        }

                        break;
                    }
                }
            }

            if (chunk_len > 0)
            {
                if (chunk_len + ret + 2 <= len)
                {
                    http_response_append_content(resp, data + ret + chunkstr_len, chunk_len);
                    ret += chunkstr_len + chunk_len + 2;   // Additional CRLF at the end of chunk
                    chunk_len = -1;
                }
                else
                {
                    // Go on
                    break;
                }
            }
            else if (0 == chunk_len)
            {
                // Last chunk
                finished = 1;
                ret += 2;
                break;
            }
            else
            {
                // Otherwise
                break;
            }
        }
    }
    else if (resp->connection && 0 == strcasecmp("close", resp->connection))
    {
        // Server closed
        http_response_append_content(resp, data + ret, len - ret);
        ret = len;
    }
    else
    {
        // Waiting for data
        if ((len - ret) >= resp->content_length)
        {
            http_response_set_content(resp, data + ret, resp->content_length);
            ret += resp->content_length;
            finished = 1;
        }
    }

    // Body over, call here
    if (finished)
    {
        trace_msg(TRACE_LEVEL_VERBOSE, "HTTP-M : Request finished");
        _finish_http_resp(cnt);
        bsp_free(resp);
        cnt->additional = NULL;
        free_connector(cnt);
    }

    return ret;
}

static void _http_on_close(BSP_CONNECTOR *cnt)
{
    if (!cnt)
    {
        return;
    }

    // Check if "Connection : close", auto closing by HTTP server
    if (!cnt->additional)
    {
        return;
    }

    // Peer closed
    trace_msg(TRACE_LEVEL_DEBUG, "HTTP-M : HTTP peer close by remote server");
    _finish_http_resp(cnt);
    bsp_free(cnt->additional);

    return;
}

static int http_send_request(lua_State *s)
{
    if (!s || lua_gettop(s) < 2 || !lua_istable(s, -2))
    {
        return 0;
    }

    BSP_HTTP_REQUEST *req = new_http_request();
    if (!req)
    {
        return 0;
    }

    BSP_CONNECTOR *cnt = NULL;
    // Read table
    lua_getfield(s, -2, "version");
    if (lua_isstring(s, -1))
    {
        http_request_set_version(req, lua_tostring(s, -1), -1);
    }
    lua_pop(s, 1);
    lua_getfield(s, -2, "method");
    if (lua_isstring(s, -1))
    {
        http_request_set_method(req, lua_tostring(s, -1), -1);
    }
    lua_pop(s, 1);
    lua_getfield(s, -2, "useragent");
    if (lua_isstring(s, -1))
    {
        http_request_set_user_agent(req, lua_tostring(s, -1), -1);
    }
    lua_pop(s, 1);
    lua_getfield(s, -2, "host");
    if (lua_isstring(s, -1))
    {
        http_request_set_host(req, lua_tostring(s, -1), -1);
    }
    lua_pop(s, 1);
    lua_getfield(s, -2, "port");
    if (lua_isnumber(s, -1))
    {
        req->port = (int) lua_tointeger(s, -1);
    }
    lua_pop(s, 1);
    lua_getfield(s, -2, "uri");
    if (lua_isstring(s, -1))
    {
        http_request_set_request_uri(req, lua_tostring(s, -1), -1);
    }
    lua_pop(s, 1);
    lua_getfield(s, -2, "postdata");
    if (lua_isstring(s, -1))
    {
        size_t len = 0;
        const char *data = lua_tolstring(s, -1, &len);
        http_request_set_post_data(req, data, len);
    }
    lua_pop(s, 1);

    // Generate
    BSP_STRING *str = generate_http_request(req);
    if (!str)
    {
        return 0;
    }

    trace_msg(TRACE_LEVEL_DEBUG, "HTTP-M : Generate a HTTP request to host : %s, request_uri : %s", req->host, req->request_uri);
    // Send request
    if (req->host && req->request_uri)
    {
        cnt = new_connector(req->host, req->port, INET_TYPE_ANY, SOCK_TYPE_TCP);
        if (cnt)
        {
            cnt->on_data = _http_on_response;
            cnt->on_close = _http_on_close;
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
            append_data_socket(&SCK(cnt), str);
            flush_socket(&SCK(cnt));
        }
    }
    del_http_request(req);
    del_string(str);

    return 0;
}

/* URL-ENCODE / DECODE */
static int http_urlencode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_isstring(s, -1))
    {
        return 0;
    }

    size_t len, i;
    const char *input = lua_tolstring(s, -1, &len);
    unsigned char c;
    BSP_STRING *str = new_string(NULL, len * 2);
    for (i = 0; i < len; i ++)
    {
        c = (unsigned char) input[i];
        if (c == 0x20)
        {
            string_append(str, "+", 1);
        }
        else if (!isalnum(c) && strchr("_-.", c) == NULL)
        {
            string_printf(str, "%%%02X", c);
        }
        else
        {
            string_append(str, (const char *) &c, 1);
        }
    }

    lua_pushlstring(s, STR_STR(str), STR_LEN(str));
    del_string(str);

    return 1;
}

static int http_urldecode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_isstring(s, -1))
    {
        return 0;
    }

    size_t len, i;
    const char *input = lua_tolstring(s, -1, &len);
    unsigned char c;
    char tmp[3];
    BSP_STRING *str = new_string(NULL, len);
    for (i = 0; i < len; i ++)
    {
        c = (unsigned char) input[i];
        if (c == '+')
        {
            string_append(str, " ", 1);
        }

        else if (c == '%' && i < len - 2)
        {
            tmp[0] = input[i + 1];
            tmp[1] = input[i + 2];
            tmp[2] = 0;

            c = (unsigned char) strtol(tmp, NULL, 16);
            string_append(str, (const char *) &c, 1);
            i += 2;
        }

        else
        {
            string_append(str, input + i, 1);
        }
    }

    lua_pushlstring(s, STR_STR(str), STR_LEN(str));
    del_string(str);

    return 1;
}

int bsp_module_http(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    lua_pushcfunction(s, http_send_request);
    lua_setglobal(s, "bsp_http_send_request");

    lua_pushcfunction(s, http_urlencode);
    lua_setglobal(s, "bsp_http_urlencode");

    lua_pushcfunction(s, http_urldecode);
    lua_setglobal(s, "bsp_http_urldecode");

    lua_settop(s, 0);

    return 0;
}
