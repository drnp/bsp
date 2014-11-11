/*
 * module_standard.c
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
 * Standard core interfaces for LUA
 * 
 * @package modules::standard
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/26/2012
 * @changelog 
 *      [08/06/2012] - Creation
 *      [09/11/2012] - New functions
 *      [10/26/2012] - Boolean data type added
 *      [12/17/2013] - Lightuserdata supported
 */

#include "bsp.h"

#include "module_standard.h"

// Timer callback
void _timer_on_timer(BSP_TIMER *tmr)
{
    if (tmr)
    {
        script_call(&tmr->script_stack, NULL, NULL);
    }

    return;
}

void _timer_on_stop(BSP_TIMER *tmr)
{
    return;
}

/* LUA Functions */
/** Network **/
static int standard_net_send(lua_State *s)
{
    if (!s || lua_gettop(s) < 2)
    {
        return 0;
    }

    size_t len = 0, ret = 0;
    int client_fd = 0;
    int cmd = 0;
    const char *raw = NULL;
    // Make space for command_id and length
    BSP_OBJECT *obj = NULL;
    int fd_type = FD_TYPE_SOCKET_CLIENT;
    BSP_CLIENT *clt = NULL;
    int data_type = PACKET_TYPE_UNDEFINED;

    if (!lua_istable(s, 1) && !lua_isnumber(s, 1))
    {
        // Not valid client fd
        lua_pushnil(s);
        return 1;
    }

    if (lua_istable(s, 2) && 2 == lua_gettop(s))
    {
        // OBJ
        obj = lua_stack_to_object(s);
        data_type = PACKET_TYPE_OBJ;
    }
    else if (lua_isnumber(s, 2) && lua_istable(s, 3) && 3 == lua_gettop(s))
    {
        // CMD
        cmd = lua_tointeger(s, 2);
        obj = lua_stack_to_object(s);
        data_type = PACKET_TYPE_CMD;
    }
    else if (lua_isstring(s, 2) && 2 == lua_gettop(s))
    {
        // RAW
        raw = (const char *) lua_tolstring(s, 2, &len);
        data_type = PACKET_TYPE_RAW;
    }
    else
    {
        // Invalid
        lua_pushinteger(s, 0);
        return 1;
    }

    if (lua_isnumber(s, 1))
    {
        // Send to single client
        client_fd = lua_tointeger(s, 1);
        clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
        if (clt)
        {
            switch (data_type)
            {
                case PACKET_TYPE_RAW : 
                    output_client_raw(clt, raw, len);
                    break;
                case PACKET_TYPE_OBJ : 
                    output_client_obj(clt, obj);
                    break;
                case PACKET_TYPE_CMD : 
                    output_client_cmd(clt, cmd, obj);
                default : 
                    break;
            }
            ret = 1;
        }
    }
    else
    {
        // Send to group
        lua_checkstack(s, 3);
        lua_pushnil(s);
        while (0 != lua_next(s, 1))
        {
            if (lua_isnumber(s, -1))
            {
                client_fd = lua_tonumber(s, -1);
                clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
                if (clt)
                {
                    switch (data_type)
                    {
                        case PACKET_TYPE_RAW : 
                            output_client_raw(clt, raw, len);
                            break;
                        case PACKET_TYPE_OBJ : 
                            output_client_obj(clt, obj);
                            break;
                        case PACKET_TYPE_CMD : 
                            output_client_cmd(clt, cmd, obj);
                        default : 
                            break;
                    }
                    ret ++;
                }
            }
            lua_pop(s, 1);
        }
    }

    lua_pushinteger(s, ret);

    return 1;
}

static int standard_net_close(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    int client_fd = lua_tointeger(s, -1);
    int fd_type = FD_TYPE_SOCKET_CLIENT;
    BSP_CLIENT *clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
    if (clt)
    {
        free_client(clt);
    }

    return 0;
}

static int standard_net_info(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    char ipaddr[64];
    int client_fd = lua_tointeger(s, -1);
    int fd_type = FD_TYPE_SOCKET_CLIENT;
    BSP_CLIENT *clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
    if (clt)
    {
        // Info table
        lua_newtable(s);
        if (AF_INET6 == clt->sck.saddr.ss_family)
        {
            // IPv6
            struct sockaddr_in6 *clt_sin6 = (struct sockaddr_in6 *) &clt->sck.saddr;
            inet_ntop(AF_INET6, &clt_sin6->sin6_addr.s6_addr, ipaddr, 63);
            lua_pushstring(s, "ip_addr");
            lua_pushstring(s, ipaddr);
            lua_settable(s, -3);

            lua_pushstring(s, "port");
            lua_pushinteger(s, ntohs(clt_sin6->sin6_port));
            lua_settable(s, -3);
        }
        else
        {
            // IPv4
            struct sockaddr_in *clt_sin4 = (struct sockaddr_in *) &clt->sck.saddr;
            inet_ntop(AF_INET, &clt_sin4->sin_addr.s_addr, ipaddr, 63);
            lua_pushstring(s, "ip_addr");
            lua_pushstring(s, ipaddr);
            lua_settable(s, -3);

            lua_pushstring(s, "port");
            lua_pushinteger(s, ntohs(clt_sin4->sin_port));
            lua_settable(s, -3);
        }
    }
    else
    {
        lua_pushnil(s);
    }

    return 1;
}

