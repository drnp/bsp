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
        if (t)
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

static int bootstrap_set_entry(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    const char *callback_entry = lua_tostring(s, -1);
    if (!callback_entry)
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : No entry name specified");
        return 0;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    settings->script_callback_entry = strdup(callback_entry);

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
    if (!t)
    {
        trigger_exit(BSP_RTN_FATAL, "Cannot get main thread");
    }

    // Regist original load function
    lua_pushcfunction(t->script_runner.state, bootstrap_load_script);
    lua_setglobal(t->script_runner.state, "bsp_load_script");
    lua_pushcfunction(t->script_runner.state, bootstrap_set_entry);
    lua_setglobal(t->script_runner.state, "bsp_set_entry");

    // Change working location to script_dir
    BSP_CORE_SETTING *settings = get_core_setting();
    if (0 != chdir(settings->script_dir))
    {
        trace_msg(TRACE_LEVEL_ERROR, "BStrap : Cannot change current working directory to %s", settings->script_dir);
    }
    
    if (BSP_RTN_SUCCESS == script_load_string(t->script_runner.state, STR_STR(bs), STR_LEN(bs)))
    {
        // Run code
        script_call(t->script_runner.state, NULL, NULL);
    }
    else
    {
        trace_msg(TRACE_LEVEL_FATAL, "BStrap : Bootstrap load error");
        trigger_exit(BSP_RTN_FATAL, "Server halt");
    }

    lua_settop(t->script_runner.state, 0);
    
    return BSP_RTN_SUCCESS;
}
