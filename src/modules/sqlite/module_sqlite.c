/*
 * module_sqlite.c
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
 * SQLITE lua binding
 * 
 * @package modules::sqlite
 * @author Dr.NP <np@bsgroup.org>
 * @update 09/20/2012
 * @changelog 
 *      [09/20/2012] - Creation
 */

#include "bsp.h"

#include "module_sqlite.h"

static int sqlite_sqlite_open(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_isstring(s, -1))
    {
        return 0;
    }

    const char *dbfile = lua_tostring(s, -1);
    BSP_DB_SQLITE *l = db_sqlite_open(dbfile);

    if (!l)
    {
        lua_pushnil(s);
    }

    lua_pushlightuserdata(s, (void *) l);

    return 1;
}

static int sqlite_sqlite_close(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_SQLITE *l = (BSP_DB_SQLITE *) lua_touserdata(s, -1);
    db_sqlite_close(l);

    return 0;
}

static int sqlite_sqlite_free_result(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_SQLITE_RES *r = (BSP_DB_SQLITE_RES *) lua_touserdata(s, -1);
    db_sqlite_free_result(r);

    return 0;
}

static int sqlite_sqlite_error(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_SQLITE *l = (BSP_DB_SQLITE *) lua_touserdata(s, -1);
    const char *errmsg = db_sqlite_error(l);
    lua_pushstring(s, errmsg);

    return 1;
}

static int sqlite_sqlite_query(lua_State *s)
{
    if (!s || lua_gettop(s) < 2 || !lua_isstring(s, -1) || !lua_islightuserdata(s, -2))
    {
        return 0;
    }

    size_t len;
    const char *query = lua_tolstring(s, -1, &len);
    BSP_DB_SQLITE *l = (BSP_DB_SQLITE *) lua_touserdata(s, -2);
    BSP_DB_SQLITE_RES *r = db_sqlite_query(l, query, len);

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

static int sqlite_sqlite_fetch_row(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    BSP_DB_SQLITE_RES *r = (BSP_DB_SQLITE_RES *) lua_touserdata(s, -1);
    BSP_OBJECT *row = db_sqlite_fetch_row(r);

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

int bsp_module_sqlite(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }
    lua_pushcfunction(s, sqlite_sqlite_open);
    lua_setglobal(s, "bsp_sqlite_open");

    lua_pushcfunction(s, sqlite_sqlite_close);
    lua_setglobal(s, "bsp_sqlite_close");

    lua_pushcfunction(s, sqlite_sqlite_free_result);
    lua_setglobal(s, "bsp_sqlite_free_result");

    lua_pushcfunction(s, sqlite_sqlite_error);
    lua_setglobal(s, "bsp_sqlite_error");

    lua_pushcfunction(s, sqlite_sqlite_query);
    lua_setglobal(s, "bsp_sqlite_query");

    lua_pushcfunction(s, sqlite_sqlite_fetch_row);
    lua_setglobal(s, "bsp_sqlite_fetch_row");
    
    return 0;
}
