/*
 * bsp_object.h
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
 * Universal hash array object header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/26/2012
 * @changelog 
 *      [06/11/2012] - Creation
 *      [10/26/2012] - Boolean data type added
 */

#ifndef _LIB_BSP_CORE_OBJECT_H

#define _LIB_BSP_CORE_OBJECT_H
/* Headers */

/* Definations */
#define HASH_SIZE_INITIAL                       8
#define ARRAY_BUCKET_SIZE                       64
#define SERIALIZE_OBJECT                        0x0
#define SERIALIZE_ARRAY                         0x1

#define OBJECT_TYPE_SINGLE                      0x0
#define OBJECT_TYPE_ARRAY                       0x1
#define OBJECT_TYPE_HASH                        0x2
#define OBJECT_TYPE_UNDETERMINED                0xF

#define NO_HASH_KEY                             "_NO_HASH_KEY_"

/* Macros */

/* Structs */
typedef struct bsp_item_val_t
{
    char                lval[16];
    void                *rval;
    char                type;
} BSP_VALUE;

struct bsp_hash_item_t
{
    BSP_STRING          *key;
    BSP_VALUE           *value;
    struct bsp_hash_item_t
                        *prev;
    struct bsp_hash_item_t
                        *next;
    struct bsp_hash_item_t
                        *lprev;
    struct bsp_hash_item_t
                        *lnext;
};

struct bsp_hash_t
{
    size_t              nitems;
    size_t              hash_size;
    struct bsp_hash_item_t
                        *hash_table;
    struct bsp_hash_item_t
                        *head;
    struct bsp_hash_item_t
                        *tail;
    struct bsp_hash_item_t
                        *curr;
};

struct bsp_array_t
{
    size_t              nitems;
    size_t              nbuckets;
    size_t              curr;
    BSP_VALUE           ***items;
};

typedef struct bsp_item_val_t bsp_array_item_t;

typedef struct bsp_object_t
{
    void                *node;
    BSP_SPINLOCK        lock;
    char                type;
} BSP_OBJECT;

/* Functions */
BSP_OBJECT * new_object(char type);
void del_object(BSP_OBJECT *obj);
BSP_VALUE * new_value();
void del_value(BSP_VALUE *val);
void value_set_int(BSP_VALUE *val, const int64_t value);
void value_set_int29(BSP_VALUE *val, const int32_t value);
void value_set_boolean_true(BSP_VALUE *val);
void value_set_boolean_false(BSP_VALUE *val);
void value_set_float(BSP_VALUE *val, const float value);
void value_set_double(BSP_VALUE *val, const double value);
void value_set_pointer(BSP_VALUE *val, const void *value);
void value_set_string(BSP_VALUE *val, BSP_STRING *str);
void value_set_object(BSP_VALUE *val, BSP_OBJECT *obj);
void value_set_null(BSP_VALUE *val);
int64_t value_get_int(BSP_VALUE *val);
int value_get_boolean(BSP_VALUE *val);
float value_get_float(BSP_VALUE *val);
double value_get_double(BSP_VALUE *val);
void * value_get_pointer(BSP_VALUE *val);
BSP_STRING * value_get_string(BSP_VALUE *val);
BSP_OBJECT * value_get_object(BSP_VALUE *val);

void object_set_single(BSP_OBJECT *obj, BSP_VALUE *val);
void object_set_array(BSP_OBJECT *obj, ssize_t idx, BSP_VALUE *val);
void object_set_hash(BSP_OBJECT *obj, BSP_STRING *key, BSP_VALUE *val);
void object_set_hash_str(BSP_OBJECT *obj, const char *key, BSP_VALUE *val);

BSP_VALUE * object_get_single(BSP_OBJECT *obj);
BSP_VALUE * object_get_array(BSP_OBJECT *obj, size_t idx);
BSP_VALUE * object_get_hash(BSP_OBJECT *obj, BSP_STRING *key);
BSP_VALUE * object_get_hash_str(BSP_OBJECT *obj, const char *key);

BSP_VALUE * object_get_value(BSP_OBJECT *obj, const char *path);

BSP_VALUE * curr_item(BSP_OBJECT *obj);
size_t curr_array_index(BSP_OBJECT *obj);
BSP_STRING * curr_hash_key(BSP_OBJECT *obj);
size_t object_size(BSP_OBJECT *obj);
void reset_object(BSP_OBJECT *obj);
void next_item(BSP_OBJECT *obj);
void prev_item(BSP_OBJECT *obj);

BSP_STRING * object_serialize(BSP_OBJECT *obj);
BSP_OBJECT * object_unserialize(BSP_STRING *str);

void object_to_lua_stack(lua_State *s, BSP_OBJECT *obj);
BSP_OBJECT * lua_stack_to_object(lua_State *s);
#endif  /* _LIB_BSP_CORE_OBJECT_H */