/** Timer **/
static int standard_timer_create(lua_State *s)
{
    if (!s || lua_gettop(s) < 4)
    {
        return 0;
    }

    //const char *func = strdup(lua_tostring(s, -1));
    int loop = lua_tointeger(s, -2);
    int usec = lua_tointeger(s, -3);
    int sec = lua_tointeger(s, -4);

    if (!lua_isfunction(s, -1) || !loop || (!usec && !sec))
    {
        lua_settop(s, 0);
        return 0;
    }

    BSP_TIMER *tmr= new_timer((time_t) sec, (long) usec, loop);
    if (!tmr)
    {
        lua_settop(s, 0);
        return 0;
    }

    tmr->on_timer = _timer_on_timer;
    tmr->on_stop = _timer_on_stop;
    dispatch_to_thread(tmr->fd, curr_thread_id());
    // Push function into timer
    lua_xmove(s, tmr->script_stack.stack, 1);
    start_timer(tmr);

    lua_pushinteger(s, tmr->fd);

    return 1;
}

static int standard_timer_delete(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    int fd = lua_tointeger(s, -1);
    int fd_type = FD_TYPE_TIMER;
    BSP_TIMER *tmr = (BSP_TIMER *) get_fd(fd, &fd_type);
    if (!tmr)
    {
        return 0;
    }

    free_timer(tmr);

    return 0;
}

/** Variable operation **/
static int standard_var_dump(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    int i;
    static int layer = 0;
    const char *key;
    switch (lua_type(s, -1))
    {
        case LUA_TNIL : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;33m[NIL]\033[0m\n");
            break;
        case LUA_TBOOLEAN : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;32m[BOOLEAN]\033[0m : %s\n", lua_toboolean(s, -1) > 0 ? "true" : "false");
            break;
        case LUA_TNUMBER : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;36m[NUMBER]\033[0m : %g\n", lua_tonumber(s, -1));
            break;
        case LUA_TSTRING : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;35m[STRING]\033[0m : %s\n", lua_tostring(s, -1));
            break;
        case LUA_TTABLE : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;34m[TABLE]\033[0m => {\n");
            layer ++;
            lua_checkstack(s, 1);
            lua_pushnil(s);
            while (0 != lua_next(s, -2))
            {
                lua_checkstack(s, 1);
                lua_pushvalue(s, -2);
                // Key
                key = lua_tostring(s, -1);
                for (i = 0; i < layer; i ++)
                {
                    fprintf(stderr, "\t");
                }
                fprintf(stderr, "%s => \n", key);
                lua_pop(s, 1);

                // Value
                standard_var_dump(s);
                lua_pop(s, 1);
            }
            layer --;
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }
            fprintf(stderr, "}\n");
            break;
        case LUA_TLIGHTUSERDATA : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;34m[POINTER]\033[0m => %p\n", lua_touserdata(s, -1));
            break;
        default : 
            for (i = 0; i < layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            fprintf(stderr, "\033[1;33m[UNKNOWN]\033[0m\n");
            break;
    }
    
    return 0;
}

static int standard_deep_copy(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_istable(s, -1))
    {
        return 0;
    }

    // A new table
    lua_checkstack(s, 1);
    lua_newtable(s);

    // Loop each item
    lua_checkstack(s, 1);
    lua_pushnil(s);
    while (0 != lua_next(s, -3))
    {
        // New table?
        if (lua_type(s, -1) == LUA_TTABLE)
        {
            standard_deep_copy(s);
            lua_checkstack(s, 2);
            lua_pushvalue(s, -3);
            lua_pushvalue(s, -2);
            // Swap position
            lua_remove(s, -3);
        }
        else
        {
            lua_checkstack(s, 2);
            // New key
            lua_pushvalue(s, -2);
            // New value
            lua_pushvalue(s, -2);
        }

        lua_settable(s, -5);
        lua_pop(s, 1);
    }

    return 1;
}

/** Base64 encoding **/
static int standard_base64_encode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_isstring(s, -1))
    {
        return 0;
    }

    size_t len;
    const char *input = lua_tolstring(s, -1, &len);
    BSP_STRING *str = string_base64_encode(input, len);
    lua_pushlstring(s, STR_STR(str), STR_LEN(str));
    del_string(str);

    return 1;
}

static int standard_base64_decode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_isstring(s, -1))
    {
        return 0;
    }

    size_t len;
    const char *input = lua_tolstring(s, -1, &len);
    BSP_STRING *str = string_base64_decode(input, len);
    lua_pushlstring(s, STR_STR(str), STR_LEN(str));
    del_string(str);

    return 1;
}

/** JSON **/
static int standard_json_encode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    BSP_OBJECT *obj = lua_stack_to_object(s);
    BSP_STRING *json = json_nd_encode(obj);
    lua_pushlstring(s, STR_STR(json), STR_LEN(json));
    del_object(obj);
    del_string(json);

    return 1;
}

static int standard_json_decode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_isstring(s, -1))
    {
        return 0;
    }

    size_t len;
    const char *str = lua_tolstring(s, -1, &len);
    BSP_STRING *json = new_string_const(str, len);
    BSP_OBJECT *obj = json_nd_decode(json);
    object_to_lua_stack(s, obj);
    del_string(json);
    del_object(obj);

    return 1;
}

