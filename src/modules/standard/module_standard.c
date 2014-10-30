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
    const char *data = NULL;
    // Make space for command_id and length
    BSP_OBJECT *obj = NULL;
    int fd_type = FD_TYPE_SOCKET_CLIENT;
    BSP_CLIENT *clt = NULL;

    if (lua_istable(s, -1) && lua_isnumber(s, -2))
    {
        obj = lua_stack_to_object(s);
        if (lua_isnumber(s, -3))
        {
            // CMD
            cmd = lua_tointeger(s, -2);
            client_fd = lua_tointeger(s, -3);
            clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
            ret = output_client_cmd(clt, cmd, obj);
        }
        else
        {
            // OBJ
            client_fd = lua_tointeger(s, -2);
            clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
            ret = output_client_obj(clt, obj);
        }

        del_object(obj);
    }
    else if (lua_isstring(s, -1) && lua_isnumber(s, -2))
    {
        // RAW
        data = (const char *) lua_tolstring(s, -1, &len);
        client_fd = lua_tointeger(s, -2);
        clt = (BSP_CLIENT *) get_fd(client_fd, &fd_type);
        ret = output_client_raw(clt, data, len);
    }
    else
    {
        // Send nothing
        ret = 0;
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
    debug_object(obj);
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

    unsigned char hash_val[MD5_DIGEST_LENGTH];
    char hash_str[MD5_DIGEST_LENGTH * 2];
    size_t len;
    const char *input;

    if (lua_gettop(s) > 1 && lua_isboolean(s, -1) && lua_isstring(s, -2))
    {
        input = lua_tolstring(s, -2, &len);
        MD5((const unsigned char *) input, (unsigned long) len, hash_val);
        if (lua_toboolean(s, -1) > 0)
        {
            int i;
            char bit[3];
            for (i = 0; i < MD5_DIGEST_LENGTH; i ++)
            {
                snprintf(bit, 3, "%02x", (unsigned char) hash_val[i]);
                hash_str[i * 2] = bit[0];
                hash_str[i * 2 + 1] = bit[1];
            }

            lua_pushlstring(s, (const char *) hash_str, MD5_DIGEST_LENGTH * 2);
        }

        else
        {
            lua_pushlstring(s, (const char *) hash_val, MD5_DIGEST_LENGTH);
        }
    }

    else if (lua_gettop(s) > 0 && lua_isstring(s, -1))
    {
        input = lua_tolstring(s, -1, &len);
        MD5((const unsigned char *) input, (unsigned long) len, hash_val);
        lua_pushlstring(s, (const char *) hash_val, MD5_DIGEST_LENGTH);
    }
    
    else
    {
        return 0;
    }

    return 1;
}

