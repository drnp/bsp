/*
 * script.c
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
 * LUA script runner
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/14/2013
 * @changelog 
 *      [07/02/2012] - Creation
 *      [08/23/2012] - Stack space check in script_call()
 *      [08/27/2012] - on_load_hook callback added
 *      [07/05/2013] - Only one script per implementation
 *      [08/14/2013] - Remove multi thread
 *      [12/17/2013] - Yieldable thread supported
 */

#define _GNU_SOURCE

#include "bsp.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <ltdl.h>

// Unique script
struct bsp_script_t *script = NULL;
BSP_STRING *code = NULL;

// Memory allocator for LUA
static void * _default_allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void) ud;

    if (0 == nsize)
    {
        status_op_script(STATUS_OP_SCRIPT_MEMORY_FREE, osize);
        bsp_free(ptr);
        return NULL;
    }
    else
    {
        status_op_script(STATUS_OP_SCRIPT_MEMORY_ALLOC, nsize);
        return bsp_realloc(ptr, nsize);
    }
}

// Load script code block
int script_load_string(lua_State *l, BSP_STRING *code)
{
    if (!l || !code)
    {
        return BSP_RTN_ERROR_SCRIPT;
    }

    int ret = luaL_loadbufferx(l, STR_STR(code), STR_LEN(code), NULL, NULL);
    switch (ret)
    {
        case LUA_OK : 
            return BSP_RTN_SUCCESS;
            break;
        case LUA_ERRSYNTAX : 
            trace_msg(TRACE_LEVEL_ERROR, "Lua script syntax error : %s", lua_tostring(l, -1));
            return BSP_RTN_ERROR_SCRIPT;
            break;
        case LUA_ERRMEM : 
            trace_msg(TRACE_LEVEL_ERROR, "Memory error to load Lua script");
            return BSP_RTN_ERROR_MEMORY;
            break;
        case LUA_ERRGCMM : 
        default : 
            trigger_exit(BSP_RTN_FATAL, "Script error");
            break;
    }

    return BSP_RTN_ERROR_GENERAL;
}

// Call a script function or resume coruntine
int script_call(lua_State *caller, const char *func, BSP_OBJECT *p)
{
    int ret, status, nargs = 0;

    if (!caller)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Not available state");
        return BSP_RTN_ERROR_SCRIPT;
    }

    if (func)
    {
        lua_checkstack(caller, 1);
        lua_getglobal(caller, func);
    }

    if (!lua_isfunction(caller, -1))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : No function specified");
    }
    else
    {
        if (p)
        {
            object_to_lua_stack(caller, p);
            nargs = 1;
        }
        status = lua_status(caller);
        if (LUA_OK == status)
        {
            // Pcall
            ret = lua_pcall(caller, nargs, 0, 0);
            trace_msg(TRACE_LEVEL_VERBOSE, "Script : Call a stack");
        }
        else if (LUA_YIELD == status)
        {
            // Resume
            ret = lua_resume(caller, NULL, nargs);
            trace_msg(TRACE_LEVEL_VERBOSE, "Script : Resume a yielded stack");
        }
        else
        {
            // We cannot run
            trace_msg(TRACE_LEVEL_ERROR, "Script : Stack status error, cannot run any more");
            ret = LUA_OK;
        }
        
        if (LUA_YIELD != ret && LUA_OK != ret)
        {
            status_op_script(STATUS_OP_SCRIPT_FAILURE, 0);
            trace_msg(TRACE_LEVEL_ERROR, "Script : Call lua function error : %s", lua_tostring(caller, -1));
        }
        else
        {
            trace_msg(TRACE_LEVEL_DEBUG, "Script : Stack call successfully");
        }
        status_op_script(STATUS_OP_SCRIPT_CALL, 0);
    }
    lua_settop(caller, 0);

    return BSP_RTN_SUCCESS;
}

