/*
 * script.h
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
 * LUA script runner header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/09/2012
 * @changelog 
 *      [07/02/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_SCRIPT_H

#define _LIB_BSP_CORE_SCRIPT_H
/* Headers */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

/* Definations */
#define CALL_PTYPE_END                          0x0
#define CALL_PTYPE_BOOLEAN                      0x1
#define CALL_PTYPE_INTEGER                      0x21
#define CALL_PTYPE_INTEGER64                    0x22
#define CALL_PTYPE_FLOAT                        0x23
#define CALL_PTYPE_DOUBLE                       0x24
#define CALL_PTYPE_OSTRING                      0x31
#define CALL_PTYPE_OSTRING_F                    0x32
#define CALL_PTYPE_STRING                       0x41
#define CALL_PTYPE_STRING_F                     0x42
#define CALL_PTYPE_OBJECT                       0x51
#define CALL_PTYPE_OBJECT_F                     0x52

#define SCRIPT_HOOK_LOAD                        0x1
#define SCRIPT_HOOK_RELOAD                      0x2
#define SCRIPT_HOOK_EXIT                        0x3
#define SCRIPT_HOOK_SUB_LOAD                    0x11
#define SCRIPT_HOOK_SUB_RELOAD                  0x12
#define SCRIPT_HOOK_SUB_EXIT                    0x13

#define DEFAULT_SCRIPT_GC_INTERVAL              300

/* Macros */

/* Structs */
typedef struct bsp_script_call_param_t
{
    char                type;
    ssize_t             len;
    void                *ptr;
} BSP_SCRIPT_CALL_PARAM;

typedef struct bsp_script_stack_t
{
    lua_State           *state;
    lua_State           *stack;
    int                 stack_ref;
    BSP_SPINLOCK        lock;
} BSP_SCRIPT_STACK;

typedef struct bsp_script_symbol_t
{
    const char          *func;
    const char          *regkey;
    int                 regref;
} BSP_SCRIPT_SYMBOL;

/* Functions */
// Load script code block
int script_load_string(BSP_SCRIPT_STACK *ts, BSP_STRING *code, BSP_SCRIPT_SYMBOL *sym);

// Load script code from file
int script_load_file(BSP_SCRIPT_STACK *ts, const char *filename, BSP_SCRIPT_SYMBOL *sym);

// Call a script function with given parameters
int script_call(BSP_SCRIPT_STACK *caller, BSP_SCRIPT_SYMBOL *sym, BSP_OBJECT *p);

// New state(runner), create a new LUA state
lua_State * script_new_state();

// New stack(caller), create a new LUA thread and bind to state
int script_new_stack(BSP_SCRIPT_STACK *ts);

// Delete a thread stack
int script_remove_stack(BSP_SCRIPT_STACK *ts);

// Load LUA modules to script
int script_load_module(BSP_STRING *module, int enable_main_thread);

// Dump function to all worker with given name
int script_func_to_worker(lua_State *s, BSP_SCRIPT_SYMBOL *sym);

#endif  /* _LIB_BSP_CORE_SCRIPT_H */
