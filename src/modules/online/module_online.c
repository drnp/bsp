/*
 * module_online.c
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
 * Online list. Thread-safe for multi LUA states
 * 
 * @package module::online
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/15/2013
 * @changelog
 *      [08/15/2013] - Creation
 *      [09/30/2013] - New online list and shared global data area
 */

#include "bsp.h"

#include "module_online.h"

struct _online_entry_t *online_list = NULL;
size_t online_list_size = 0;
int max_client_id = 0;
BSP_SPINLOCK online_list_lock = BSP_SPINLOCK_INITIALIZER;

struct _online_info_map_t *online_info_list = NULL;
struct _online_info_map_t *online_info_map[ONLINE_INFO_HASH_SIZE];
size_t online_info_size = 0;

static void _shr_init()
{
    BSP_CORE_SETTING *settings = get_core_setting();
    int nfds = settings->max_fds;
    online_list = bsp_calloc(1, sizeof(struct _online_entry_t) * nfds);
    if (!online_list)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Online : Create online list error");
    }
    memset(online_info_map, 0, sizeof(struct _online_info_map_t *) * ONLINE_INFO_HASH_SIZE);
    online_list_size = nfds;
    trace_msg(TRACE_LEVEL_DEBUG, "Online : Online list initialized to %d", nfds);

    return;
}

static void _shr_hash_insert(struct _online_info_map_t *entry)
{
    int hkey;
    if (entry && entry->key)
    {
        hkey = bsp_hash(entry->key, -1) % ONLINE_INFO_HASH_SIZE;
        entry->next = online_info_map[hkey];
        online_info_map[hkey] = entry;
    }

    return;
}

static struct _online_info_map_t * _shr_hash_find(const char *key)
{
    if (!key)
    {
        return NULL;
    }

    int hkey = bsp_hash(key, -1) % ONLINE_INFO_HASH_SIZE;
    struct _online_info_map_t *ret = online_info_map[hkey];
    while (ret)
    {
        if (0 == strcmp(ret->key, key))
        {
            break;
        }
        ret = ret->next;
    }

    return ret;
}

static int shr_online_map(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_istable(s, -1))
    {
        return 0;
    }

    const char *key, *value;
    int offset = 0, len;
    struct _online_info_map_t *item;
    online_info_list = NULL;
    
    // Read list
    bsp_spin_lock(&online_list_lock);
    lua_checkstack(s, 1);
    lua_pushnil(s);
    while (0 != lua_next(s, -2))
    {
        lua_checkstack(s, 1);
        lua_pushvalue(s, -2);
        // Key
        key = lua_tostring(s, -1);
        lua_pop(s, 1);

        // Value
        value = lua_tostring(s, -1);
        lua_pop(s, 1);

        if (!key || !value || !strlen(key) || !strlen(value))
        {
            continue;
        }
        // New entry
        item = bsp_calloc(1, sizeof(struct _online_info_map_t));
        if (item)
        {
            // Key
            strncpy(item->key, key, ONLINE_MAP_KEY_LENGTH - 1);
            item->key[ONLINE_MAP_KEY_LENGTH - 1] = 0x0;
            item->offset = offset;
            item->next = NULL;
            // Type
            if (0 == strncasecmp(value, "i", 1))
            {
                // Integer
                item->type = BSP_VAL_INT;
                item->length = 9;
                // Check index
                if (0 == strncasecmp(value, "ii", 2))
                {
                    item->index = 1;
                    /* TODO : Generate B+ index */
                }
            }

            else if (0 == strncasecmp(value, "s", 1))
            {
                // String
                item->type = BSP_VAL_STRING;
                // Check length
                while (*value > 0 && (*value < 48 || *value > 57))
                {
                    value ++;
                }
                len = atoi(value);
                item->length = (len > 0) ? len : DEFAULT_ONLINE_INFO_STRING_LENGTH;
            }

            else
            {
                // Ignore me
                item->type = BSP_VAL_NULL;
                item->length = 0;
            }

            // Insert to hash
            _shr_hash_insert(item);
            item->lnext = online_info_list;
            online_info_list = item;
            
            offset += item->length;
            online_info_size += item->length;
        }
    }
    bsp_spin_unlock(&online_list_lock);
    trace_msg(TRACE_LEVEL_VERBOSE, "Online : Online info map marked to %d bytes", (int) online_info_size);

    return 0;
}

