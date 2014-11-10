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
int script_load_string(BSP_SCRIPT_STACK *ts, BSP_STRING *code, BSP_SCRIPT_SYMBOL *sym)
{
    if (!ts || !ts->state || !code)
    {
        return BSP_RTN_ERROR_SCRIPT;
    }

    int ret = BSP_RTN_ERROR_GENERAL;
    int r;
    bsp_spin_lock(&ts->lock);
    r = luaL_loadbufferx(ts->state, STR_STR(code), STR_LEN(code), NULL, NULL);
    switch (r)
    {
        case LUA_OK : 
            if (lua_isfunction(ts->state, -1) && sym)
            {
                if (sym->func)
                {
                    // Set global
                    lua_setglobal(ts->state, sym->func);
                }
                else if (sym->regkey)
                {
                    // Set register
                    lua_setfield(ts->state, LUA_REGISTRYINDEX, sym->regkey);
                }
                else if (sym->regref < 0)
                {
                    // Set reference
                    sym->regref = luaL_ref(ts->state, LUA_REGISTRYINDEX);
                }
            }
            trace_msg(TRACE_LEVEL_VERBOSE, "Script : Code chunk loaded into state");
            ret = BSP_RTN_SUCCESS;
            break;
        case LUA_ERRSYNTAX : 
            trace_msg(TRACE_LEVEL_ERROR, "Script : Lua script syntax error : %s", lua_tostring(ts->state, -1));
            ret = BSP_RTN_ERROR_SCRIPT;
            break;
        case LUA_ERRMEM : 
            trace_msg(TRACE_LEVEL_ERROR, "Script : Memory error to load Lua script");
            ret = BSP_RTN_ERROR_MEMORY;
            break;
        case LUA_ERRGCMM : 
        default : 
            bsp_spin_unlock(&ts->lock);
            trigger_exit(BSP_RTN_FATAL, "Script error");
            break;
    }
    bsp_spin_unlock(&ts->lock);

    return ret;
}

// Load script code from file
int script_load_file(BSP_SCRIPT_STACK *ts, const char *filename, BSP_SCRIPT_SYMBOL *sym)
{
    if (!ts || !ts->state || !filename)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_STRING *code = new_string_from_file(filename);
    int ret = script_load_string(ts, code, sym);
    del_string(code);

    return ret;
}

// Call a script function or resume coruntine
static int _cont(lua_State *l)
{
    //lua_settop(l, 0);

    return 0;
}

int script_call(BSP_SCRIPT_STACK *caller, BSP_SCRIPT_SYMBOL *sym, BSP_OBJECT *p)
{
    int ret, status, nargs = 0;

    if (!caller || !caller->state)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Not available state");
        return BSP_RTN_ERROR_SCRIPT;
    }
    lua_State *l = (caller->stack) ? (caller->stack) : (caller->state);

    //bsp_spin_lock(&caller->lock);
    if (sym)
    {
        // Try get function
        lua_checkstack(l, 1);
        if (sym->func)
        {
            // From global table
            lua_getglobal(l, sym->func);
        }
        else if (sym->regkey)
        {
            // From register
            lua_getfield(l, LUA_REGISTRYINDEX, sym->regkey);
        }
        else if (sym->regref > 0)
        {
            // From reference(register table)
            lua_rawgeti(l, LUA_REGISTRYINDEX, sym->regref);
        }
    }

    if (!lua_isfunction(l, -1))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : No function specified");
    }
    else
    {
        if (p)
        {
            object_to_lua_stack(l, p);
            nargs = 1;
        }
        status = lua_status(l);
        if (LUA_OK == status)
        {
            // Pcall
            trace_msg(TRACE_LEVEL_VERBOSE, "Script : Call a stack");
            ret = lua_pcallk(l, nargs, 0, 0, 0, _cont);
        }
        else if (LUA_YIELD == status)
        {
            // Resume
            trace_msg(TRACE_LEVEL_VERBOSE, "Script : Resume a yielded stack");
            ret = lua_resume(l, NULL, nargs);
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
            trace_msg(TRACE_LEVEL_ERROR, "Script : Call lua function error : %s", lua_tostring(l, -1));
        }
        else
        {
            trace_msg(TRACE_LEVEL_DEBUG, "Script : Stack call successfully");
        }
        status_op_script(STATUS_OP_SCRIPT_CALL, 0);
    }
    _cont(l);
    //bsp_spin_unlock(&caller->lock);

    return BSP_RTN_SUCCESS;
}

// New state(runner), create a new LUA state
lua_State * script_new_state()
{
    lua_State *state = lua_newstate(_default_allocator, NULL);
    if (!state)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Create new script state error");
        return NULL;
    }
    luaL_openlibs(state);
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Create a new script state");
    status_op_script(STATUS_OP_SCRIPT_STATE_ADD, 0);

    return state;
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
                if (t && t->script_runner.state)
                {
                    bsp_spin_lock(&t->script_runner.lock);
                    loader(t->script_runner.state);
                    bsp_spin_unlock(&t->script_runner.lock);
                    trace_msg(TRACE_LEVEL_VERBOSE, "Script : Symbol %s loaded into main thread", symbol);
                }
            }

            for (i = 0; i < settings->static_workers; i ++)
            {
                t = get_thread(i);
                if (t && t->script_runner.state)
                {
                    bsp_spin_lock(&t->script_runner.lock);
                    loader(t->script_runner.state);
                    bsp_spin_unlock(&t->script_runner.lock);
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

// Dump (copy) a LUA function (chunk) to all worker
static int _dump_chunk_to_str(lua_State *s, const void *p, size_t len, void *ud)
{
    if (!p || !len || !ud)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_STRING *chunk = (BSP_STRING *) ud;
    string_append(chunk, (const char *) p, len);

    return BSP_RTN_SUCCESS;
}

int script_func_to_worker(lua_State *s, BSP_SCRIPT_SYMBOL *sym)
{
    if (!s)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    if (!lua_isfunction(s, -1))
    {
        return BSP_RTN_ERROR_SCRIPT;
    }

    BSP_STRING *chunk = new_string(NULL, 0);
    lua_dump(s, _dump_chunk_to_str, (void *) chunk);
    BSP_CORE_SETTING *settings = get_core_setting();
    BSP_THREAD *t = NULL;
    int i;

    for (i = 0; i < settings->static_workers; i ++)
    {
        t = get_thread(i);
        if (t && t->script_runner.state)
        {
            script_load_string(&t->script_runner, chunk, sym);
        }
    }
    del_string(chunk);

    return BSP_RTN_SUCCESS;
}
