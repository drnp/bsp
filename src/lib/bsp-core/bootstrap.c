/*
 * bootstrap.c
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
 * Load and execute bootstrap code block
 * 
 * @package libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/08/2014
 * @changelog 
 *      [05/08/2014] - Creation
 */

#include "bsp.h"

static int bootstrap_load_script(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    const char *script_name = lua_tostring(s, -1);
    if (!script_name)
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : No script name specified");
        return 0;
    }

    BSP_THREAD *t;
    BSP_CORE_SETTING *settings = get_core_setting();
    int i;
    for (i = 0; i < settings->static_workers; i ++)
    {
        t = get_thread(i);
        if (t && t->script_runner.state)
        {
            if (LUA_OK == luaL_dofile(t->script_runner.state, script_name))
            {
                trace_msg(TRACE_LEVEL_VERBOSE, "BStrap : Load script %s to thread %d", script_name, i);
            }
            else
            {
                // Cannot load file
                trace_msg(TRACE_LEVEL_ERROR, "BStrap : Cannot load script %s to thread %d : %s", script_name, i, lua_tostring(t->script_runner.state, -1));
            }
        }
    }

    return 0;
}

// Set LUA entry function
static int bootstrap_set_lua_entry(lua_State *s)
{
    if (!s || !lua_isstring(s, -1) || !lua_isstring(s, -2))
    {
        return 0;
    }

    const char *server_name = lua_tostring(s, -2);
    const char *callback_entry = lua_tostring(s, -1);
    if (!callback_entry)
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : No entry name specified");
        return 0;
    }

    BSP_SERVER *srv = get_server(server_name);
    if (srv)
    {
        srv->lua_entry = bsp_strdup(callback_entry);
        trace_msg(TRACE_LEVEL_VERBOSE, "BStrap : Server %s LUA entry set", server_name);
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : Cannot find server %s", server_name);
    }

    return 0;
}

// Set FastCGI pass upstream
static int bootstrap_set_fcgi_upstream(lua_State *s)
{
    if (!s || !lua_istable(s, -1) || !lua_isstring(s, -2))
    {
        return 0;
    }

    const char *server_name = lua_tostring(s, -2);
    // Upstream
    BSP_SERVER *srv = get_server(server_name);
    if (!srv)
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : Cannot find server %s", server_name);
        return 0;
    }

    BSP_FCGI_UPSTREAM *upstream = new_fcgi_upstream(server_name);
    struct bsp_fcgi_upstream_entry_t *entry = NULL;
    const char *host;
    const char *sock;
    const char *script_filename;
    int port;
    int weight;
    lua_checkstack(s, 1);
    lua_pushnil(s);
    while (0 != lua_next(s, -2))
    {
        if (lua_istable(s, -1))
        {
            // New entry
            entry = bsp_calloc(1, sizeof(struct bsp_fcgi_upstream_entry_t));
            if (entry)
            {
                lua_getfield(s, -1, "host");
                host = lua_tostring(s, -1);
                lua_pop(s, 1);
                lua_getfield(s, -1, "sock");
                sock = lua_tostring(s, -1);
                lua_pop(s, 1);
                lua_getfield(s, -1, "script_filename");
                script_filename = lua_tostring(s, -1);
                lua_pop(s, 1);
                lua_getfield(s, -1, "port");
                port = lua_tointeger(s, -1);
                lua_pop(s, 1);
                lua_getfield(s, -1, "weight");
                weight = lua_tointeger(s, -1);
                lua_pop(s, 1);

                if (host)
                {
                    entry->host = bsp_strdup(host);
                }
                if (sock)
                {
                    entry->sock = bsp_strdup(sock);
                }
                if (script_filename)
                {
                    entry->script_filename = bsp_strdup(script_filename);
                }
                entry->port = port;
                entry->weight = weight;

                add_fcgi_server_entry(upstream, entry);
            }
        }
        lua_pop(s, 1);
    }
    srv->fcgi_upstream = upstream;
    trace_msg(TRACE_LEVEL_VERBOSE, "BStrap : Server %s FastCGI upstream set", server_name);

    return 0;
}

// Set online behavior
static int bootstrap_set_online_handler(lua_State *s)
{
    if (!s || !lua_istable(s, -1))
    {
        return 0;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    lua_getfield(s, -1, "load");
    if (lua_isfunction(s, -1))
    {
        // Set load behavior
        lua_setfield(s, LUA_REGISTRYINDEX, ONLINE_HANDLER_NAME_LOAD);
        trace_msg(TRACE_LEVEL_VERBOSE, "BStrap : Online handler <load> set");
    }
    else
    {
        lua_pop(s, 1);
    }

    lua_getfield(s, -1, "save");
    if (lua_isfunction(s, -1))
    {
        // Set save behavior
        lua_setfield(s, LUA_REGISTRYINDEX, ONLINE_HANDLER_NAME_SAVE);
        trace_msg(TRACE_LEVEL_VERBOSE, "BStrap : Online handler <save> set");
    }
    else
    {
        lua_pop(s, 1);
    }

    lua_getfield(s, -1, "autosave");
    if (lua_isnumber(s, -1))
    {
        // Set auto-save interval
        settings->online_autosave_interval = lua_tointeger(s, -1);
    }

    return 0;
}

// Start bootstrap
int start_bootstrap(BSP_STRING *bs)
{
    if (!bs)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    // Get main state
    BSP_THREAD *t = get_thread(MAIN_THREAD);
    if (!t || !t->script_runner.state)
    {
        trigger_exit(BSP_RTN_FATAL, "Cannot get main thread state");
    }

    // Regist original load function
    lua_pushcfunction(t->script_runner.state, bootstrap_load_script);
    lua_setglobal(t->script_runner.state, "bsp_load_script");
    lua_pushcfunction(t->script_runner.state, bootstrap_set_lua_entry);
    lua_setglobal(t->script_runner.state, "bsp_set_lua_entry");
    lua_pushcfunction(t->script_runner.state, bootstrap_set_fcgi_upstream);
    lua_setglobal(t->script_runner.state, "bsp_set_fcgi_upstream");
    lua_pushcfunction(t->script_runner.state, bootstrap_set_online_handler);
    lua_setglobal(t->script_runner.state, "bsp_set_online_handler");

    // Change working location to script_dir
    BSP_CORE_SETTING *settings = get_core_setting();
    if (0 != chdir(settings->script_dir))
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : Cannot change current working directory to %s", settings->script_dir);
    }

    if (BSP_RTN_SUCCESS == script_load_string(t->script_runner.state, bs))
    {
        // Run code
        script_call(&t->script_runner, NULL, NULL);
    }
    else
    {
        trace_msg(TRACE_LEVEL_FATAL, "BStrap : Bootstrap load error");
        trigger_exit(BSP_RTN_FATAL, "Server halt");
    }

    lua_settop(t->script_runner.state, 0);
    trace_msg(TRACE_LEVEL_DEBUG, "BSTrap : Bootstrap started");

    return BSP_RTN_SUCCESS;
}
