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
/*
// Default code reader -- Get string from file with identifier as filename
static void _default_code_reader(const char *identifier, BSP_STRING *code)
{
    if (!identifier || !code)
    {
        return;
    }

    char buff[16384];
    size_t nbytes;

    trace_msg(TRACE_LEVEL_DEBUG, "Script : Try to read script file <%s>", identifier);
    FILE *fp = fopen(identifier, "r");
    if (!fp)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Cannot open script file <%s>", identifier);
        return;
    }

    while (!feof(fp) && !ferror(fp))
    {
        nbytes = fread(buff, 1, 16384, fp);
        if (nbytes > 0)
        {
            string_append(code, (const char *) buff, nbytes);
        }

        else
        {
            break;
        }
    }

    return;
}
*/
/*
// Create a new script handler
int script_init()
{
    //BSP_CORE_SETTING *settings = get_core_setting();
    script = bsp_calloc(1, sizeof(struct bsp_script_t));
    if (!script)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "Script : Script alloc error");
    }
    script->allocator = _default_allocator;
    script->code = new_string(NULL, 0);
    
    BSP_THREAD *thread = NULL;
    int i;

    script = mempool_calloc(sizeof(struct bsp_script_t), 1);
    if (!script)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Script alloc error");
        return BSP_RTN_ERROR_MEMORY;
    }

    script->sub_states = mempool_calloc(sizeof(lua_State *) * settings->static_workers, 1);
    if (!script->sub_states)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : LUA state list initialize failed");
        mempool_free(script);
        script = NULL;
        return BSP_RTN_ERROR_MEMORY;
    }
    
    code = new_string(NULL, 0);
    if (!code)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Script code buffer alloc error");
        mempool_free(script->sub_states);
        mempool_free(script);
        script = NULL;
        return BSP_RTN_ERROR_MEMORY;
    }
    
    trace_msg(TRACE_LEVEL_VERBOSE, "Script : Try to create script");
    script->main_state = lua_newstate(_allocator, NULL);
    if (!script->main_state)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Script state initialize failed");
        mempool_free(script->sub_states);
        mempool_free(script);
        script = NULL;
        return BSP_RTN_ERROR_SCRIPT;
    }
    
    // Main thread
    thread = get_thread(0);
    thread->runner = script->main_state;
    status_op_script(STATUS_OP_SCRIPT_STATE_ADD, 0);
    for (i = 1; i <= settings->static_workers; i ++)
    {
        // Add link
        thread = get_thread(i);
        if (thread)
        {
            thread->runner = lua_newstate(_allocator, NULL);
            if (!thread->runner)
            {
                trace_msg(TRACE_LEVEL_ERROR, "Script : Script sub-state initialize failed");
                thread->runner = NULL;
                mempool_free(script->sub_states);
                mempool_free(script);
                script = NULL;
                return BSP_RTN_ERROR_SCRIPT;
            }
            script->sub_states[i - 1] = thread->runner;
            status_op_script(STATUS_OP_SCRIPT_STATE_ADD, 0);
            luaL_openlibs(thread->runner);
            lua_settop(thread->runner, 0);
        }
    }

    script->nsub_states = settings->static_workers;
    script->load_times = 0;
    script->identifier = NULL;
    luaL_openlibs(script->main_state);
    lua_settop(script->main_state, 0);
    script->code_reader = reader ? reader : _default_code_reader;
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Script initialized");
    
    return BSP_RTN_SUCCESS;
}
*/
// Load script code block
int script_load_string(lua_State *l, const char *code, ssize_t len)
{
    if (!l || !code)
    {
        return BSP_RTN_ERROR_SCRIPT;
    }
    
    if (len < 0)
    {
        len = strlen(code);
    }

    int ret = luaL_loadbufferx(l, code, len, NULL, NULL);
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

/*
// Set script identifier
void script_set_identifier(const char *identifier)
{
    if (script && identifier)
    {
        if (script->identifier)
        {
            bsp_free(script->identifier);
        }

        script->identifier = bsp_strdup(identifier);
    }

    return;
}

// Set hooks
void script_set_hook(int hook, const char *func)
{
    if (script && func)
    {
        switch (hook)
        {
            case SCRIPT_HOOK_LOAD : 
                script->func_on_load = (char *) func;
                break;
            case SCRIPT_HOOK_RELOAD : 
                script->func_on_reload = (char *) func;
                break;
            case SCRIPT_HOOK_EXIT : 
                script->func_on_exit = (char *) func;
                break;
            case SCRIPT_HOOK_SUB_LOAD : 
                script->func_on_sub_load = (char *) func;
                break;
            case SCRIPT_HOOK_SUB_RELOAD : 
                script->func_on_sub_reload = (char *) func;
                break;
            case SCRIPT_HOOK_SUB_EXIT : 
                script->func_on_sub_exit = (char *) func;
                break;
            default : 
                break;
        }
    }

    return;
}

// Load and run LUA script
int script_try_load()
{
    if (!script || !script->identifier || !script->main_state || !script->sub_states)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Cannot load script, check script status and identifier first");
        return BSP_RTN_ERROR_GENERAL;
    }
    int i;
    
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Try load script : %s", script->identifier);
    clean_string(code);
    if (script->code_reader)
    {
        script->code_reader(script->identifier, code);
    }
    if (code->str && code->ori_len)
    {
        if (0 != luaL_dostring(script->main_state, code->str))
        {
            trace_msg(TRACE_LEVEL_ERROR, "Script %s load error : %s", script->identifier, lua_tostring(script->main_state, -1));
            return BSP_RTN_ERROR_SCRIPT;
        }
        status_op_script(STATUS_OP_SCRIPT_COMPILE, 0);

        // Sub states
        for (i = 0; i < script->nsub_states; i ++)
        {
            if (script->sub_states[i])
            {
                if (0 != luaL_dostring(script->sub_states[i], code->str))
                {
                    trace_msg(TRACE_LEVEL_ERROR, "Script %s load error on sub state %d : %s", script->identifier, i, lua_tostring(script->sub_states[i], -1));
                }
                status_op_script(STATUS_OP_SCRIPT_COMPILE, 0);
            }
        }
    }
    clean_string(code);

    // On_load callback
    if (0 == script->load_times)
    {
        if (script->func_on_load)
        {
            script_caller_call_func(script->main_state, script->func_on_load, NULL);
        }
    }
    else
    {
        if (script->func_on_reload)
        {
            script_caller_call_func(script->main_state, script->func_on_reload, NULL);
        }
    }
    script->load_times ++;

    return BSP_RTN_SUCCESS;
}

// Unload / close LUA script
int script_close()
{
    if (!script || !script->main_state || !script->sub_states)
    {
        return BSP_RTN_ERROR_SCRIPT;
    }
    int i;

    if (script->func_on_exit)
    {
        script_caller_call_func(script->main_state, script->func_on_exit, NULL);
    }

    for (i = 0; i < script->nsub_states; i ++)
    {
        if (script->sub_states[i])
        {
            lua_close(script->sub_states[i]);
            status_op_script(STATUS_OP_SCRIPT_STATE_DEL, 0);
        }
    }
    
    lua_close(script->main_state);
    status_op_script(STATUS_OP_SCRIPT_STATE_DEL, 0);
    trace_msg(TRACE_LEVEL_DEBUG, "Script closed");

    return BSP_RTN_SUCCESS;
}

static lua_State * _get_caller()
{
    BSP_THREAD * thread = find_thread();
    if (!thread)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Script : Invalid thread");
        return NULL;
    }
    
    return thread->runner;
}
*/
int script_call(lua_State *caller, const char *func, BSP_SCRIPT_CALL_PARAM p[])
{
    int ret, status;
    size_t len;
    int nargs = 0;
    BSP_SCRIPT_CALL_PARAM *curr;
    int32_t vi;
    int64_t vl;
    float vf;
    double vd;
    BSP_STRING *str;

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
        debug_lua_stack(caller);
        trace_msg(TRACE_LEVEL_ERROR, "Script : No function specified");
    }
    else
    {
        while (p)
        {
            curr = &p[nargs];
            if (!curr || CALL_PTYPE_END == curr->type || !curr->ptr)
            {
                break;
            }
            
            if (!lua_checkstack(caller, 1))
            {
                // Stack full!
                trace_msg(TRACE_LEVEL_ERROR, "Script : Lua stack full when call");
                lua_settop(caller, 0);
                return BSP_RTN_ERROR_SCRIPT;
            }
            
            switch (curr->type)
            {
                case CALL_PTYPE_BOOLEAN : 
                    // Boolean
                    memcpy(&vi, curr->ptr, sizeof(int32_t));
                    lua_pushboolean(caller, (int) vi);
                    break;
                case CALL_PTYPE_INTEGER : 
                    // Integer
                    memcpy(&vi, curr->ptr, sizeof(int32_t));
                    lua_pushinteger(caller, (lua_Integer) vi);
                    break;
                case CALL_PTYPE_INTEGER64 : 
                    // Integer
                    memcpy(&vl, curr->ptr, sizeof(int64_t));
                    lua_pushinteger(caller, (lua_Integer) vl);
                    break;
                case CALL_PTYPE_FLOAT : 
                    // Float
                    memcpy(&vf, curr->ptr, sizeof(float));
                    lua_pushnumber(caller, (lua_Number) vf);
                    break;
                case CALL_PTYPE_DOUBLE : 
                    // Integer
                    memcpy(&vd, curr->ptr, sizeof(double));
                    lua_pushnumber(caller, (lua_Number) vd);
                    break;
                case CALL_PTYPE_OSTRING : 
                case CALL_PTYPE_OSTRING_F : 
                    // String / Binary
                    len = (curr->len < 0) ? strlen((const char *) curr->ptr) : curr->len;
                    lua_pushlstring(caller, (const char *) curr->ptr, len);
                    break;
                case CALL_PTYPE_STRING : 
                case CALL_PTYPE_STRING_F : 
                    // BSP.String
                    str = (BSP_STRING *) curr->ptr;
                    lua_pushlstring(caller, STR_STR(str), STR_LEN(str));
                    break;
                case CALL_PTYPE_OBJECT : 
                case CALL_PTYPE_OBJECT_F : 
                    // BSP Object
                    object_to_lua((BSP_OBJECT *) curr->ptr, caller);
                    break;
                default : 
                    // Unknown
                    break;
            }
            nargs ++;
        }
        status = lua_status(caller);
        if (LUA_OK == status)
        {
            // Pcall
            ret = lua_pcall(caller, nargs, 0, 0);
        }
        else if (LUA_YIELD == status)
        {
            // Resume
            ret = lua_resume(caller, NULL, nargs);
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
        status_op_script(STATUS_OP_SCRIPT_CALL, 0);
    }
    lua_settop(caller, 0);
    
    // Recycle
    while (p)
    {
        curr = &p[nargs];
        if (!curr || CALL_PTYPE_END == curr->type || !curr->ptr)
        {
            break;
        }

        switch (curr->type)
        {
            case CALL_PTYPE_OSTRING_F : 
                // Try to free
                bsp_free(curr->ptr);
                break;
            case CALL_PTYPE_STRING_F : 
                del_string(curr->ptr);
                break;
            case CALL_PTYPE_OBJECT_F : 
                del_object(curr->ptr);
                break;
            default : 
                break;
        }
        nargs ++;
    }
    
    return BSP_RTN_SUCCESS;
}
/*
// Call LUA function with specified state
void script_caller_call_func(lua_State *caller, const char *func, BSP_SCRIPT_CALL_PARAM p[])
{
    if (!caller || !func)
    {
        return;
    }

    trace_msg(TRACE_LEVEL_VERBOSE, "Script : Try call LUA function %s in state", func);
    lua_getglobal(caller, func);
    _real_call(caller, p);
    
    return;
}

// Call LUA function registered in register with specified state
void script_caller_call_idx(lua_State *caller, int idx, BSP_SCRIPT_CALL_PARAM p[])
{
    if (!caller)
    {
        return;
    }

    trace_msg(TRACE_LEVEL_VERBOSE, "Script : Try call LUA function registered with index %d in state", idx);
    lua_rawgeti(caller, LUA_REGISTRYINDEX, idx);
    _real_call(caller, p);
    
    return;
}

// Call LUA function
void script_call_func(const char *func, BSP_SCRIPT_CALL_PARAM p[])
{
    lua_State *caller = _get_caller();
    script_caller_call_func(caller, func, p);
    
    return;
}

// Call LUA function registered in register
void script_call_idx(int idx, BSP_SCRIPT_CALL_PARAM p[])
{
    lua_State *caller = _get_caller();
    script_caller_call_idx(caller, idx, p);
    
    return;
}
*/
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
        return BSP_RTN_ERROR_SCRIPT;
    }
    luaL_openlibs(state);
    ss->state = state;
    ss->load_times = 0;
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Create a new script state");

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
        return -1;
    }
    
    ts->stack = stack;
    int ref = ts->stack_ref = luaL_ref(ts->state, LUA_REGISTRYINDEX);
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
    trace_msg(TRACE_LEVEL_NOTICE, "Script : unregisted a LUA thread from index %d", ts->stack_ref);
    ts->stack = NULL;
    ts->stack_ref = 0;

    return BSP_RTN_SUCCESS;
}

