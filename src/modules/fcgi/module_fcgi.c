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

static int fcgi_send_request(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_istable(s, -1))
    {
        return 0;
    }
    
    BSP_FCGI_PARAMS p;
    memset(&p, 0, sizeof(BSP_FCGI_PARAMS));
    
    lua_getfield(s, -1, "QUERY_STRING");
    p.query_string = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "REQUEST_METHOD");
    p.request_method = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "CONTENT_TYPE");
    p.content_type = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "CONTENT_LENGTH");
    p.content_length = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_FILENAME");
    p.script_filename = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    lua_getfield(s, -1, "SCRIPT_NAME");
    p.script_name = lua_tostring(s, -1);
    lua_pop(s, 1);
    
    BSP_STRING *req = build_fcgi_request();
    
    return 0;
}

int bsp_module_fcgi(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }

    lua_pushcfunction(s, fcgi_send_request);
    lua_setglobal(s, "bsp_fcgi_send_request");
    
    lua_settop(s, 0);
    
    return 0;
}
