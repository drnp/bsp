/*
 * debugger.c
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
 * Data tracer for debug
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/14/2012
 * @changelog 
 *      [06/14/2012] - Creation
 *      [12/17/2013] - BSP_VAL_POINTER added
 *      [06/04/2014] - debug_lua_stack() added
 *      [08/11/2014] - debug_object rewrite
 */

#include "bsp.h"

// To prevent display transplacement
BSP_SPINLOCK debug_lock = BSP_SPINLOCK_INITIALIZER;

// Print some formatted message with terminal color
void trace_output(time_t now, int level, const char *msg)
{
    struct tm *loctime;
    char tgdate[64];

    if (!level || !msg)
    {
        return;
    }

    loctime = localtime(&now);
    strftime(tgdate, 64, "%m/%d/%Y %H:%M:%S", loctime);
    bsp_spin_lock(&debug_lock);
    fprintf(stderr, "\033[1;37m[\033[0m"
                    "\033[0;36m%s\033[0m"
                    "\033[1;37m]\033[0m"
                    " - "
                    "\033[1;37m[\033[0m"
                    "%s"
                    "\033[1;37m]\033[0m"
                    " : %s\n", tgdate, get_trace_level_str(level, 1), msg);
    bsp_spin_unlock(&debug_lock);

    return;
}