/** Hash **/
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

static int standard_md5(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    int raw = 0;
    size_t len;
    const char *input;

    if (lua_gettop(s) > 1 && lua_isboolean(s, -1) && lua_isstring(s, -2))
    {
        input = lua_tolstring(s, -2, &len);
        if (lua_toboolean(s, -1) > 0)
        {
            // Need raw data
            raw = 1;
        }
    }
    else if (lua_gettop(s) > 0 && lua_isstring(s, -1))
    {
        input = lua_tolstring(s, -1, &len);
    }
    else
    {
        return 0;
    }

    BSP_STRING *output = string_md5(input, len, raw);
    lua_pushlstring(s, STR_STR(output), STR_LEN(output));
    del_string(output);

    return 1;
}

static int standard_sha1(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    int raw = 0;
    size_t len;
    const char *input;

    if (lua_gettop(s) > 1 && lua_isboolean(s, -1) && lua_isstring(s, -2))
    {
        input = lua_tolstring(s, -2, &len);
        if (lua_toboolean(s, -1) > 0)
        {
            // Need raw data
            raw = 1;
        }
    }
    else if (lua_gettop(s) > 0 && lua_isstring(s, -1))
    {
        input = lua_tolstring(s, -1, &len);
    }
    else
    {
        return 0;
    }

    BSP_STRING *output = string_sha1(input, len, raw);
    lua_pushlstring(s, STR_STR(output), STR_LEN(output));
    del_string(output);

    return 1;
}

static int standard_hash(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    size_t len;
    const char *input = lua_tolstring(s, -1, &len);
    uint32_t hash_value = bsp_hash(input, len);
    lua_pushinteger(s, (lua_Integer) hash_value);

    return 1;
}

/** Randomize **/
static int standard_random_seed(lua_State *s)
{
    if (!s)
    {
        return 0;
    }

    int32_t val;
    char str[4];
    get_rand(str, 4);
    val = get_int32((const char *) str);
    lua_pushinteger(s, val);

    return 1;
}

/** Generate log **/
static int standard_log_open(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_isstring(s, -1))
    {
        return 0;
    }

    const char *filename = lua_tostring(s, -1);
    if (!filename)
    {
        return 0;
    }

    FILE *f = fopen(filename, "a");
    if (f)
    {
        reg_fd(fileno(f), FD_TYPE_GENERAL, NULL);
        lua_pushlightuserdata(s, f);
    }

    else
    {
        lua_pushnil(s);
    }

    return 1;
}

static int standard_log_close(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    FILE *f = (FILE *) lua_touserdata(s, -1);
    int fd_type = FD_TYPE_ANY;
    get_fd(fileno(f), &fd_type);
    if (FD_TYPE_GENERAL == fd_type)
    {
        unreg_fd(fileno(f));
        fclose(f);
    }

    return 0;
}

static int standard_log(lua_State *s)
{
    if (!s || lua_gettop(s) < 2 || !lua_isstring(s, -1) || !lua_islightuserdata(s, -2))
    {
        return 0;
    }

    FILE *f = (FILE *) lua_touserdata(s, -2);
    const char *content = lua_tostring(s, -1);
    if (!f || !content)
    {
        return 0;
    }

    char tgdate[64];
    time_t now = time(NULL);
    struct tm *loctime = localtime(&now);
    strftime(tgdate, 64, "%m/%d/%Y %H:%M:%S", loctime);
    fprintf(f, "[%s] : %s\n", tgdate, content);

    return 0;
}

