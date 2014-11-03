/*
 * bson.c
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
 * Native BSON encoder / decoder
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/31/2014
 * @changelog 
 *      [10/31/2014] - Creation
 */

#include "bsp.h"

/* === SERIALIZE === */
static void _append_value_to_bson(BSP_STRING *str, BSP_VALUE *val);
static void _append_object_to_bson(BSP_STRING *str, BSP_OBJECT *obj);

static void _append_value_to_bson(BSP_STRING *str, BSP_VALUE *val)
{
    if (!str || !val || str->is_const)
    {
        return;
    }

    int vlen = 0;
    BSP_STRING *src = NULL;
    BSP_OBJECT *sub_obj = NULL;
    char vbuf[9];
    int64_t vint64;
    double vdouble;
    switch (val->type)
    {
        case BSP_VAL_NULL : 
            vbuf[0] = BSON_ELEM_NULL;
            string_append(str, vbuf, 1);
            break;
        case BSP_VAL_INT : 
            vint64 = get_vint(val->lval, &vlen);
            if (vint64 >> 32)
            {
                // Int64
                vbuf[0] = BSON_ELEM_INT64;
                set_int64_le(vint64, vbuf + 1);
                string_append(str, vbuf, 9);
            }
            else
            {
                // Int32
                vbuf[0] = BSON_ELEM_INT32;
                set_int32_le(vint64, vbuf + 1);
                string_append(str, vbuf, 5);
            }
            break;
        case BSP_VAL_FLOAT : 
            vdouble = (double) get_float(val->lval);
            vbuf[0] = BSON_ELEM_DOUBLE;
            set_double(vdouble, vbuf + 1);
            string_append(str, vbuf, 9);
            break;
        case BSP_VAL_DOUBLE : 
            vbuf[0] = BSON_ELEM_DOUBLE;
            memcpy(vbuf + 1, val->lval, sizeof(double));
            string_append(str, vbuf, 9);
            break;
        case BSP_VAL_BOOLEAN_TRUE : 
            vbuf[0] = BSON_ELEM_BOOLEAN;
            vbuf[1] = BSON_ELEM_BOOLEAN_TRUE;
            string_append(str, vbuf, 2);
            break;
        case BSP_VAL_BOOLEAN_FALSE : 
            vbuf[0] = BSON_ELEM_BOOLEAN;
            vbuf[1] = BSON_ELEM_BOOLEAN_FALSE;
            string_append(str, vbuf, 2);
            break;
        case BSP_VAL_STRING : 
            src = (BSP_STRING *) val->rval;
            vbuf[0] = BSON_ELEM_STRING;
            set_int32_le((int32_t) STR_LEN(src) + 1, vbuf + 1);
            string_append(str, vbuf, 5);
            string_append(str, STR_STR(src), STR_LEN(src));
            string_append(str, "\0", 1);
            break;
        case BSP_VAL_OBJECT : 
            sub_obj = (BSP_OBJECT *) val->rval;
            if (OBJECT_TYPE_ARRAY == sub_obj->type)
            {
                vbuf[0] = BSON_ELEM_ARRAY;
            }
            else
            {
                vbuf[0] = BSON_ELEM_DOCUMENT;
            }
            string_append(str, vbuf, 1);
            _append_object_to_bson(str, sub_obj);
            break;
        default : 
            break;
    }

    return;
}

static void _append_object_to_bson(BSP_STRING *str, BSP_OBJECT *obj)
{
    BSP_STRING *body = new_string(NULL, 0);
    if (!str || !obj || str->is_const || !body)
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
            _append_value_to_bson(body, val);
            break;
        case OBJECT_TYPE_ARRAY : 
            // Array
            array = (struct bsp_array_t *) obj->node;
            size_t idx;
            if (array)
            {
                //string_append(str, "[", 1);
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
                    // Write key
                    string_printf(body, "%d", idx);
                    string_append(body, "\0", 1);
                    // Write value
                    _append_value_to_bson(str, val);
                }
            }
            break;
        case OBJECT_TYPE_HASH : 
            // Hash
            hash = (struct bsp_hash_t *) obj->node;
            BSP_STRING *key;
            if (hash)
            {
                reset_object(obj);
                val = curr_item(obj);
                while (val)
                {
                    key = curr_hash_key(obj);
                    if (key)
                    {
                        // Write key
                        string_append(body, STR_STR(key), STR_LEN(key));
                        string_append(body, "\0", 1);
                        _append_value_to_bson(body, val);
                    }
                    next_item(obj);
                    val = curr_item(obj);
                }
            }
            break;
        case OBJECT_TYPE_UNDETERMINED : 
        default : 
            break;
    }
    string_append(body, "\0", 1);
    char hdr[4];
    set_int32_le((int32_t) STR_LEN(body), hdr);
    string_append(str, hdr, 4);
    string_append(str, STR_STR(body), STR_LEN(body));
    del_string(body);

    return;
}