// Load module (C library) into script
int script_load_module(const char *module_name)
{
    lt_dlhandle dl;
    int (* loader)(lua_State *) = NULL;
    int i;
    BSP_THREAD *t;

    if (!module_name)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    char symbol[_SYMBOL_NAME_MAX];
    char filename[_POSIX_PATH_MAX];
    
    lt_dlinit();
    snprintf(filename, _POSIX_PATH_MAX - 1, "%s/module-%s.so", settings->mod_dir, module_name);
    snprintf(symbol, _SYMBOL_NAME_MAX - 1, "bsp_module_%s", module_name);
    trace_msg(TRACE_LEVEL_DEBUG, "Script : Try to load module %s from %s", symbol, filename);
    dl = lt_dlopen(filename);

    // Try load
    if (dl)
    {
        loader = (int (*)(lua_State *)) lt_dlsym(dl, (const char *) symbol);
        if (loader)
        {
            // Main thread)
            t = get_thread(MAIN_THREAD);
            if (t)
            {
                loader(t->script_runner.state);
                trace_msg(TRACE_LEVEL_VERBOSE, "Script : Symbol %s loaded into main thread", symbol);
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
        trace_msg(TRACE_LEVEL_ERROR, "Script : Cannot open module file %s", filename);
        return BSP_RTN_ERROR_IO;
    }
    
    return BSP_RTN_SUCCESS;
}