// New state(runner), create a new LUA state
int script_new_state(BSP_SCRIPT_STATE *ss)
{
    if (!ss)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    lua_State *state = lua_newstate(_default_allocator, NULL);
    if (!state)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Create new script state error");
        return BSP_RTN_ERROR_SCRIPT;
    }
    luaL_openlibs(state);
    ss->state = state;
    ss->load_times = 0;
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Create a new script state");
    status_op_script(STATUS_OP_SCRIPT_STATE_ADD, 0);

    return BSP_RTN_SUCCESS;
}

// New stack(caller), create a new LUA thread and bind to state
// The new stack can bind to a socket client
int script_new_stack(BSP_SCRIPT_STACK *ts)
{
    if (!ts || !ts->state)
    {
        return -1;
    }

    lua_State *stack = lua_newthread(ts->state);
    if (!stack)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Create new LUA thread error");
        return -1;
    }

    ts->stack = stack;
    int ref = ts->stack_ref = luaL_ref(ts->state, LUA_REGISTRYINDEX);
    bsp_spin_init(&ts->lock);
    status_op_script(STATUS_OP_SCRIPT_STACK_ADD, 0);
    trace_msg(TRACE_LEVEL_NOTICE, "Script : Generate a new LUA thread registed at index %d", ref);

    return ref;
}

// Delete a thread stack
// LUA_THREAD will be removed by GC automatically, so we needn't free it by hand
int script_remove_stack(BSP_SCRIPT_STACK *ts)
{
    if (!ts || !ts->state || !ts->stack)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    luaL_unref(ts->state, LUA_REGISTRYINDEX, ts->stack_ref);
    status_op_script(STATUS_OP_SCRIPT_STACK_DEL, 0);
    trace_msg(TRACE_LEVEL_NOTICE, "Script : unregisted a LUA thread from index %d", ts->stack_ref);
    ts->stack = NULL;
    ts->stack_ref = 0;

    return BSP_RTN_SUCCESS;
}

// Load module (C library) into script
int script_load_module(BSP_STRING *module, int enable_main_thread)
{
    lt_dlhandle dl;
    int (* loader)(lua_State *) = NULL;
    int i;
    BSP_THREAD *t;

    if (!module)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    char *module_name = bsp_strndup(STR_STR(module), STR_LEN(module));
    if (!module_name)
    {
        return BSP_RTN_ERROR_MEMORY;
    }
    BSP_CORE_SETTING *settings = get_core_setting();
    char symbol[_SYMBOL_NAME_MAX];
    char file_name[_POSIX_PATH_MAX];

    lt_dlinit();
    snprintf(file_name, _POSIX_PATH_MAX - 1, "%s/module-%s.so", settings->mod_dir, module_name);
    snprintf(symbol, _SYMBOL_NAME_MAX - 1, "bsp_module_%s", module_name);
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Try to load module %s from %s", symbol, file_name);
    dl = lt_dlopen(file_name);
    bsp_free(module_name);

    // Try load
    if (dl)
    {
        loader = (int (*)(lua_State *)) lt_dlsym(dl, (const char *) symbol);
        if (loader)
        {
            // Main thread
            if (enable_main_thread)
            {
                t = get_thread(MAIN_THREAD);
                if (t)
                {
                    loader(t->script_runner.state);
                    trace_msg(TRACE_LEVEL_VERBOSE, "Script : Symbol %s loaded into main thread", symbol);
                }
            }

            for (i = 0; i < settings->static_workers; i ++)
            {
                t = get_thread(i);
                if (t)
                {
                    loader(t->script_runner.state);
                    trace_msg(TRACE_LEVEL_VERBOSE, "Script : Symbol %s loaded into thread %d", symbol, i);
                }
            }
            trace_msg(TRACE_LEVEL_NOTICE, "Script : Symbol %s loaded", symbol);
        }
        else
        {
            trace_msg(TRACE_LEVEL_ERROR, "Script : Symbol %s load error", symbol);
            return BSP_RTN_ERROR_RESOURCE;
        }
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Cannot open module file %s", file_name);
        return BSP_RTN_ERROR_IO;
    }

    return BSP_RTN_SUCCESS;
}
