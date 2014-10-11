/*
 * module_mysql.c
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
 * MySQL Database module
 * 
 * @package modules::mysql
 * @author Dr.NP <np@bsgroup.org>
 * @update 09/01/2012
 * @changelog 
 *      [09/01/2012] - Creation
 */

#include "bsp.h"

#include "module_mysql.h"

static int mysql_mysql_connect(lua_State *s)
{
    if (!s || lua_gettop(s) < 4)
    {
        return 0;
    }

    if (!lua_isstring(s, -1) || 
        !lua_isstring(s, -2) || 
        !lua_isstring(s, -3) || 
        !lua_isstring(s, -4))
    {
        return 0;
    }

    const char *db = lua_tostring(s, -1);
    const char *pass = lua_tostring(s, -2);
    const char *user = lua_tostring(s, -3);
    const char *host = lua_tostring(s, -4);

    BSP_DB_MYSQL *m = db_mysql_connect(host, user, pass, db);
    if (!m)
    {
        lua_pushnil(s);
    }

    else
    {
        lua_pushlightuserdata(s, (void *) m);
    }
    
    return 1;
}

static int mysql_mysql_close(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_islightuserdata(s, -1))
    {
        return 0;
    }
    
    BSP_DB_MYSQL *m = (BSP_DB_MYSQL *) lua_touserdata(s, -1);
    db_mysql_close(m);
    
    return 0;
}

static int mysql_mysql_free_result(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_islightuserdata(s, -1))
    {
        return 0;
    }

    // You must call me when you finished your query
    BSP_DB_MYSQL_RES *r = (BSP_DB_MYSQL_RES *) lua_touserdata(s, -1);
    db_mysql_free_result(r);

    return 0;
}

static int mysql_mysql_error(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MYSQL *m = (BSP_DB_MYSQL *) lua_touserdata(s, -1);
    const char *errmsg = db_mysql_error(m);
    lua_pushstring(s, errmsg);
    
    return 1;
}

static int mysql_mysql_query(lua_State *s)
{
    if (!s || lua_gettop(s) < 2)
    {
        return 0;
    }

    if (!lua_isstring(s, -1) || !lua_islightuserdata(s, -2))
    {
        return 0;
    }

    size_t len;
    const char *query = lua_tolstring(s, -1, &len);
    BSP_DB_MYSQL *m = (BSP_DB_MYSQL *) lua_touserdata(s, -2);
    BSP_DB_MYSQL_RES *r = db_mysql_query(m, query, len);

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

static int mysql_mysql_fetch_row(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MYSQL_RES *r = (BSP_DB_MYSQL_RES *) lua_touserdata(s, -1);
    BSP_OBJECT *row = db_mysql_fetch_row(r);

    if (row)
    {
        object_to_lua_stack(s, row);
        del_object(row);
    }

    else
    {
        lua_pushnil(s);
    }

    return 1;
}

static int mysql_mysql_num_rows(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_MYSQL_RES *r = (BSP_DB_MYSQL_RES *) lua_touserdata(s, -1);

    if (r && r->res)
    {
        lua_pushinteger(s, r->num_rows);
    }

    else
    {
        lua_pushinteger(s, 0);
    }
    
    return 1;
}

int bsp_module_mysql(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }
    lua_pushcfunction(s, mysql_mysql_connect);
    lua_setglobal(s, "bsp_mysql_connect");

    lua_pushcfunction(s, mysql_mysql_close);
    lua_setglobal(s, "bsp_mysql_close");

    lua_pushcfunction(s, mysql_mysql_free_result);
    lua_setglobal(s, "bsp_mysql_free_result");

    lua_pushcfunction(s, mysql_mysql_error);
    lua_setglobal(s, "bsp_mysql_error");

    lua_pushcfunction(s, mysql_mysql_query);
    lua_setglobal(s, "bsp_mysql_query");

    lua_pushcfunction(s, mysql_mysql_fetch_row);
    lua_setglobal(s, "bsp_mysql_fetch_row");

    lua_pushcfunction(s, mysql_mysql_num_rows);
    lua_setglobal(s, "bsp_mysql_num_rows");
    
    return 0;
}
