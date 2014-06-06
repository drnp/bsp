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
 */

#include "bsp.h"

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
    
    fprintf(stderr, "\033[1;37m[\033[0m"
                    "\033[0;36m%s\033[0m"
                    "\033[1;37m]\033[0m"
                    " - "
                    "\033[1;37m[\033[0m"
                    "%s"
                    "\033[1;37m]\033[0m"
                    " : %s\n", tgdate, get_trace_level_str(level, 1), msg);
    
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
    fprintf(stderr, "\n\033[1;37m=== [Debug String] === <%s START> ===\033[0m\n"
                    "\033[1;33m %s\033[0m\n"
                    "\033[1;37m=== [Debug String] === <%s END  > ===\033[0m\n\n", tgdate, msg, tgdate);

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
    
    return;
}

// Pretty print a BSP-Object with indent
void _var_dump_object(BSP_OBJECT *obj, int layer);
void _var_dump_array(BSP_OBJECT_ITEM *array, int layer);
static void _var_dump_item(BSP_OBJECT_ITEM *item, const char *curr_key)
{
    if (!item)
    {
        return;
    }

    switch (item->value.type)
    {
        case BSP_VAL_INT8 : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(INT8)\033[0m => %d\n", curr_key, get_int8(item->value.lval));
            break;
        case BSP_VAL_INT16 : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(INT16)\033[0m => %d\n", curr_key, get_int16(item->value.lval));
            break;
        case BSP_VAL_INT32 : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(INT32)\033[0m => %d\n", curr_key, get_int32(item->value.lval));
            break;
        case BSP_VAL_INT64 : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(INT64)\033[0m => %lld\n", curr_key, (long long int) get_int64(item->value.lval));
            break;
        case BSP_VAL_BOOLEAN : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(BOOLEAN)\033[0m => %s\n", curr_key, (get_int8(item->value.lval) == 0) ? "false" : "true");
            break;
        case BSP_VAL_FLOAT : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(FLOAT)\033[0m => %f\n", curr_key, get_float(item->value.lval));
            break;
        case BSP_VAL_DOUBLE : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(DOUBLE)\033[0m => %f\n", curr_key, get_double(item->value.lval));
            break;
        case BSP_VAL_STRING : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(STRING)\033[0m => ", curr_key);
            if (item->value.rval)
            {
                write(STDERR_FILENO, (char *) item->value.rval, item->value.rval_len);
            }
            else
            {
                fprintf(stderr, "### NULL STRING ###");
            }
            
            fprintf(stderr, "\n");
            break;
        case BSP_VAL_POINTER : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(POINTER)\033[0m => %p\n", curr_key, item->value.rval);
            break;
        case BSP_VAL_NULL : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(NULL)\033[0m\n", curr_key);
            break;
        default : 
            fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(UNKNOWN)\033[0m\n", curr_key);
            break;
    }
    
    return;
}

void _var_dump_object(BSP_OBJECT *obj, int layer)
{
    if (!obj)
    {
        return;
    }

    int i;
    BSP_OBJECT *next_obj = NULL;
    BSP_OBJECT_ITEM *curr = NULL;
    char curr_key[1024];

    for (i = 0; i < layer; i ++)
    {
        fprintf(stderr, "\t");
    }

    fprintf(stderr, "{\n");
    reset_object(obj);
    while ((curr = curr_item(obj)))
    {
        for (i = 0; i <= layer; i ++)
        {
            fprintf(stderr, "\t");
        }

        if (curr->key_len > 1023)
        {
            strncpy(curr_key, curr->key, 1023);
            curr_key[1023] = 0x0;
        }
        else
        {
            strncpy(curr_key, curr->key, curr->key_len);
            curr_key[curr->key_len] = 0x0;
        }

        switch (curr->value.type)
        {
            case BSP_VAL_ARRAY : 
                fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(ARRAY)\033[0m =>\n", curr_key);
                _var_dump_array(curr, layer + 1);
                break;
            case BSP_VAL_OBJECT : 
                next_obj = (BSP_OBJECT *) curr->value.rval;
                if (next_obj)
                {
                    fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(OBJECT)\033[0m => %d items\n", curr_key, (int) next_obj->nitems);
                    _var_dump_object(next_obj, layer + 1);
                }
                break;
            default : 
                _var_dump_item(curr, curr_key);
        }

        next_item(obj);
    }

    for (i = 0; i < layer; i ++)
    {
        fprintf(stderr, "\t");
    }

    fprintf(stderr, "}\n");
    
    return;
}

void _var_dump_array(BSP_OBJECT_ITEM *array, int layer)
{
    if (!array || BSP_VAL_ARRAY != array->value.type)
    {
        return;
    }

    int i;
    BSP_OBJECT *next_obj = NULL;
    BSP_OBJECT_ITEM *curr = NULL;
    BSP_OBJECT_ITEM **list = (BSP_OBJECT_ITEM **) array->value.rval;
    char curr_key[1024];

    for (i = 0; i < layer; i ++)
    {
        fprintf(stderr, "\t");
    }

    fprintf(stderr, "{\n");
    size_t idx;
    for (idx = 0; idx < array->value.rval_len; idx ++)
    {
        if (list[idx])
        {
            for (i = 0; i <= layer; i ++)
            {
                fprintf(stderr, "\t");
            }

            curr = list[idx];
            sprintf(curr_key, "%llu", (unsigned long long int) idx);
            switch (curr->value.type)
            {
                case BSP_VAL_ARRAY : 
                    fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(ARRAY)\033[0m =>\n", curr_key);
                    _var_dump_array(curr, layer + 1);
                    break;
                case BSP_VAL_OBJECT : 
                    next_obj = (BSP_OBJECT *) curr->value.rval;
                    if (next_obj)
                    {
                        fprintf(stderr, "\033[1;33m%s\033[0m \033[1;32m(OBJECT)\033[0m => %d items\n", curr_key, (int) next_obj->nitems);
                        _var_dump_object(next_obj, layer + 1);
                    }
                    break;
                default : 
                    _var_dump_item(curr, curr_key);
            }
        }
    }

    for (i = 0; i < layer; i ++)
    {
        fprintf(stderr, "\t");
    }

    fprintf(stderr, "}\n");

    return;
}

void debug_object(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }
    
    fprintf(stderr, "\n\033[1;37m=== [Debug Object %d items] === < START > ===\033[0m\n", (int) obj->nitems);
    _var_dump_object(obj, 0);
    fprintf(stderr, "\033[1;37m=== [Debug Object] === < END > ===\033[0m\n\n");

    return;
}

// Print lua stack
void debug_lua_stack(lua_State *l)
{
    if (!l)
    {
        return;
    }
    
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
    
    return;
}