// Trigger some message and exit(terminate) process
void trigger_exit(int level, const char *fmt, ...)
{
    time_t now = time((time_t *) NULL);
    char msg[MAX_TRACE_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    size_t nbytes = vsnprintf(msg, MAX_TRACE_LENGTH - 1, fmt, ap);
    if (nbytes >= 0)
    {
        msg[nbytes] = 0;
    }
    va_end(ap);

    fprintf(stderr, "Process exit : %d - code[%d] - %s\n", (int) now, level, msg);
    _exit(level);

    return;
}

// Output some message
void debug_str(const char *fmt, ...)
{
    time_t now = time((time_t *) NULL);
    struct tm *loctime;
    char tgdate[64];
    char msg[MAX_TRACE_LENGTH];

    va_list ap;
    va_start(ap, fmt);
    size_t nbytes = vsnprintf(msg, MAX_TRACE_LENGTH - 1, fmt, ap);
    if (nbytes >= 0)
    {
        msg[nbytes] = 0;
    }
    va_end(ap);

    loctime = localtime(&now);
    strftime(tgdate, 64, "%m/%d/%Y %H:%M:%S", loctime);
    bsp_spin_lock(&debug_lock);
    fprintf(stderr, "\n\033[1;37m=== [Debug String] === <%s START> ===\033[0m\n"
                    "\033[1;33m %s\033[0m\n"
                    "\033[1;37m=== [Debug String] === <%s END  > ===\033[0m\n\n", tgdate, msg, tgdate);
    bsp_spin_unlock(&debug_lock);

    return;
}

// Print one memory block as also human-readable and a hexadecimal table
void debug_hex(const char *data, ssize_t len)
{
    int i;
    if (len < 0)
    {
        len = strlen(data);
    }

    time_t now = time((time_t *) NULL);
    struct tm *loctime;
    char tgdate[64];
    loctime = localtime(&now);
    strftime(tgdate, 64, "%m/%d/%Y %H:%M:%S", loctime);
    bsp_spin_lock(&debug_lock);
    fprintf(stderr, "\n\033[1;37m=== [Debug Hex %d bytes] === <%s START> ===\033[0m\n", (int) len, tgdate);
    for (i = 0; i < len; i ++)
    {
        fprintf(stderr, "\033[1;33m%02X\033[0m ", (unsigned char) data[i]);
        if (i % 32 == 31)
        {
            fprintf(stderr, "\n");
        }
        
        else if (i % 8 == 7)
        {
            fprintf(stderr, "  ");
        }
    }

    fprintf(stderr, "\n\033[1;37m=== [Debug Hex %d bytes] === <%s DATA > ===\033[0m\n", (int) len, tgdate);
    for (i = 0; i < len; i ++)
    {
        if (data[i] >= 32 && data[i] <= 127)
        {
            fprintf(stderr, "\033[1;35m %c \033[0m", data[i]);
        }
        else
        {
            fprintf(stderr, "\033[0;34m . \033[0m");
        }

        if (i % 32 == 31)
        {
            fprintf(stderr, "\n");
        }
        else if (i % 8 == 7)
        {
            fprintf(stderr, "  ");
        }
    }

    fprintf(stderr, "\n\033[1;37m=== [Debug Hex %d bytes] === <%s END  > ===\033[0m\n\n", (int) len, tgdate);
    bsp_spin_unlock(&debug_lock);

    return;
}

// Pretty print a BSP-Object with indent
static void _dump_value(BSP_VALUE *val, int layer);
static void _dump_object(BSP_OBJECT *obj, int layer);

static void _dump_value(BSP_VALUE *val, int layer)
{
    if (!val)
    {
        fprintf(stderr, "\033[1;35m### NO VALUE ###\033[0m\n");
        return;
    }

    switch (val->type)
    {
        case BSP_VAL_NULL : 
            fprintf(stderr, "\033[1;31m(NULL)\033[0m\n");
            break;
        case BSP_VAL_INT : 
            fprintf(stderr, "\033[1;33m(INTEGER)\033[0m => %lld\n", (long long int) get_vint(val->lval, NULL));
            break;
        case BSP_VAL_INT29 : 
            fprintf(stderr, "\033[1;33m(INTEGER29)\033[0m => %d\n", (int) get_vint29(val->lval, NULL));
            break;
        case BSP_VAL_FLOAT : 
            fprintf(stderr, "\033[1;34m(FLOAT)\033[0m => %f\n", get_float(val->lval));
            break;
        case BSP_VAL_DOUBLE : 
            fprintf(stderr, "\033[1;34m(DOUBLE)\033[0m => %g\n", get_double(val->lval));
            break;
        case BSP_VAL_BOOLEAN_TRUE : 
            fprintf(stderr, "\033[1;35m(BOOLEAN_TRUE)\033[0m\n");
            break;
        case BSP_VAL_BOOLEAN_FALSE : 
            fprintf(stderr, "\033[1;35m(BOOLEAN_FALSE)\033[0m\n");
            break;
        case BSP_VAL_STRING : 
            fprintf(stderr, "\033[1;32m(STRING)\033[0m => ");
            BSP_STRING *str = (BSP_STRING *) val->rval;
            if (str && STR_STR(str))
            {
                write(STDERR_FILENO, STR_STR(str), STR_LEN(str));
            }
            else
            {
                fprintf(stderr, "\033[1,31m### NULL_STRING ###\033[0m");
            }
            fprintf(stderr, "\n");
            break;
        case BSP_VAL_POINTER : 
            fprintf(stderr, "\033[1;36m(POINTER)\033[0m => %p\n", (void *) val->rval);
            break;
        case BSP_VAL_OBJECT : 
            fprintf(stderr, "\033[1;36m(OBJECT)\033[0m => ");
            BSP_OBJECT *sub_obj = (BSP_OBJECT *) val->rval;
            _dump_object(sub_obj, layer + 1);
            break;
        case BSP_VAL_UNKNOWN : 
            fprintf(stderr, "\033[1;31m(UNKNOWN)\033[0m\n");
            break;
        default : 
            break;
    }

    return;
}

static void _dump_object(BSP_OBJECT *obj, int layer)
{
    if (!obj)
    {
        return;
    }

    int i;
    BSP_VALUE *val;
    reset_object(obj);
    switch (obj->type)
    {
        case OBJECT_TYPE_SINGLE : 
            // Single
            fprintf(stderr, "\033[1;37mObject type : [SINGLE]\033[0m\n");
            val = object_get_single(obj);
            _dump_value(val, layer);
            break;
        case OBJECT_TYPE_ARRAY : 
            // Array
            fprintf(stderr, "\033[1;37mObject type : [ARRAY]\033[0m\n");
            size_t idx = 0;
            for (idx = 0; idx < object_size(obj); idx ++)
            {
                for (i = 0; i <= layer; i ++)
                {
                    fprintf(stderr, "\t");
                }
                fprintf(stderr, "\033[1;35m%lld\033[0m\t=> ", (long long int) idx);
                val = object_get_array(obj, idx);
                _dump_value(val, layer);
            }
            fprintf(stderr, "\n");
            break;
        case OBJECT_TYPE_HASH : 
            // Dict
            fprintf(stderr, "\033[1;37mObject type : [HASH]\033[0m\n");
            val = curr_item(obj);
            BSP_STRING *key;
            while (val)
            {
                key = curr_hash_key(obj);
                for (i = 0; i <= layer; i ++)
                {
                    fprintf(stderr, "\t");
                }
                if (key)
                {
                    fprintf(stderr, "\033[1;33m");
                    write(STDERR_FILENO, STR_STR(key), STR_LEN(key));
                    fprintf(stderr, "\033[0m");
                }
                else
                {
                    fprintf(stderr, "### NO KEY ###");
                }
                fprintf(stderr, "\t=> ");
                _dump_value(val, layer);
                next_item(obj);
                val = curr_item(obj);
            }
            fprintf(stderr, "\n");
            break;
        case OBJECT_TYPE_UNDETERMINED : 
        default : 
            // Null
            break;
    }

    return;
}

void debug_object(BSP_OBJECT *obj)
{
    if (!obj)
    {
        bsp_spin_lock(&debug_lock);
        fprintf(stderr, "\n\033[1;37m === [NOTHING TO DEBUG] ===\033[0m\n\n");
        bsp_spin_unlock(&debug_lock);

        return;
    }

    bsp_spin_lock(&debug_lock);
    fprintf(stderr, "\n\033[1;37m=== [Debug Object] === < START > ===\033[0m\n");
    _dump_object(obj, 0);
    fprintf(stderr, "\033[1;37m=== [Debug Object] === < END > ===\033[0m\n\n");
    bsp_spin_unlock(&debug_lock);

    return;
}

void debug_value(BSP_VALUE *val)
{
    if (!val)
    {
        bsp_spin_lock(&debug_lock);
        fprintf(stderr, "\n\033[1;37m === [NOTHING TO DEBUG] ===\033[0m\n\n");
        bsp_spin_unlock(&debug_lock);

        return;
    }

    bsp_spin_lock(&debug_lock);
    fprintf(stderr, "\n\033[1;37m=== [Debug Value] === < START > ===\033[0m\n");
    _dump_value(val, 0);
    fprintf(stderr, "\033[1;37m=== [Debug Value] === < END > ===\033[0m\n\n");
    bsp_spin_unlock(&debug_lock);

    return;
}

// Print lua stack
void debug_lua_stack(lua_State *l)
{
    if (!l)
    {
        return;
    }

    bsp_spin_lock(&debug_lock);
    fprintf(stderr, "\n\033[1;33m== [Stack top] ==\033[0m\n");
    int size = lua_gettop(l), n, type;
    for (n = 1; n <= size; n ++)
    {
        type = lua_type(l, n);
        switch (type)
        {
            case LUA_TNIL : 
                fprintf(stderr, "\t\033[1;37m[NIL]\033[0m\n");
                break;
            case LUA_TNUMBER : 
                fprintf(stderr, "\t\033[1;37m[NUMBER]    => %f\033[0m\n", lua_tonumber(l, n));
                break;
            case LUA_TBOOLEAN : 
                fprintf(stderr, "\t\033[1;37m[BOOLEAN]   => %d\033[0m\n", lua_toboolean(l, n));
                break;
            case LUA_TSTRING : 
                fprintf(stderr, "\t\033[1;37m[STRING]    => %s\033[0m\n", lua_tostring(l, n));
                break;
            case LUA_TTABLE : 
                fprintf(stderr, "\t\033[1;37m[TABLE]\033[0m\n");
                break;
            case LUA_TFUNCTION : 
                fprintf(stderr, "\t\033[1;37m[FUNCTION]\033[0m\n");
                break;
            case LUA_TUSERDATA : 
                fprintf(stderr, "\t\033[1;37m[USERDATA]  => %p\033[0m\n", lua_touserdata(l, n));
                break;
            case LUA_TTHREAD : 
                fprintf(stderr, "\t\033[1;37m[THREAD]\033[0m\n");
                break;
            case LUA_TLIGHTUSERDATA : 
                fprintf(stderr, "\t\033[1;37m[LUSERDATA] => %p\033[0m\n", lua_touserdata(l, n));
                break;
            default : 
                break;
        }
    }
    fprintf(stderr, "\033[1;33m== [Stack button] ==\033[0m\n\n");
    bsp_spin_unlock(&debug_lock);

    return;
}