static int shr_online_property(lua_State *s)
{
    if (!s || lua_gettop(s) < 2)
    {
        return 0;
    }

    if (!lua_isnumber(s, -2) || !online_list || !online_info_size)
    {
        return 0;
    }

    int client_id = lua_tointeger(s, -2);
    const char *key;
    int v_int;
    const char *v_str;
    size_t v_str_len;
    struct _online_info_map_t *map = NULL;
    if (client_id < 0 || client_id >= online_list_size)
    {
        return 0;
    }

    bsp_spin_lock(&online_list_lock);
    if (client_id > max_client_id)
    {
        max_client_id = client_id;
    }
    bsp_spin_unlock(&online_list_lock);
    if (ONLINE_STATUS_UNINITIALIZED == online_list[client_id].status)
    {
        bsp_spin_init(&online_list[client_id].lock);
    }

    bsp_spin_lock(&online_list[client_id].lock);
    if (lua_istable(s, -1))
    {
        // Add or modify
        if (!online_list[client_id].info)
        {
            online_list[client_id].info = bsp_calloc(1, online_info_size);
        }
        online_list[client_id].status = ONLINE_STATUS_ONLINE;

        // Input data
        lua_pushnil(s);
        while (0 != lua_next(s, -2))
        {
            lua_checkstack(s, 1);
            lua_pushvalue(s, -2);
            key = lua_tostring(s, -1);
            lua_pop(s, 1);
            map = _shr_hash_find(key);
            if (map)
            {
                switch (map->type)
                {
                    case BSP_VAL_INT : 
                        // Integer
                        v_int = lua_tointeger(s, -1);
                        set_vint(v_int, online_list[client_id].info + map->offset);
                        break;
                    case BSP_VAL_STRING : 
                        v_str = lua_tolstring(s, -1, &v_str_len);
                        set_string(v_str, map->length, online_list[client_id].info + map->offset);
                        break;
                    default : 
                        // Unsupport type
                        break;
                }
            }

            lua_pop(s, 1);
        }

        trace_msg(TRACE_LEVEL_VERBOSE, "Online : Set online info of client %d", client_id);
    }
    else if (lua_isnil(s, -1))
    {
        // Remove from table
        if (online_list[client_id].info)
        {
            memset(online_list[client_id].info, 0, online_info_size);
            online_list[client_id].status = ONLINE_STATUS_OFFLINE;
        }

        trace_msg(TRACE_LEVEL_VERBOSE, "Online : Remove online info of client %d", client_id);
    }
    else
    {
        // Unknown type
        trace_msg(TRACE_LEVEL_ERROR, "Online : Unknown operation");
    }
    bsp_spin_unlock(&online_list[client_id].lock);
    
    return 0;
}