static int standard_sha1(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    unsigned char hash_val[SHA_DIGEST_LENGTH];
    char hash_str[SHA_DIGEST_LENGTH * 2];
    size_t len;
    const char *input;

    if (lua_gettop(s) > 1 && lua_isboolean(s, -1) && lua_isstring(s, -2))
    {
        input = lua_tolstring(s, -2, &len);
        SHA1((const unsigned char *) input, (unsigned long) len, hash_val);
        if (lua_toboolean(s, -1) > 0)
        {
            int i;
            char bit[3];
            for (i = 0; i < SHA_DIGEST_LENGTH; i ++)
            {
                snprintf(bit, 3, "%02x", (unsigned char) hash_val[i]);
                hash_str[i * 2] = bit[0];
                hash_str[i * 2 + 1] = bit[1];
            }

            lua_pushlstring(s, (const char *) hash_str, SHA_DIGEST_LENGTH * 2);
        }

        else
        {
            lua_pushlstring(s, (const char *) hash_val, SHA_DIGEST_LENGTH);
        }
    }

    else if (lua_gettop(s) > 0 && lua_isstring(s, -1))
    {
        input = lua_tolstring(s, -1, &len);
        SHA1((const unsigned char *) input, (unsigned long) len, hash_val);
        lua_pushlstring(s, (const char *) hash_val, SHA_DIGEST_LENGTH);
    }
    
    else
    {
        return 0;
    }

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

/** Global variable **/
struct _global_entry_t *global_hash_table[GLOBAL_HASH_SIZE];
BSP_SPINLOCK global_lock;
static void _global_init()
{
    bsp_spin_init(&global_lock);
    memset(global_hash_table, 0, sizeof(struct _global_entry_t *) * GLOBAL_HASH_SIZE);
    return;
}

static struct _global_entry_t * _global_new(const char *key)
{
    if (!key)
    {
        return NULL;
    }
    // Generate a new hash entry
    uint32_t hash_value = bsp_hash(key, -1);
    int slot = hash_value % GLOBAL_HASH_SIZE;
    struct _global_entry_t *g = bsp_calloc(1, sizeof(struct _global_entry_t));
    if (!g)
    {
        return NULL;
    }
    bsp_spin_lock(&global_lock);
    g->next = global_hash_table[slot];
    global_hash_table[slot] = g;
    bsp_spin_unlock(&global_lock);
    
    return g;
}

static struct _global_entry_t * _global_find(const char *key)
{
    if (!key)
    {
        return NULL;
    }
    // Search hash
    uint32_t hash_value = bsp_hash(key, -1);
    int slot = hash_value % GLOBAL_HASH_SIZE;
    bsp_spin_lock(&global_lock);
    struct _global_entry_t *g = global_hash_table[slot];
    while (g)
    {
        if (0 == strcmp(key, g->key))
        {
            break;
        }
        g = g->next;
    }
    bsp_spin_unlock(&global_lock);
    return g;
}

static int standard_set_global(lua_State *s)
{
    if (!s || lua_gettop(s) < 2 || !lua_isstring(s, -2))
    {
        return 0;
    }

    const char *key = lua_tostring(s, -2);
    if (!key)
    {
        return 0;
    }

    // Find exist
    struct _global_entry_t *g = _global_find(key);
    lua_Number vn;
    int vb;
    const char *vs;
    void *value;
    BSP_OBJECT *vt;
    
    if (!g)
    {
        g = _global_new(key);
        if (!g)
        {
            return 0;
        }
        // Set key
        strncpy(g->key, key, GLOBAL_NAME_LENGTH - 1);
        g->key[GLOBAL_NAME_LENGTH - 1] = 0x0;
    }
    else
    {
        // Clean value
        if (LUA_TSTRING == g->type)
        {
            memcpy(&value, g->value, sizeof(void *));
            if (value && g->value_len)
            {
                bsp_free(value);
                memset(g->value, 0, 8);
                g->value_len = 0;
            }
        }

        else if (LUA_TTABLE == g->type)
        {
            memcpy(&vt, g->value, sizeof(BSP_OBJECT *));
            if (vt)
            {
                del_object(vt);
                memset(g->value, 0, 8);
                g->value_len = 0;
            }
        }
    }
    
    switch (lua_type(s, -1))
    {
        case LUA_TNIL : 
            g->type = LUA_TNIL;
            break;
        case LUA_TNUMBER : 
            g->type = LUA_TNUMBER;
            vn = lua_tonumber(s, -1);
            memcpy(g->value, &vn, sizeof(lua_Number));
            break;
        case LUA_TBOOLEAN : 
            g->type = LUA_TBOOLEAN;
            vb = lua_toboolean(s, -1);
            g->value[0] = (char) vb;
            break;
        case LUA_TSTRING : 
            g->type = LUA_TSTRING;
            vs = lua_tolstring(s, -1, &g->value_len);
            value = bsp_malloc(g->value_len);
            if (!value)
            {
                return 0;
            }
            memcpy(value, vs, g->value_len);
            memcpy(g->value, &value, sizeof(void *));
            break;
        case LUA_TLIGHTUSERDATA : 
            g->type = LUA_TLIGHTUSERDATA;
            value = lua_touserdata(s, -1);
            memcpy(g->value, &value, sizeof(void *));
            break;
        case LUA_TTABLE : 
            g->type = LUA_TTABLE;
            vt = lua_stack_to_object(s);
            memcpy(g->value, &vt, sizeof(BSP_OBJECT *));
            break;
        case LUA_TFUNCTION : 
        case LUA_TUSERDATA : 
        case LUA_TTHREAD : 
        default : 
            // Chen Qie Zuo Bu Dao A!!!~~~
            g->type = LUA_TNONE;
            break;
    }
    
    return 0;
}

static int standard_get_global(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_isstring(s, -1))
    {
        return 0;
    }

    const char *key = lua_tostring(s, -1);
    if (!key)
    {
        return 0;
    }

    // Find exist
    struct _global_entry_t *g = _global_find(key);
    if (!g)
    {
        lua_pushnil(s);
        return 1;
    }
    lua_Number vn;
    int vb;
    const char *vs;
    void *value;
    BSP_OBJECT *vt;
    
    switch (g->type)
    {
        case LUA_TNIL : 
            lua_pushnil(s);
            break;
        case LUA_TNUMBER : 
            memcpy(&vn, g->value, sizeof(lua_Number));
            lua_pushnumber(s, vn);
            break;
        case LUA_TBOOLEAN : 
            vb = g->value[0];
            lua_pushboolean(s, vb);
            break;
        case LUA_TSTRING : 
            memcpy(&vs, g->value, sizeof(const char *));
            lua_pushlstring(s, vs, g->value_len);
            break;
        case LUA_TLIGHTUSERDATA : 
            memcpy(&value, g->value, sizeof(void *));
            lua_pushlightuserdata(s, value);
            break;
        case LUA_TTABLE : 
            memcpy(&vt, g->value, sizeof(BSP_OBJECT *));
            object_to_lua_stack(s, vt);
            break;
        default : 
            lua_pushnil(s);
            break;
    }
    
    return 1;
}

/* Module */
int bsp_module_standard(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    _global_init();
    
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

    lua_pushcfunction(s, standard_set_global);
    lua_setglobal(s, "bsp_set_global");

    lua_pushcfunction(s, standard_get_global);
    lua_setglobal(s, "bsp_get_global");

    lua_settop(s, 0);
    
    return 0;
}
