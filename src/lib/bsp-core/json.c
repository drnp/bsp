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
 *      [08/18/2014[ - Recode
 */

#include "bsp.h"

/* === SERIALIZE === */
static inline void _append_string_to_json(BSP_STRING *str, BSP_STRING *src)
{
    if (str && src && !str->is_const)
    {
        size_t i;
        unsigned char c;
        const char *esc;
        int utf_len;
        int32_t utf_value;
        bsp_spin_lock(&src->lock);
        for (i = 0; i < STR_LEN(src); i ++)
        {
            c = (unsigned char) STR_STR(src)[i];
            if (c < 0x80)
            {
                esc = escape_char(c);
                if (esc)
                {
                    // Escaped char
                    string_append(str, esc, -1);
                }
                else
                {
                    // Normal
                    string_append(str, STR_STR(src) + i, 1);
                }
            }
            else
            {
                // UTF
                utf_value = utf8_to_value(STR_STR(src) + i, STR_LEN(src) - i, &utf_len);
                string_printf(str, "\\u%04x", utf_value);
                i += (utf_len - 1);
            }
        }
        bsp_spin_unlock(&src->lock);
    }

    return;
}

static void _append_key_to_json(BSP_STRING *str, BSP_STRING *key);
static void _append_value_to_json(BSP_STRING *str, BSP_VALUE *val);
static void _append_object_to_json(BSP_STRING *str, BSP_OBJECT *obj);

static void _append_key_to_json(BSP_STRING *str, BSP_STRING *key)
{
    if (!str || !key || str->is_const)
    {
        return;
    }

    string_append(str, "\"", 1);
    _append_string_to_json(str, key);
    string_append(str, "\"", 1);

    return;
}

static void _append_value_to_json(BSP_STRING *str, BSP_VALUE *val)
{
    if (!str || !val || str->is_const)
    {
        return;
    }

    int vlen = 0;
    BSP_STRING *src = NULL;
    BSP_OBJECT *sub_obj = NULL;
    switch (val->type)
    {
        case BSP_VAL_NULL : 
            string_append(str, "null", 4);
            break;
        case BSP_VAL_INT : 
            string_printf(str, "%lld", get_vint(val->lval, &vlen));
            break;
        case BSP_VAL_FLOAT : 
            string_printf(str, "%.14g", get_float(val->lval));
            break;
        case BSP_VAL_DOUBLE : 
            string_printf(str, "%.14g", get_double(val->lval));
            break;
        case BSP_VAL_BOOLEAN_TRUE : 
            string_append(str, "true", 4);
            break;
        case BSP_VAL_BOOLEAN_FALSE : 
            string_append(str, "false", 5);
            break;
        case BSP_VAL_STRING : 
            src = (BSP_STRING *) val->rval;
            string_append(str, "\"", 1);
            _append_string_to_json(str, src);
            string_append(str, "\"", 1);
            break;
        case BSP_VAL_OBJECT : 
            sub_obj = (BSP_OBJECT *) val->rval;
            _append_object_to_json(str, sub_obj);
            break;
        default : 
            break;
    }

    return;
}

static void _append_object_to_json(BSP_STRING *str, BSP_OBJECT *obj)
{
    if (!str || !obj || str->is_const)
    {
        return;
    }

    BSP_VALUE *val = NULL;
    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    switch (obj->type)
    {
        case OBJECT_TYPE_SINGLE : 
            // Single
            val = (BSP_VALUE *) obj->node;
            _append_value_to_json(str, val);
            break;
        case OBJECT_TYPE_ARRAY : 
            // Array
            array = (struct bsp_array_t *) obj->node;
            size_t idx;
            if (array)
            {
                string_append(str, "[", 1);
                for (idx = 0; idx < array->nitems; idx ++)
                {
                    size_t bucket = idx / ARRAY_BUCKET_SIZE;
                    size_t seq = idx % ARRAY_BUCKET_SIZE;
                    if (bucket < array->nbuckets && array->items[bucket])
                    {
                        val = array->items[bucket][seq];
                    }
                    else
                    {
                        val = NULL;
                    }
                    _append_value_to_json(str, val);
                    if (idx < array->nitems - 1)
                    {
                        string_append(str, ",", 1);
                    }
                }
                string_append(str, "]", 1);
            }
            break;
        case OBJECT_TYPE_HASH : 
            // Hash
            hash = (struct bsp_hash_t *) obj->node;
            BSP_STRING *key;
            if (hash)
            {
                string_append(str, "{", 1);
                reset_object(obj);
                val = curr_item(obj);
                while (val)
                {
                    key = curr_hash_key(obj);
                    if (key)
                    {
                        _append_key_to_json(str, key);
                        string_append(str, ":", 1);
                        _append_value_to_json(str, val);
                    }
                    next_item(obj);
                    val = curr_item(obj);
                    if (val)
                    {
                        string_append(str, ",", 1);
                    }
                }
                string_append(str, "}", 1);
            }
            break;
        case OBJECT_TYPE_UNDETERMINED : 
        default : 
            break;
    }

    return;
}

BSP_STRING * json_nd_encode(BSP_OBJECT *obj)
{
    BSP_STRING *json = NULL;
    if (obj)
    {
        json = new_string(NULL, 0);
        _append_object_to_json(json, obj);
    }

    return json;
}

/* === UNSERIALIZE === */
static BSP_VALUE * _get_value_from_json(BSP_STRING *str);
static void _traverse_json_array(BSP_OBJECT *obj, BSP_STRING *str);
static void _traverse_json_hash(BSP_OBJECT *obj, BSP_STRING *str);

static BSP_VALUE * _get_value_from_json(BSP_STRING *str)
{
    if (!str)
    {
        return NULL;
    }

    BSP_VALUE *ret = NULL;
    unsigned char c;
    char status = 0;
    char utf[5];
    char utf_data[4];
    long int utf_value;
    BSP_STRING *v_str = NULL;
    char *digit_end = NULL;
    double intpart;
    double v_digit = 0.0f;
    BSP_OBJECT *v_obj = NULL;
    int end = 0;
    size_t normal = 0;

    while (STR_REMAIN(str) > 0)
    {
        c = STR_CHAR(str);
        //fprintf(stderr, "%d => %03d => %c\n", (int) STR_NOW(str), c, c);
        if (status & JSON_DECODE_STATUS_STR)
        {
            // In string
            if (('\\' == c) && 0 == (status & JSON_DECODE_STATUS_STRIP))
            {
                // Strip
                status |= JSON_DECODE_STATUS_STRIP;
                // If normal remain
                if (STR_NOW(str) > normal && v_str)
                {
                    string_append(v_str, STR_STR(str) + normal, (STR_NOW(str) - normal));
                }
            }

            if (status & JSON_DECODE_STATUS_STRIP)
            {
                switch (c)
                {
                    case '\\' : 
                        string_append(v_str, "\\", 1);
                        break;
                    case '/' : 
                        string_append(v_str, "/", 1);
                        break;
                    case '"' : 
                        string_append(v_str, "\"", 1);
                        break;
                    case 'b' : 
                        string_append(v_str, "\b", 1);
                        break;
                    case 't' : 
                        string_append(v_str, "\t", 1);
                        break;
                    case 'n' : 
                        string_append(v_str, "\n", 1);
                        break;
                    case 'f' : 
                        string_append(v_str, "\f", 1);
                        break;
                    case 'r' : 
                        string_append(v_str, "\r", 1);
                        break;
                    case 'u' : 
                        // Unicode
                        if (STR_REMAIN(str) > 4)
                        {
                            memcpy(utf, STR_CURR(str) + 1, 4);
                            utf[4] = 0x0;
                            utf_value = strtol(utf, NULL, 16);

                            if (utf_value < 0x80)
                            {
                                utf_data[0] = utf_value;
                                string_append(v_str, utf_data, 1);
                            }
                            else if (utf_value < 0x800)
                            {
                                // 2 bytes
                                utf_data[0] = ((utf_value >> 6) & 0x1f) | 0xc0;
                                utf_data[1] = (utf_value & 0x3f) | 0x80;
                                string_append(v_str, utf_data, 2);
                            }
                            else
                            {
                                // 3 bytes
                                utf_data[0] = ((utf_value >> 12) & 0x0f) | 0xe0;
                                utf_data[1] = ((utf_value >> 6) & 0x3f) | 0x80;
                                utf_data[2] = (utf_value & 0x3f) | 0x80;
                                string_append(v_str, utf_data, 3);
                            }
                            str->cursor += 4;
                        }
                        else
                        {
                            string_append(v_str, "u", 1);  
                        }
                        break;
                    default : 
                        break;
                }

                status &= ~JSON_DECODE_STATUS_STRIP;
                STR_NEXT(str);
                normal = str->cursor;
            }
            else
            {
                if ('"' == c && v_str)
                {
                    // String exit
                    status &= ~JSON_DECODE_STATUS_STR;
                    if (STR_NOW(str) > normal)
                    {
                        string_append(v_str, STR_STR(str) + normal, (STR_NOW(str) - normal));
                    }
                    ret = new_value();
                    value_set_string(ret, v_str);
                    end = 1;
                }
                else
                {
                    // Normal
                    // Just append
                }
                STR_NEXT(str);
            }
        }
        else
        {
            if ('"' == c)
            {
                // String
                status |= JSON_DECODE_STATUS_STR;
                v_str = new_string(NULL, 0);
                STR_NEXT(str);
                normal = str->cursor;
            }
            else if ('{' == c)
            {
                // A new hash
                v_obj = new_object(OBJECT_TYPE_HASH);
                STR_NEXT(str);
                _traverse_json_hash(v_obj, str);
                ret = new_value();
                value_set_object(ret, v_obj);
                end = 1;
            }
            else if ('}' == c)
            {
                // hash endding
                ret = new_value();
                ret->type = BSP_VAL_OBJECT_HASH_END;
                STR_NEXT(str);
                end = 1;
            }
            else if ('[' == c)
            {
                // A new array
                v_obj = new_object(OBJECT_TYPE_ARRAY);
                STR_NEXT(str);
                _traverse_json_array(v_obj, str);
                ret = new_value();
                value_set_object(ret, v_obj);
                end = 1;
            }
            else if (']' == c)
            {
                // Array endding
                ret = new_value();
                ret->type = BSP_VAL_OBJECT_ARRAY_END;
                STR_NEXT(str);
                end = 1;
            }
            else if (STR_REMAIN(str) >= 4 && 0 == strncmp("null", STR_CURR(str), 4))
            {
                // null
                ret = new_value();
                value_set_null(ret);
                str->cursor += 4;
                end = 1;
            }
            else if (STR_REMAIN(str) >= 4 && 0 == strncmp("true", STR_CURR(str), 4))
            {
                // Boolean true
                ret = new_value();
                value_set_boolean_true(ret);
                str->cursor += 4;
                end = 1;
            }
            else if (STR_REMAIN(str) >= 5 && 0 == strncmp("false", STR_CURR(str), 5))
            {
                // Boolean false
                ret = new_value();
                value_set_boolean_false(ret);
                str->cursor += 5;
                end = 1;
            }
            else if (c > 0x20)
            {
                // Digit
                errno = 0;
                v_digit = strtod(STR_CURR(str), &digit_end);
                if (digit_end && digit_end > STR_CURR(str) && !errno)
                {
                    // Has value
                    ret = new_value();
                    if (0.0f == modf(v_digit, &intpart))
                    {
                        // Integer
                        value_set_int(ret, (const int64_t) v_digit);
                    }
                    else
                    {
                        // Double
                        value_set_double(ret, (const double) v_digit);
                    }
                    str->cursor += (digit_end - STR_CURR(str));
                    end = 1;
                }
                else
                {
                    // Other char
                    STR_NEXT(str);
                }
            }
            else
            {
                // Just ignore
                STR_NEXT(str);
            }
        }
        if (end)
        {
            break;
        }
    }
    // Non-closed object
    if (status & JSON_DECODE_STATUS_STR && v_str)
    {
        if (STR_NOW(str) > normal)
        {
            string_append(v_str, STR_STR(str) + normal, (STR_NOW(str) - normal));
        }
        ret = new_value();
        value_set_string(ret, v_str);
    }

    return ret;
}

static void _traverse_json_array(BSP_OBJECT *obj, BSP_STRING *str)
{
    if (!obj || OBJECT_TYPE_ARRAY != obj->type)
    {
        return;
    }

    BSP_VALUE *val = NULL;
    // One by one
    while (1)
    {
        val = _get_value_from_json(str);
        if (!val)
        {
            // unwilling break
            break;
        }
        if (BSP_VAL_OBJECT_ARRAY_END == val->type)
        {
            // Endding
            del_value(val);
            break;
        }

        object_set_array(obj, -1, val);
    }

    return;
}

static void _traverse_json_hash(BSP_OBJECT *obj, BSP_STRING *str)
{
    if (!obj || OBJECT_TYPE_HASH != obj->type)
    {
        return;
    }

    BSP_VALUE *key = NULL;
    BSP_VALUE *val = NULL;
    BSP_STRING *key_str = NULL;
    // Key : Value
    // We ignore [:] here, we accept any decollator here -_-
    while (1)
    {
        key = _get_value_from_json(str);
        if (!key)
        {
            // Unwilling break
            break;
        }

        if (BSP_VAL_OBJECT_HASH_END == key->type)
        {
            // Succeed
            del_value(key);
            break;
        }

        val = _get_value_from_json(str);
        if (!val)
        {
            // No value
            del_value(key);
            break;
        }

        if (BSP_VAL_OBJECT_HASH_END == val->type)
        {
            // Endding
            del_value(key);
            del_value(val);
            break;
        }

        if (BSP_VAL_STRING == key->type)
        {
            // K/V pair
            key_str = (BSP_STRING *) key->rval;
            object_set_hash(obj, key_str, val);
        }
        else
        {
            // Unavailable key
            del_value(key);
            del_value(val);
        }
    }

    return;
}

BSP_OBJECT * json_nd_decode(BSP_STRING *str)
{
    BSP_OBJECT *ret = NULL;
    if (str)
    {
        str->cursor = 0;
        bsp_spin_lock(&str->lock);
        // Try first value
        BSP_VALUE *first = _get_value_from_json(str);
        if (first)
        {
            if (BSP_VAL_OBJECT == first->type)
            {
                ret = value_get_object(first);
                switch (ret->type)
                {
                    case OBJECT_TYPE_ARRAY : 
                        // Json array : [n1, n2, n3, n4 ...]
                        _traverse_json_array(ret, str);
                        break;
                    case OBJECT_TYPE_HASH : 
                        // Json hash : {key1:n1, key2:n2, key3:n3 ...}
                        _traverse_json_hash(ret, str);
                        break;
                    case OBJECT_TYPE_SINGLE : 
                    default : 
                        // You kidding me?
                        // Ni Ta Ma Zai Dou Wo?
                        break;
                }
                // No leak -_-
                first->rval = NULL;
                del_value(first);
            }
            else
            {
                // Single value
                ret = new_object(OBJECT_TYPE_SINGLE);
                object_set_single(ret, first);
            }
        }
        bsp_spin_unlock(&str->lock);
    }

    return ret;
}