static int shr_online_list(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    const char *cond_key = NULL;
    const char *op_tmp = NULL;
    int cond_operator = 0;
    const char *cond_val_str = NULL;
    int cond_val_int = 0;
    struct _online_info_map_t *map = NULL;
    if (lua_istable(s, -1))
    {
        // Condition
        lua_pushinteger(s, 1);
        lua_rawget(s, -2);
        cond_key = lua_tostring(s, -1);
        lua_pop(s, 1);

        lua_pushinteger(s, 2);
        lua_rawget(s, -2);
        op_tmp = lua_tostring(s, -1);
        lua_pop(s, 1);

        if (0 == strcmp(op_tmp, "="))
        {
            // Equal
            cond_operator = BSP_OP_EQ;
        }

        else if (0 == strcmp(op_tmp, ">"))
        {
            // Greater than
            cond_operator = BSP_OP_GT;
        }

        else if (0 == strcmp(op_tmp, "<"))
        {
            // Little than
            cond_operator = BSP_OP_LT;
        }

        else if (0 == strcmp(op_tmp, ">=") || 0 == strcmp(op_tmp, "=>"))
        {
            // Greater or equal
            cond_operator = BSP_OP_GE;
        }

        else if (0 == strcmp(op_tmp, "<=") || 0 == strcmp(op_tmp, "=<"))
        {
            // Little or equal
            cond_operator = BSP_OP_LE;
        }

        else if (0 == strcmp(op_tmp, "<>") || 0 == strcmp(op_tmp, "><"))
        {
            // Not equal
            cond_operator = BSP_OP_NE;
        }

        else
        {
            // Unsupported operator
        }

        map = _shr_hash_find(cond_key);
        if (map && cond_operator > 0)
        {
            // Get value
            lua_pushinteger(s, 3);
            lua_rawget(s, -2);
            switch (map->type)
            {
                case BSP_VAL_INT : 
                    cond_val_int = lua_tointeger(s, -1);
                    break;
                case BSP_VAL_STRING : 
                    cond_val_str = lua_tostring(s, -1);
                    break;
                default : 
                    // Unknown data type
                    break;
            }
        }
    }
    // Each entry of list
    lua_newtable(s);
    /* TODO B+ index */
    int i, j;
    int ori_val_int;
    const char *ori_val_str;
    int inst;
    bsp_spin_lock(&online_list_lock);
    if (cond_key && cond_operator && map)
    {
        j = 1;
        // Condition
        for (i = 0; i <= max_client_id; i ++)
        {
            if (ONLINE_STATUS_ONLINE == online_list[i].status && online_list[i].info)
            {
                inst = 0;
                switch (map->type)
                {
                    case BSP_VAL_INT : 
                        ori_val_int = get_vint(online_list[i].info + map->offset, NULL);
                        switch (cond_operator)
                        {
                            case BSP_OP_EQ : 
                                inst = (ori_val_int == cond_val_int) ? 1 : 0;
                                break;
                            case BSP_OP_NE : 
                                inst = (ori_val_int != cond_val_int) ? 1 : 0;
                                break;
                            case BSP_OP_GT : 
                                inst = (ori_val_int > cond_val_int) ? 1 : 0;
                                break;
                            case BSP_OP_GE : 
                                inst = (ori_val_int >= cond_val_int) ? 1 : 0;
                                break;
                            case BSP_OP_LT : 
                                inst = (ori_val_int < cond_val_int) ? 1 : 0;
                                break;
                            case BSP_OP_LE : 
                                inst = (ori_val_int <= cond_val_int) ? 1 : 0;
                                break;
                            default : 
                                break;
                        }
                        break;
                    case BSP_VAL_STRING : 
                        ori_val_str = get_string(online_list[i].info + map->offset);
                        switch (cond_operator)
                        {
                            case BSP_OP_EQ : 
                                inst = (0 == strcmp(cond_val_str, ori_val_str)) ? 1 : 0;
                                break;
                            default : 
                                inst = (0 != strcmp(cond_val_str, ori_val_str)) ? 1 : 0;
                                break;
                        }
                        break;
                    default : 
                        break;
                }
                if (inst)
                {
                    lua_pushinteger(s, j);
                    lua_pushinteger(s, i);
                    lua_settable(s, -3);
                    j ++;
                }
            }
        }
    }

    else
    {
        j = 1;
        // All
        for (i = 0; i <= max_client_id; i ++)
        {
            if (ONLINE_STATUS_ONLINE == online_list[i].status)
            {
                lua_pushinteger(s, j);
                lua_pushinteger(s, i);
                lua_settable(s, -3);
                j ++;
            }
        }
    }
    bsp_spin_unlock(&online_list_lock);

    return 1;
}

static int shr_online_info(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    if (!lua_isnumber(s, -1))
    {
        return 0;
    }

    int client_id = lua_tointeger(s, -1);
    struct _online_info_map_t *map;
    if (client_id < 0 || client_id >= online_list_size)
    {
        return 0;
    }

    if (ONLINE_STATUS_ONLINE != online_list[client_id].status)
    {
        lua_pushnil(s);
    }
    else
    {
        bsp_spin_lock(&online_list[client_id].lock);
        // All properties
        lua_newtable(s);
        map = online_info_list;
        while (map)
        {
            lua_pushstring(s, map->key);
            switch (map->type)
            {
                case BSP_VAL_INT : 
                    lua_pushinteger(s, get_int32(online_list[client_id].info + map->offset));
                    break;
                case BSP_VAL_STRING : 
                    lua_pushstring(s, get_string(online_list[client_id].info + map->offset));
                    break;
                default : 
                    lua_pushnil(s);
                    break;
            }
            lua_settable(s, -3);
            map = map->lnext;
        }
        bsp_spin_unlock(&online_list[client_id].lock);
    }

    return 1;
}

int bsp_module_online(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    bsp_spin_lock(&online_list_lock);
    if (!online_list)
    {
        _shr_init();
    }
    bsp_spin_unlock(&online_list_lock);

    if (!online_list)
    {
        // Initialized error?
        trace_msg(TRACE_LEVEL_ERROR, "Online : Initialization error");
        return 0;
    }

    lua_pushcfunction(s, shr_online_map);
    lua_setglobal(s, "bsp_online_init");
                  
    lua_pushcfunction(s, shr_online_property);
    lua_setglobal(s, "bsp_set_online");

    lua_pushcfunction(s, shr_online_list);
    lua_setglobal(s, "bsp_online_list");

    lua_pushcfunction(s, shr_online_info);
    lua_setglobal(s, "bsp_online_info");
    
    lua_settop(s, 0);

    return 0;
}
