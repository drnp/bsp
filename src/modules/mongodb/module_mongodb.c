/*
 * module_mongodb.c
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
 * MongoDB lua binding
 * 
 * @package module::mongodb
 * @author Dr.NP <np@bsgroup.org>
 * @update 11/03/2014
 * @changelog 
 *      [11/03/2014] - Creation
 */

#include "bsp.h"

#include "module_mongodb.h"

static int mongodb_mongo_connect(lua_State *s)
{
    if (!s || !lua_isnumber(s, -1) || !lua_isstring(s, -2))
    {
        return 0;
    }

    int port = lua_tointeger(s, -1);
    const char *host = lua_tostring(s, -2);
    BSP_DB_MONGODB *m = db_mongodb_connect(host, port);
    lua_checkstack(s, 1);
    if (m)
    {
        lua_pushlightuserdata(s, (void *) m);
    }
    else
    {
        lua_pushnil(s);
    }

    return 1;
}

static int mongodb_mongo_close(lua_State *s)
{
    if (!s || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MONGODB *m = (BSP_DB_MONGODB *) lua_touserdata(s, -1);
    db_mongodb_close(m);

    return 0;
}

static int mongodb_mongo_free_result(lua_State *s)
{
    if (!s || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MONGODB_RES *r = (BSP_DB_MONGODB_RES *) lua_touserdata(s, -1);
    db_mongodb_free_result(r);

    return 0;
}

static int mongodb_mongo_error(lua_State *s)
{
    if (!s || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MONGODB *m = (BSP_DB_MONGODB *) lua_touserdata(s, -1);
    const char *msg = db_mongodb_error(m);
    lua_checkstack(s, 1);
    if (msg)
    {
        lua_pushstring(s, msg);
    }
    else
    {
        lua_pushnil(s);
    }

    return 1;
}

static int mongodb_mongo_update(lua_State *s)
{
    if (!s || !lua_istable(s, -1) || !lua_istable(s, -2) || !lua_isstring(s, -3) || !lua_islightuserdata(s, -4))
    {
        return 0;
    }

    BSP_DB_MONGODB *m = (BSP_DB_MONGODB *) lua_touserdata(s, -4);
    const char *ns = lua_tostring(s, -3);
    BSP_OBJECT *query = lua_stack_to_object(s);
    lua_pop(s, 1);
    BSP_OBJECT *obj = lua_stack_to_object(s);

    int ret = db_mongodb_update(m, ns, obj, query);
    lua_checkstack(s, 1);
    if (BSP_RTN_SUCCESS == ret)
    {
        lua_pushboolean(s, BSP_BOOLEAN_TRUE);
    }
    else
    {
        lua_pushboolean(s, BSP_BOOLEAN_FALSE);
    }
    del_object(query);
    del_object(obj);

    return 1;
}

static int mongodb_mongo_delete(lua_State *s)
{
    if (!s || !lua_istable(s, -1) || !lua_isstring(s, -2) || !lua_islightuserdata(s, -3))
    {
        return 0;
    }

    BSP_DB_MONGODB *m = (BSP_DB_MONGODB *) lua_touserdata(s, -3);
    const char *ns = lua_tostring(s, -2);
    BSP_OBJECT *query = lua_stack_to_object(s);

    int ret = db_mongodb_delete(m, ns, query);
    lua_checkstack(s, 1);
    if (BSP_RTN_SUCCESS == ret)
    {
        lua_pushboolean(s, BSP_BOOLEAN_TRUE);
    }
    else
    {
        lua_pushboolean(s, BSP_BOOLEAN_FALSE);
    }
    del_object(query);

    return 1;
}

static int mongodb_mongo_query(lua_State *s)
{
    if (!s || !lua_istable(s, -1) || !lua_isstring(s, -2) || !lua_islightuserdata(s, -3))
    {
        return 0;
    }

    BSP_DB_MONGODB *m = (BSP_DB_MONGODB *) lua_touserdata(s, -3);
    const char *ns = lua_tostring(s, -2);
    BSP_OBJECT *query = lua_stack_to_object(s);

    BSP_DB_MONGODB_RES *r = db_mongodb_query(m, ns, query);
    lua_checkstack(s, 1);
    if (r)
    {
        lua_pushlightuserdata(s, (void *) r);
    }
    else
    {
        lua_pushnil(s);
    }

    return 1;
}

static int mongodb_mongo_fetch_doc(lua_State *s)
{
    if (!s || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MONGODB_RES *r = (BSP_DB_MONGODB_RES *) lua_touserdata(s, -1);
    BSP_OBJECT *obj = db_mongodb_fetch_doc(r);
    lua_checkstack(s, 1);
    if (obj)
    {
        object_to_lua_stack(s, obj);
    }
    else
    {
        lua_pushnil(s);
    }

    return 1;
}

int bsp_module_mongodb(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    lua_pushcfunction(s, mongodb_mongo_connect);
    lua_setglobal(s, "bsp_mongodb_connect");

    lua_pushcfunction(s, mongodb_mongo_close);
    lua_setglobal(s, "bsp_mongodb_close");

    lua_pushcfunction(s, mongodb_mongo_free_result);
    lua_setglobal(s, "bsp_mongodb_free_result");

    lua_pushcfunction(s, mongodb_mongo_error);
    lua_setglobal(s, "bsp_mongodb_error");

    lua_pushcfunction(s, mongodb_mongo_update);
    lua_setglobal(s, "bsp_mongodb_update");

    lua_pushcfunction(s, mongodb_mongo_delete);
    lua_setglobal(s, "bsp_mongodb_delete");

    lua_pushcfunction(s, mongodb_mongo_query);
    lua_setglobal(s, "bsp_mongodb_query");

    lua_pushcfunction(s, mongodb_mongo_fetch_doc);
    lua_setglobal(s, "bsp_mongodb_fetch_doc");

    return 0;
}
