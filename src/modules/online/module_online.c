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

static int online_set_online(lua_State *s)
{
    if (!s || !lua_isstring(s, -1) || !lua_isnumber(s, -2))
    {
        return 0;
    }

    int fd = lua_tointeger(s, -2);
    const char *key = lua_tostring(s, -1);

    new_online(fd, key);

    return 1;
}

static int online_set_offline_by_fd(lua_State *s)
{
    if (!s || !lua_isnumber(s, -1))
    {
        return 0;
    }

    int fd = lua_tointeger(s, -1);
    del_online_by_bind(fd);

    return 0;
}

static int online_set_offline_by_key(lua_State *s)
{
    if (!s || !lua_isstring(s, -1))
    {
        return 0;
    }

    const char *key = lua_tostring(s, -1);
    del_online_by_key(key);

    return 0;
}

static int online_get_data(lua_State *s)
{
    if (!s || !lua_isstring(s, -1))
    {
        return 0;
    }

    return 0;
}

int bsp_module_online(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    lua_pushcfunction(s, online_set_online);
    lua_setglobal(s, "bsp_set_online");
                  
    lua_pushcfunction(s, online_set_offline_by_fd);
    lua_setglobal(s, "bsp_set_offline");

    lua_pushcfunction(s, online_set_offline_by_key);
    lua_setglobal(s, "bsp_set_offline_by_key");

    lua_pushcfunction(s, online_get_data);
    lua_setglobal(s, "bsp_get_online");
    
    lua_settop(s, 0);

    return 0;
}
