/*
 * json.c
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
 * Native JSON encoder / decoder
 * 
 * @package libbsp::core
 * @author Dr.NP <np@bsgroup.org>
 * @update 01/28/2014
 * @chagelog 
 *      [01/14/2014] - Creation
 *      [01/28/2014] - next_item() mixed    
 */

#include "bsp.h"

static void _json_append_obj(BSP_STRING *str, BSP_OBJECT *obj);
static void _json_append_arr(BSP_STRING *str, BSP_OBJECT_ITEM *arr);
// String operator
static inline void _json_append_str(BSP_STRING *str, const char *data, ssize_t len)
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

    if (len < 0)
    {
        len = strlen(data);
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

// Append value
static void _json_append_value(BSP_STRING *str, BSP_OBJECT_ITEM *item)
{
    if (!str || !item)
    {
        return;
    }

    BSP_OBJECT *next_obj;
    switch (item->value.type)
    {
        case BSP_VAL_ARRAY : 
            _json_append_arr(str, item);
            break;
        case BSP_VAL_OBJECT : 
            next_obj = (BSP_OBJECT *) item->value.rval;
            _json_append_obj(str, next_obj);
            break;
        case BSP_VAL_BOOLEAN : 
            string_append(str, get_int8(item->value.lval) ? "true" : "false", -1);
            break;
        case BSP_VAL_FLOAT : 
            string_printf(str, "%.14g", get_float(item->value.lval));
            break;
        case BSP_VAL_DOUBLE : 
            string_printf(str, "%.14g", get_double(item->value.lval));
            break;
        case BSP_VAL_INT8 : 
            string_printf(str, "%d", get_int8(item->value.lval));
            break;
        case BSP_VAL_INT16 : 
            string_printf(str, "%d", get_int16(item->value.lval));
            break;
        case BSP_VAL_INT32 : 
            string_printf(str, "%d", get_int32(item->value.lval));
            break;
        case BSP_VAL_INT64 : 
            string_printf(str, "%lld", get_int64(item->value.lval));
            break;
        case BSP_VAL_STRING : 
            _json_append_str(str, (const char *) item->value.rval, item->value.rval_len);
            break;
        case BSP_VAL_NULL : 
        default : 
            string_append(str, "null", 4);
            break;
    }

    return;
}

// Array recursive
static void _json_append_arr(BSP_STRING *str, BSP_OBJECT_ITEM *arr)
{
    if (!str || !arr || BSP_VAL_ARRAY != arr->value.type)
    {
        return;
    }

    //BSP_OBJECT *next_obj = NULL;
    BSP_OBJECT_ITEM *curr = NULL;
    BSP_OBJECT_ITEM **list = (BSP_OBJECT_ITEM **) arr->value.rval;
    size_t idx;
    int has_item = 0;
    
    string_append(str, "[", 1);
    for (idx = 0; idx < arr->value.rval_len; idx ++)
    {
        if (list[idx])
        {
            curr = list[idx];
            if (has_item)
            {
                string_append(str, ",", 1);
            }
            _json_append_value(str, curr);
            has_item = 1;
        }
    }
    string_append(str, "]", 1);
    
    return;
}

// Object recursive
static void _json_append_obj(BSP_STRING *str, BSP_OBJECT *obj)
{
    if (!str || !obj)
    {
        return;
    }

    int has_item = 0;
    BSP_OBJECT_ITEM *curr = NULL;
    
    reset_object(obj);
    string_append(str, "{", 1);
    while ((curr = curr_item(obj)))
    {
        if (has_item)
        {
            string_append(str, ",", 1);
        }

        // Read key
        _json_append_str(str, curr->key, curr->key_len);
        string_append(str, ":", 1);
        
        // Value
        _json_append_value(str, curr);
        has_item = 1;
        next_item(obj);
    }
    string_append(str, "}", 1);

    return;
}

// Decode string to normal
static int _json_decode_arr(const char *data, ssize_t len, BSP_OBJECT_ITEM *arr);
static int _json_decode_obj(const char *data, ssize_t len, BSP_OBJECT *obj);
static inline void _json_decode_int(BSP_OBJECT_ITEM *item, int64_t data)
{
    if (!item)
    {
        return;
    }

    if (0 != llabs(data) >> 31)
    {
        set_item_int64(item, (const int64_t) data);
    }
    else if (0 != llabs(data) >> 15)
    {
        set_item_int32(item, (const int32_t) data);
    }
    else if (0 != llabs(data) >> 7)
    {
        set_item_int16(item, (const int16_t) data);
    }
    else
    {
        set_item_int8(item, (const int8_t) data);
    }

    return;
}

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

// Decode data
static int _json_decode_arr(const char *data, ssize_t len, BSP_OBJECT_ITEM *arr)
{
    if (!data || !arr)
    {
        return 0;
    }

    if (len < 0)
    {
        len = strlen(data);
    }
    
    int in_str = 0, str_start = 0;
    size_t i, next_len, idx = 0;
    char *digit_next;
    double v_number;
    BSP_STRING *str = new_string(NULL, 0);
    BSP_OBJECT_ITEM *curr_item = NULL;
    BSP_OBJECT *next_obj;
    
    for (i = 0; i < len; i ++)
    {
        if (data[i] == '"')
        {
            // String
            if (i > 0 && data[i - 1] == '\\')
            {
                continue;
            }
            
            if (in_str && str_start >= 0)
            {
                // String end
                clean_string(str);
                _json_decode_str(str, data + str_start + 1, (i - str_start - 1));
                curr_item = new_object_item(NULL, 0);
                if (curr_item)
                {
                    set_item_string(curr_item, STR_STR(str), STR_LEN(str));
                    array_set_item(arr, curr_item, idx ++);
                }
                in_str = 0;
            }
            else
            {
                in_str = 1;
                str_start = i;
            }
        }
        else if (!in_str)
        {
            if (data[i] == '{')
            {
                // Next object
                curr_item = new_object_item(NULL, 0);
                if (curr_item)
                {
                    next_obj = new_object();
                    next_len = _json_decode_obj(data + i + 1, len - i - 1, next_obj);
                    set_item_object(curr_item, next_obj);
                    array_set_item(arr, curr_item, idx ++);
                    i += next_len;
                }
            }
            else if (data[i] == '[')
            {
                // Next array
                curr_item = new_object_item(NULL, 0);
                {
                    if (curr_item)
                    {
                        set_item_array(curr_item);
                        next_len = _json_decode_arr(data + i + 1, len - i - 1, curr_item);
                        array_set_item(arr, curr_item, idx ++);
                        i += next_len;
                    }
                }
            }
            else if (data[i] == ']')
            {
                // Array ending
                del_string(str);
                return i + 1;
            }
            else if (len - i >= 4 && 0 == strncasecmp(data + i, "true", 4))
            {
                // Boolean true
                curr_item = new_object_item(NULL, 0);
                set_item_boolean(curr_item, 1);
                array_set_item(arr, curr_item, idx ++);
                i += 3;
            }
            else if (len - i >= 5 && 0 == strncasecmp(data + i, "false", 5))
            {
                // Boolean false
                curr_item = new_object_item(NULL, 0);
                set_item_boolean(curr_item, 0);
                array_set_item(arr, curr_item, idx ++);
                i += 4;
            }
            else if (len - i >= 4 && 0 == strncasecmp(data + i, "null", 4))
            {
                // Null
                curr_item = new_object_item(NULL, 0);
                set_item_null(curr_item);
                array_set_item(arr, curr_item, idx ++);
                i += 3;
            }
            else
            {
                // Digital
                curr_item = new_object_item(NULL, 0);
                v_number = strtod(data + i, &digit_next);
                if (curr_item && digit_next > (data + i))
                {
                    double intpart;
                    if (0.0 == modf(v_number, &intpart))
                    {
                        // Integer
                        _json_decode_int(curr_item, (int64_t) v_number);
                    }
                    else
                    {
                        // Double
                        set_item_double(curr_item, v_number);
                    }
                    array_set_item(arr, curr_item, idx ++);
                    i = digit_next - data - 1;
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

static int _json_decode_obj(const char *data, ssize_t len, BSP_OBJECT *obj)
{
    if (!data || !obj)
    {
        return 0;
    }
    
    if (len < 0)
    {
        len = strlen(data);
    }
    
    int in_str = 0, str_start = 0, is_value = 0;
    size_t i, next_len;
    char *digit_next;
    double v_number;
    BSP_STRING *str = new_string(NULL, 0);
    BSP_OBJECT_ITEM *curr_item = NULL;
    BSP_OBJECT *next_obj;

    for (i = 0; i < len; i ++)
    {
        if (data[i] == '"')
        {
            // String
            if (i > 0 && data[i - 1] == '\\')
            {
                continue;
            }
            
            if (in_str && str_start >= 0)
            {
                // String end
                clean_string(str);
                _json_decode_str(str, data + str_start + 1, (i - str_start - 1));
                if (is_value)
                {
                    // Value
                    if (curr_item)
                    {
                        set_item_string(curr_item, STR_STR(str), STR_LEN(str));
                        object_insert_item(obj, curr_item);
                        is_value = 0;
                        curr_item = NULL;
                    }
                }
                else
                {
                    // Key
                    curr_item = new_object_item(STR_STR(str), STR_LEN(str));
                    is_value = 1;
                }
                in_str = 0;
            }
            else
            {
                in_str = 1;
                str_start = i;
            }
        }
        else if (!in_str && is_value)
        {
            if (data[i] == '{')
            {
                // Next object
                if (curr_item)
                {
                    next_obj = new_object();
                    next_len = _json_decode_obj(data + i + 1, len - i - 1, next_obj);
                    set_item_object(curr_item, next_obj);
                    object_insert_item(obj, curr_item);
                    i += next_len;
                    is_value = 0;
                    curr_item = NULL;
                }
            }
            else if (data[i] == '[')
            {
                // Next array
                if (curr_item)
                {
                    set_item_array(curr_item);
                    next_len = _json_decode_arr(data + i + 1, len - i - 1, curr_item);
                    object_insert_item(obj, curr_item);
                    i += next_len;
                    is_value = 0;
                    curr_item = NULL;
                }
            }
            else if (len - i >= 4 && 0 == strncasecmp(data + i, "true", 4))
            {
                // Boolean true
                if (curr_item)
                {
                    set_item_boolean(curr_item, 1);
                    object_insert_item(obj, curr_item);
                    is_value = 0;
                    curr_item = NULL;
                }
                i += 3;
            }
            else if (len - i >= 5 && 0 == strncasecmp(data + i, "false", 5))
            {
                // Boolean false
                if (curr_item)
                {
                    set_item_boolean(curr_item, 0);
                    object_insert_item(obj, curr_item);
                    is_value = 0;
                    curr_item = NULL;
                }
                i += 4;
            }
            else if (len - i >= 4 && 0 == strncasecmp(data + i, "null", 4))
            {
                // Null
                if (curr_item)
                {
                    set_item_null(curr_item);
                    object_insert_item(obj, curr_item);
                    is_value = 0;
                    curr_item = NULL;
                }
                i += 3;
            }
            else
            {
                // Digital
                v_number = strtod(data + i, &digit_next);
                if (digit_next > (data + i))
                {
                    if (curr_item)
                    {
                        double intpart;
                        if (0.0 == modf(v_number, &intpart))
                        {
                            // Integer
                            _json_decode_int(curr_item, (int64_t) v_number);
                        }
                        else
                        {
                            // Double
                            set_item_double(curr_item, v_number);
                        }
                        object_insert_item(obj, curr_item);
                        is_value = 0;
                        curr_item = NULL;
                    }
                    i = digit_next - data - 1;
                }
            }
        }
        else
        {
            if (data[i] == '}')
            {
                del_string(str);
                return i + 1;
            }
        }
    }
    del_string(str);
    
    return 0;
}

int json_nd_encode(BSP_OBJECT *obj, BSP_STRING *str)
{
    if (!obj || !str)
    {
        return 0;
    }

    _json_append_obj(str, obj);

    return 0;
}

int json_nd_decode(const char *data, ssize_t len, BSP_OBJECT *obj)
{
    if (!data || !obj)
    {
        return 0;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    size_t i;
    for (i = 0; i < len; i ++)
    {
        // Support object only
        if ('{' == data[i])
        {
            _json_decode_obj(data + i + 1, len - i - 1, obj);
            break;
        }
    }

    return 0;
}