BSP_STRING * bson_nd_encode(BSP_OBJECT *obj)
{
    BSP_STRING *bson = NULL;
    if (obj)
    {
        bson = new_string(NULL, 0);
        _append_object_to_bson(bson, obj);
    }
    return bson;
}

/* === UNSERIALIZE == */
void _traverse_bson_array(BSP_OBJECT *obj, BSP_STRING *str);
void _traverse_bson_hash(BSP_OBJECT *obj, BSP_STRING *str);

BSP_VALUE * _get_value_from_bson(BSP_STRING *str)
{
    if (!str || !STR_REMAIN(str))
    {
        return NULL;
    }

    BSP_VALUE *ret = NULL;
    int32_t v_int32 = 0;
    int64_t v_int64 = 0;
    double v_double = 0.0f;
    size_t reg_len = 0;
    BSP_STRING *v_string = NULL;
    BSP_OBJECT *v_obj = NULL;

    unsigned char type = STR_CHAR(str);
    unsigned char sub_type = 0;
    STR_NEXT(str);
    switch (type)
    {
        case BSON_ELEM_ARRAY : 
            // Array document
            if (STR_REMAIN(str) >= 5)
            {
                v_int32 = get_int32_le(STR_CURR(str));
                if (STR_REMAIN(str) >= (4 + v_int32))
                {
                    v_string = new_string_const(STR_CURR(str) + 4, v_int32);
                    v_obj = new_object(OBJECT_TYPE_ARRAY);
                    _traverse_bson_array(v_obj, v_string);
                    ret = new_value();
                    value_set_object(ret, v_obj);
                    del_string(v_string);
                    str->cursor += (4 + v_int32);
                }
            }
            break;
        case BSON_ELEM_DOCUMENT : 
            // Normal document
            if (STR_REMAIN(str) >= 5)
            {
                v_int32 = get_int32_le(STR_CURR(str));
                if (STR_REMAIN(str) >= (4 + v_int32))
                {
                    v_string = new_string_const(STR_CURR(str) + 4, v_int32);
                    v_obj = new_object(OBJECT_TYPE_HASH);
                    _traverse_bson_hash(v_obj, str);
                    ret = new_value();
                    value_set_object(ret, v_obj);
                    del_string(v_string);
                    str->cursor += (4 + v_int32);
                }
            }
            break;
        case BSON_ELEM_NULL : 
            // Null
            ret = new_value();
            value_set_null(ret);
            break;
        case BSON_ELEM_BINARY : 
            // Binary
            if (STR_REMAIN(str) >= 1)
            {
                sub_type = STR_CHAR(str);
                STR_NEXT(str);
                switch (sub_type)
                {
                    case BSON_ELEM_BINARY_GENERIC : 
                    case BSON_ELEM_BINARY_BINARY : 
                        // Fixed length
                        if (STR_REMAIN(str) >= 4)
                        {
                            // Binary length
                            v_int32 = get_int32_le(STR_CURR(str));
                            if (STR_REMAIN(str) >= (4 + v_int32))
                            {
                                v_string = new_string(STR_CURR(str), v_int32);
                                ret = new_value();
                                value_set_string(ret, v_string);
                                str->cursor += (4 + v_int32);
                            }
                        }
                        break;
                    case BSON_ELEM_BINARY_UUID : 
                    case BSON_ELEM_BINARY_UUID_OLD : 
                    case BSON_ELEM_BINARY_MD5 : 
                        // 16 bytes
                        if (STR_REMAIN(str) >= 16)
                        {
                            v_string = new_string(STR_CURR(str), 16);
                            ret = new_value();
                            value_set_string(ret, v_string);
                            str->cursor += 16;
                        }
                        break;
                    case BSON_ELEM_BINARY_FUNCTION : 
                    default : 
                        break;
                }
            }
            break;
        case BSON_ELEM_INT32 : 
            // Int32
            if (STR_REMAIN(str) >= 4)
            {
                v_int32 = get_int32_le(STR_CURR(str));
                ret = new_value();
                value_set_int(ret, (const int64_t) v_int32);
                str->cursor += 4;
            }
            break;
        case BSON_ELEM_INT64 : 
        case BSON_ELEM_TIMESTAMP : 
        case BSON_ELEM_UTC_DATETIME : 
            // Int64
            if (STR_REMAIN(str) >= 8)
            {
                v_int64 = get_int64_le(STR_CURR(str));
                ret = new_value();
                value_set_int(ret, (const int64_t) v_int64);
                str->cursor += 8;
            }
            break;
        case BSON_ELEM_STRING : 
        case BSON_ELEM_JS_CODE : 
        case BSON_ELEM_SYMBOL : 
            // String
            if (STR_REMAIN(str) >= 4)
            {
                // Get length
                v_int32 = get_int32_le(STR_CURR(str));
                if (STR_REMAIN(str) >= (4 + v_int32))
                {
                    v_string = new_string(STR_CURR(str) + 4, v_int32);
                    ret = new_value();
                    value_set_string(ret, v_string);
                    str->cursor += (4 + v_int32);
                }
            }
            break;
        case BSON_ELEM_BOOLEAN : 
            // Boolean
            if (STR_REMAIN(str) >= 1)
            {
                sub_type = STR_CHAR(str);
                ret = new_value();
                if (BSON_ELEM_BOOLEAN_TRUE == sub_type)
                {
                    value_set_boolean_true(ret);
                }
                else
                {
                    value_set_boolean_false(ret);
                }
                STR_NEXT(str);
            }
            break;
        case BSON_ELEM_DOUBLE : 
            // Double, there's no float int BSON
            if (STR_REMAIN(str) >= 8)
            {
                v_double = get_double(STR_CURR(str));
                ret = new_value();
                value_set_double(ret, (const double) v_double);
                str->cursor += 8;
            }
            break;
        case BSON_ELEM_REGEXP : 
            // Zero-terminate string
            reg_len = strnlen(STR_CURR(str), STR_REMAIN(str));
            if (reg_len < STR_REMAIN(str))
            {
                v_string = new_string(STR_CURR(str), reg_len);
                ret = new_value();
                value_set_string(ret, v_string);
                str->cursor += (1 + reg_len);
            }
            break;
        case BSON_ELEM_NONE : 
        case BSON_ELEM_UNDEFINED : 
        case BSON_ELEM_DBPOINTER : 
        case BSON_ELEM_JS_CODE_WS : 
        default : 
            // We do not like...
            break;
    }

    return ret;
}

