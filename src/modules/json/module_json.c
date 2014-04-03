/*
 * module_json.c
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
 * JSON encoder / decoder for lua
 * 
 * @package modules::json
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/29/2012
 * @changelog 
 *      [08/29/2012] - Creation
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "bsp.h"

#include "module_json.h"

// String operator
static inline void _json_append_str(BSP_STRING *str, const char *data, size_t len)
{
    int c;
    ssize_t i, seg = -1;
    const char *esc;
    int utf_len;
    int32_t utf_value;

    if (!str || !data)
    {
        return;
    }

    string_append(str, "\"", 1);

    for (i = 0; i < len; i ++)
    {
        c = (unsigned char) data[i];
        if (c < 0x80)
        {
            // Try escape
            esc = escape_char(c);
            if (esc)
            {
                // Last
                if (seg >= 0 && i > seg)
                {
                    string_append(str, data + seg, (i - seg));
                    seg = -1;
                }
                
                // Append escaped data
                string_append(str, esc, -1);
            }

            else
            {
                // Continue normal characters
                if (seg < 0)
                {
                    seg = i;
                }
            }
        }

        else
        {
            if (seg >= 0 && i > seg)
            {
                string_append(str, data + seg, (i - seg));
            }
            
            // Try UTF
            utf_value = utf8_to_value(data + i, (len - i), &utf_len);
            string_printf(str, "\\u%04x", utf_value);
            i += (utf_len - 1);
            seg = -1;
        }
    }

    if (seg >= 0)
    {
        string_append(str, data + seg, (len - seg));
    }
    
    string_append(str, "\"", 1);

    return;
}

// LUA table recursive
static void _json_append_obj(BSP_STRING *str, lua_State *s, int idx)
{
    if (!str || !s)
    {
        return;
    }

    if (!lua_istable(s, idx))
    {
        return;
    }

    const char *key;
    size_t key_len, len;
    int v_boolean;
    lua_Integer v_integer;
    double v_number;
    const char *v_string;
    int has_item = 0;
    size_t n = 0;
    
    while (lua_next(s, idx - 1))
    {
        n ++;
        lua_pop(s, 1);
    }
    
    if (n == luaL_len(s, idx))
    {
        // Array
        string_append(str, "[", 1);
        for (n = 1; n <= luaL_len(s, idx); n ++)
        {
            if (!lua_checkstack(s, 1))
            {
                return;
            }
            
            lua_rawgeti(s, idx, n);
            if (lua_isboolean(s, -1))
            {
                v_boolean = lua_toboolean(s, -1);
                string_append(str, v_boolean ? "true" : "false", -1);
            }
            
            else if (lua_isnumber(s, -1))
            {
                v_number = lua_tonumber(s, -1);
                // If integer?
                v_integer = lua_tointeger(s, -1);
                if ((double) v_integer == v_number)
                {
                    // Integer
                    string_printf(str, "%lld", (long long) v_integer);
                }

                else
                {
                    // Float
                    string_printf(str, "%.14g", v_number);
                }
            }

            else if (lua_isstring(s, -1))
            {
                v_string = lua_tolstring(s, -1, &len);
                _json_append_str(str, v_string, len);
            }

            else if (lua_istable(s, -1))
            {
                _json_append_obj(str, s, -1);
            }

            else
            {
                string_append(str, "null", 4);
            }

            has_item = 1;
            string_append(str, ",", 1);
            
            lua_pop(s, 1);
        }

        if (has_item)
        {
            str->str[str->ori_len - 1] = ']';
        }

        else
        {
            string_append(str, "]", 1);
        }
    }

    else
    {
        // Object
        string_append(str, "{", 1);
        if (!lua_checkstack(s, 2))
        {
            return;
        }
        lua_pushnil(s);
        while (lua_next(s, idx - 1))
        {
            // Read key
            lua_pushvalue(s, -2);
            key = lua_tolstring(s, -1, &key_len);

            _json_append_str(str, key, key_len);
            lua_pop(s, 1);

            // Key/Value, a ':'
            string_append(str, ":", 1);

            // Value
            if (lua_isboolean(s, -1))
            {
                v_boolean = lua_toboolean(s, -1);
                string_append(str, v_boolean ? "true" : "false", -1);
            }
            
            else if (lua_isnumber(s, -1))
            {
                v_number = lua_tonumber(s, -1);
                // If integer?
                v_integer = lua_tointeger(s, -1);
                if ((double) v_integer == v_number)
                {
                    // Integer
                    string_printf(str, "%lld", (long long) v_integer);
                }

                else
                {
                    // Float
                    string_printf(str, "%.14g", v_number);
                }
            }

            else if (lua_isstring(s, -1))
            {
                v_string = lua_tolstring(s, -1, &len);
                _json_append_str(str, v_string, len);
            }

            else if (lua_istable(s, -1))
            {
                _json_append_obj(str, s, -1);
            }

            else
            {
                string_append(str, "null", 4);
            }

            has_item = 1;
            string_append(str, ",", 1);

            lua_pop(s, 1);
        }

        // Last ',' ?
        if (has_item)
        {
            str->str[str->ori_len - 1] = '}';
        }

        else
        {
            string_append(str, "}", 1);
        }
    }
}

// Decode string to normal
static inline void _json_decode_str(BSP_STRING *str, const char *data, ssize_t len)
{
    if (!str || !data)
    {
        return;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    ssize_t i, seg = -1;
    char c;
    char utf[5];
    char utf_data[8];
    long utf_value;

    for (i = 0; i < len; i ++)
    {
        if (data[i] == '\\')
        {
            // In escape
            if (seg >= 0 && seg < i)
            {
                string_append(str, data + seg, (i - seg));
                seg = -1;
            }
            
            if (len - i >= 1)
            {
                c = data[i + 1];
                i ++;
                switch (c)
                {
                    case 'b' : 
                        string_append(str, "\b", 1);
                        break;

                    case 't' : 
                        string_append(str, "\t", 1);
                        break;

                    case 'n' : 
                        string_append(str, "\n", 1);
                        break;

                    case 'f' : 
                        string_append(str, "\f", 1);
                        break;

                    case 'r' : 
                        string_append(str, "\r", 1);
                        break;

                    case '"' : 
                        string_append(str, "\"", 1);
                        break;

                    case '/' : 
                        string_append(str, "/", 1);
                        break;

                    case '\\' : 
                        string_append(str, "\\", 1);
                        break;

                    case 'u' : 
                        // UTF code
                        if (len - i > 4)
                        {
                            memcpy(utf, data + i + 1, 4);
                            utf[4] = 0x0;
                            utf_value = strtol(utf, NULL, 16);

                            if (utf_value < 0x80)
                            {
                                utf_data[0] = utf_value;
                                string_append(str, utf_data, 1);
                            }

                            else if (utf_value < 0x800)
                            {
                                // 2 bytes
                                utf_data[0] = ((utf_value >> 6) & 0x1f) | 0xc0;
                                utf_data[1] = (utf_value & 0x3f) | 0x80;
                                string_append(str, utf_data, 2);
                            }

                            else
                            {
                                // 3 bytes
                                utf_data[0] = ((utf_value >> 12) & 0x0f) | 0xe0;
                                utf_data[1] = ((utf_value >> 6) & 0x3f) | 0x80;
                                utf_data[2] = (utf_value & 0x3f) | 0x80;
                                string_append(str, utf_data, 3);
                            }

                            i += 4;
                        }

                        else
                        {
                            string_append(str, "u", 1);
                        }
                        
                        break;

                    default : 
                        break;
                }
            }
        }

        else
        {
            // Normal char
            if (seg < 0)
            {
                seg = i;
            }
        }
    }

    if (seg >= 0 && seg < len)
    {
        string_append(str, data + seg, (len - seg));
    }

    return;
}

// Decode json string to lua value (not on table).
static int _json_decode_item(const char *input, size_t len, lua_State *s, int layer)
{
    int in_str = 0, str_start = 0, is_value = 0;
    double v_number = 0;
    size_t i, next_len, idx = 1;
    char *digit_next;
    BSP_STRING *str = new_string(NULL, 0);
    
    for (i = 0; i < len; i ++)
    {
        if (input[i] == '"')
        {
            if (i > 0 && input[i - 1] == '\\')
            {
                continue;
            }
            
            if (in_str && str_start >= 0)
            {
                // String end
                clean_string(str);
                _json_decode_str(str, input + str_start + 1, (i - str_start - 1));
                if (!lua_checkstack(s, 1))
                {
                    return i;
                }
                lua_pushlstring(s, str->str, str->ori_len);
                if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
                {
                    // Write index
                    lua_rawseti(s, -2, idx ++);
                }

                else if (layer == JSON_LAYER_OBJECT)
                {
                    if (!lua_istable(s, -2) && lua_istable(s, -3) && is_value)
                    {
                        lua_settable(s, -3);
                        is_value = 0;
                    }

                    else
                    {
                        is_value = 1;
                    }
                }

                else
                {
                    // Single value?
                }
                
                in_str = 0;
            }

            else
            {
                in_str = 1;
                str_start = i;
            }
        }

        else if (!in_str && input[i] == '{')
        {
            if (!lua_checkstack(s, 1))
            {
                return i;
            }
            
            lua_newtable(s);
            next_len = _json_decode_item(input + i + 1, len - i - 1, s, JSON_LAYER_OBJECT);
            if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
            {
                // Write index
                lua_rawseti(s, -2, idx ++);
            }

            else if (layer == JSON_LAYER_OBJECT)
            {
                if ((lua_isnumber(s, -2) || lua_isstring(s, -2)) && lua_istable(s, -3) && is_value)
                {
                    lua_settable(s, -3);
                    is_value = 0;
                }

                else
                {
                    lua_pop(s, 1);
                    if (!lua_checkstack(s, 1))
                    {
                        return i;
                    }
                    lua_pushstring(s, JSON_INVALID_KEY);
                    is_value = 1;
                }
            }

            else
            {
                // Single object
            }
            
            i += next_len;
        }

        else if (!in_str && input[i] == '}' && layer == JSON_LAYER_OBJECT)
        {
            if (1 == is_value)
            {
                // Last item
                if (!lua_checkstack(s, 1))
                {
                    return i;
                }
                lua_pushnil(s);

                if (lua_istable(s, -3))
                {
                    lua_settable(s, -3);
                }
            }

            return i + 1;
        }

        else if (!in_str && input[i] == '[')
        {
            lua_newtable(s);
            next_len = _json_decode_item(input + i + 1, len - i - 1, s, JSON_LAYER_ARRAY);
            if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
            {
                // Write index
                lua_rawseti(s, -2, idx ++);
            }

            else if (layer == JSON_LAYER_OBJECT)
            {
                if ((lua_isnumber(s, -2) || lua_isstring(s, -2)) && lua_istable(s, -3) && is_value)
                {
                    lua_settable(s, -3);
                    is_value = 0;
                }

                else
                {
                    lua_pop(s, 1);
                    if (!lua_checkstack(s, 1))
                    {
                        return i;
                    }
                    lua_pushstring(s, JSON_INVALID_KEY);
                    is_value = 1;
                }
            }

            else
            {
                // Single array
            }
            
            i += next_len;
        }

        else if (!in_str && input[i] == ']' && layer == JSON_LAYER_ARRAY)
        {
            del_string(str);
            
            return i + 1;
        }

        else if (!in_str)
        {
            // Boolean
            if (len - i >= 4 && 0 == strncasecmp(input + i, "true", 4))
            {
                if (!lua_checkstack(s, 1))
                {
                    return i;
                }
                lua_pushboolean(s, 1);
                if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
                {
                    // Write index
                    lua_rawseti(s, -2, idx ++);
                }

                else if (layer == JSON_LAYER_OBJECT)
                {
                    if (!lua_istable(s, -2) && lua_istable(s, -3) && is_value)
                    {
                        lua_settable(s, -3);
                        is_value = 0;
                    }

                    else
                    {
                        lua_pop(s, 1);
                        if (!lua_checkstack(s, 1))
                        {
                            return i;
                        }
                        lua_pushstring(s, JSON_INVALID_KEY);
                        is_value = 1;
                    }
                }

                else
                {
                    // Single value?
                }

                i += 3;
            }

            else if (len - i >= 5 && 0 == strncasecmp(input + i, "false", 5))
            {
                if (!lua_checkstack(s, 1))
                {
                    return i;
                }
                lua_pushboolean(s, 0);
                if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
                {
                    // Write index
                    lua_rawseti(s, -2, idx ++);
                }

                else if (layer == JSON_LAYER_OBJECT)
                {
                    if (!lua_istable(s, -2) && lua_istable(s, -3) && is_value)
                    {
                        lua_settable(s, -3);
                        is_value = 0;
                    }

                    else
                    {
                        lua_pop(s, 1);
                        if (!lua_checkstack(s, 1))
                        {
                            return i;
                        }
                        lua_pushstring(s, JSON_INVALID_KEY);
                        is_value = 1;
                    }
                }

                else
                {
                    // Single value?
                }

                i += 4;
            }

            // null
            else if (len - i >= 4 && 0 == strncasecmp(input + i, "null", 4))
            {
                if (!lua_checkstack(s, 1))
                {
                    return i;
                }
                lua_pushnil(s);
                if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
                {
                    // Write index
                    lua_rawseti(s, -2, idx ++);
                }

                else if (layer == JSON_LAYER_OBJECT)
                {
                    if (!lua_istable(s, -2) && lua_istable(s, -3) && is_value)
                    {
                        lua_settable(s, -3);
                        is_value = 0;
                    }

                    else
                    {
                        lua_pop(s, 1);
                        if (!lua_checkstack(s, 1))
                        {
                            return i;
                        }
                        lua_pushstring(s, JSON_INVALID_KEY);
                        is_value = 1;
                    }
                }

                else
                {
                    // Single value?
                }

                i += 3;
            }
            
            // Digital
            else
            {
                v_number = strtod(input + i, &digit_next);
                if (digit_next > (input + i))
                {
                    // Convert successfully
                    if (!lua_checkstack(s, 1))
                    {
                        return i;
                    }
                    lua_pushnumber(s, v_number);
                    if (layer == JSON_LAYER_ARRAY && lua_istable(s, -2))
                    {
                        // Write index
                        lua_rawseti(s, -2, idx ++);
                    }

                    else if (layer == JSON_LAYER_OBJECT)
                    {
                        if ((lua_isnumber(s, -2) || lua_isstring(s, -2)) && lua_istable(s, -3) && is_value)
                        {
                            lua_settable(s, -3);
                            is_value = 0;
                        }

                        else
                        {
                            is_value = 1;
                        }
                    }

                    else
                    {
                        // Single value?
                    }
                    i = digit_next - input - 1;
                }
            }
        }

        else
        {
            // Just ignore
        }
    }

    del_string(str);
    
    return i;
}

static int json_decode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        return 0;
    }

    if (!lua_isstring(s, -1))
    {
        return 0;
    }

    size_t len;
    const char *json_str = lua_tolstring(s, -1, &len);
    _json_decode_item(json_str, len, s, JSON_LAYER_TOP);
    
    return 1;
}

// Encode LUA value to json stream
static int json_encode(lua_State *s)
{
    if (!s || lua_gettop(s) < 1)
    {
        // Nothing to do
        return 0;
    }

    BSP_STRING *str = new_string(NULL, 0);
    int v_boolean;
    lua_Integer v_integer;
    double v_number;
    const char *v_string;
    size_t len;
    
    // Only one value
    switch (lua_type(s, -1))
    {
        case LUA_TNIL : 
            string_append(str, "null", 4);
            break;
        
        case LUA_TBOOLEAN : 
            v_boolean = lua_toboolean(s, -1);
            string_append(str, v_boolean ? "true" : "false", -1);
            break;

        case LUA_TNUMBER : 
            v_number = lua_tonumber(s, -1);
            // If integer?
            v_integer = lua_tointeger(s, -1);
            if ((double) v_integer == v_number)
            {
                // Integer
                string_printf(str, "%lld", (long long) v_integer);
            }

            else
            {
                // Float
                string_printf(str, "%.14g", v_number);
            }
            
            break;

        case LUA_TSTRING : 
            v_string = lua_tolstring(s, -1, &len);
            _json_append_str(str, v_string, len);
            
            break;

        case LUA_TTABLE : 
            _json_append_obj(str, s, -1);
            
            break;

        default : 
            // Other types, we do not support encode an un-readable value into string ~_~
            break;
    }

    lua_pop(s, 1);
    
    // Write back
    if (!lua_checkstack(s, 1))
    {
        return 0;
    }
    lua_pushlstring(s, str->str, str->ori_len);

    return 1;
}

int bsp_module_json(lua_State *s)
{
    if (!lua_checkstack(s, 1))
    {
        return 0;
    }
    lua_pushcfunction(s, json_decode);
    lua_setglobal(s, "bsp_json_decode");

    lua_pushcfunction(s, json_encode);
    lua_setglobal(s, "bsp_json_encode");

    lua_settop(s, 0);

    return 0;
}