/** Memdb **/
static int standard_set_cache(lua_State *s)
{
    return 0;
}

static int standard_get_cache(lua_State *s)
{
    return 1;
}

/** Online **/
static int standard_set_online(lua_State *s)
{
    if (!s || !lua_isnumber(s, -2))
    {
        return 0;
    }

    const char *key = lua_tostring(s, -1);
    int fd = lua_tointeger(s, -2);
    new_online(fd, key);
    load_online_data_by_key(key);

    return 0;
}

static int standard_set_offline(lua_State *s)
{
    if (!s)
    {
        return 0;
    }

    if (lua_isnumber(s, -1))
    {
        int fd = lua_tointeger(s, -1);
        save_online_data_by_bind(fd);
        del_online_by_bind(fd);
    }
    else
    {
        const char *key = lua_tostring(s, -1);
        save_online_data_by_key(key);
        del_online_by_key(key);
    }

    return 0;
}

static int standard_online_data(lua_State *s)
{
    if (!s)
    {
        return 0;
    }

    BSP_OBJECT *data = NULL;
    if (lua_isnumber(s, -1))
    {
        int fd = lua_tointeger(s, -1);
        data = get_online_data_by_bind(fd);
    }
    else
    {
        const char *key = lua_tostring(s, -1);
        data = get_online_data_by_key(key);
    }

    lua_checkstack(s, 1);
    object_to_lua_stack(s, data);

    return 1;
}

static int standard_online_list(lua_State *s)
{
    if (!s)
    {
        return 0;
    }

    BSP_OBJECT *list = get_online_list(NULL);
    lua_checkstack(s, 1);
    object_to_lua_stack(s, list);
    del_object(list);

    return 1;
}

/* Module */
int bsp_module_standard(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    lua_pushcfunction(s, standard_net_send);
    lua_setglobal(s, "bsp_net_send");

    lua_pushcfunction(s, standard_net_close);
    lua_setglobal(s, "bsp_net_close");

    lua_pushcfunction(s, standard_net_info);
    lua_setglobal(s, "bsp_net_info");

    lua_pushcfunction(s, standard_timer_create);
    lua_setglobal(s, "bsp_timer_create");

    lua_pushcfunction(s, standard_timer_delete);
    lua_setglobal(s, "bsp_timer_delete");

    lua_pushcfunction(s, standard_var_dump);
    lua_setglobal(s, "bsp_var_dump");

    lua_pushcfunction(s, standard_deep_copy);
    lua_setglobal(s, "bsp_deep_copy");

    lua_pushcfunction(s, standard_base64_encode);
    lua_setglobal(s, "bsp_base64_encode");

    lua_pushcfunction(s, standard_base64_decode);
    lua_setglobal(s, "bsp_base64_decode");

    lua_pushcfunction(s, standard_json_encode);
    lua_setglobal(s, "bsp_json_encode");

    lua_pushcfunction(s, standard_json_decode);
    lua_setglobal(s, "bsp_json_decode");

    lua_pushcfunction(s, standard_md5);
    lua_setglobal(s, "bsp_md5");

    lua_pushcfunction(s, standard_sha1);
    lua_setglobal(s, "bsp_sha1");

    lua_pushcfunction(s, standard_hash);
    lua_setglobal(s, "bsp_hash");

    lua_pushcfunction(s, standard_random_seed);
    lua_setglobal(s, "bsp_random_seed");

    lua_pushcfunction(s, standard_log_open);
    lua_setglobal(s, "bsp_log_open");

    lua_pushcfunction(s, standard_log_close);
    lua_setglobal(s, "bsp_log_close");

    lua_pushcfunction(s, standard_log);
    lua_setglobal(s, "bsp_log");

    lua_pushcfunction(s, standard_set_cache);
    lua_setglobal(s, "bsp_set_cache");

    lua_pushcfunction(s, standard_get_cache);
    lua_setglobal(s, "bsp_get_cache");

    lua_pushcfunction(s, standard_set_online);
    lua_setglobal(s, "bsp_set_online");

    lua_pushcfunction(s, standard_set_offline);
    lua_setglobal(s, "bsp_set_offline");

    lua_pushcfunction(s, standard_online_data);
    lua_setglobal(s, "bsp_online_data");

    lua_pushcfunction(s, standard_online_list);
    lua_setglobal(s, "bsp_online_list");

    lua_settop(s, 0);

    return 0;
}