BSP_STRING * _get_str_from_bson(BSP_STRING *str)
{
    if (!str || !STR_REMAIN(str))
    {
        return NULL;
    }

    size_t len = strnlen(STR_CURR(str), STR_REMAIN(str));
    BSP_STRING *ret = NULL;
    if (len < STR_REMAIN(str))
    {
        // New string
        ret = new_string(STR_CURR(str), len);
        str->cursor += (1 + len);
    }

    return ret;
}

void _traverse_bson_array(BSP_OBJECT *obj, BSP_STRING *str)
{
    if (!obj || OBJECT_TYPE_ARRAY != obj->type || !str)
    {
        return;
    }

    BSP_STRING *key = NULL;
    BSP_VALUE *val = NULL;
    while (STR_REMAIN(str))
    {
        key = _get_str_from_bson(str);
        val = _get_value_from_bson(str);
        if (key && val)
        {
            object_set_array(obj, -1, val);
        }
    }
}

void _traverse_bson_hash(BSP_OBJECT *obj, BSP_STRING *str)
{
    if (!obj || OBJECT_TYPE_HASH != obj->type || !str)
    {
        return;
    }

    BSP_STRING *key = NULL;
    BSP_VALUE *val = NULL;
    while (STR_REMAIN(str))
    {
        key = _get_str_from_bson(str);
        val = _get_value_from_bson(str);
        if (key && val)
        {
            object_set_hash(obj, key, val);
        }
    }
}

BSP_OBJECT * bson_nd_decode(BSP_STRING *str)
{
    BSP_OBJECT *obj = NULL;
    if (str)
    {
        bsp_spin_lock(&str->lock);
        // If valid ?
        if (STR_LEN(str) >= 5)
        {
            int32_t body_len = get_int32_le(STR_STR(str));
            if (STR_LEN(str) >= (4 + body_len))
            {
                // Valid stream
                BSP_STRING *body = new_string_const(STR_STR(str) + 4, body_len);
                obj = new_object(OBJECT_TYPE_HASH);
                _traverse_bson_hash(obj, body);
                del_string(body);
            }
        }
        bsp_spin_unlock(&str->lock);
    }
    return obj;
}
