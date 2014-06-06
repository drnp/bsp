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
#define ARRAY_SIZE_INITIAL                      64
#define SERIALIZE_OBJECT                        0x0
#define SERIALIZE_ARRAY                         0x1

/* Macros */

/* Structs */
struct bsp_object_item_val_t
{
    char                lval[8];
    void                *rval;
    size_t              rval_len;
    char                type;
};

typedef struct bsp_object_item_t
{
    char                *key;
    size_t              key_len;
    int64_t             key_int;
    struct bsp_object_item_t
                        *prev;
    struct bsp_object_item_t
                        *next;
    struct bsp_object_item_t
                        *lprev;
    struct bsp_object_item_t
                        *lnext;
    struct bsp_object_item_val_t
                        value;
} BSP_OBJECT_ITEM;

typedef struct bsp_object_t
{
    size_t              nitems;
    size_t              hash_size;
    BSP_OBJECT_ITEM     *hash_table[2];
    BSP_OBJECT_ITEM     *head;
    BSP_OBJECT_ITEM     *tail;
    BSP_OBJECT_ITEM     *curr;
    int                 curr_table;
    BSP_SPINLOCK        obj_lock;
} BSP_OBJECT;

typedef struct bsp_val_t
{
    int64_t             v_int;
    double              v_float;
    char                *v_str;
    size_t              v_str_len;
    BSP_OBJECT          *v_obj;
    BSP_OBJECT_ITEM     **v_arr;
    size_t              v_arr_size;
    char                type;
} BSP_VAL;

/* Functions */
BSP_OBJECT_ITEM * new_object_item(const char *key, ssize_t key_len);
void free_object_item(BSP_OBJECT_ITEM *item);
void del_object_item(BSP_OBJECT_ITEM *item);

BSP_OBJECT * new_object(void);
void free_object(BSP_OBJECT *obj);
void del_object(BSP_OBJECT *obj);
int sort_object(BSP_OBJECT *obj);
BSP_OBJECT_ITEM * curr_item(BSP_OBJECT *obj);
void next_item(BSP_OBJECT *obj);
void prev_item(BSP_OBJECT *obj);
void reset_object(BSP_OBJECT *obj);

int array_set_item(BSP_OBJECT_ITEM *array, BSP_OBJECT_ITEM *item, size_t idx);
BSP_OBJECT_ITEM * array_get_item(BSP_OBJECT_ITEM *array, size_t idx);
int object_insert_item(BSP_OBJECT *obj, BSP_OBJECT_ITEM *item);
BSP_OBJECT_ITEM * object_remove_item(BSP_OBJECT *obj, BSP_OBJECT_ITEM *item);
BSP_OBJECT_ITEM * object_get_item(BSP_OBJECT *obj, const char *key, ssize_t key_len);
BSP_VAL * object_item_value(BSP_OBJECT *obj, BSP_VAL *val, const char *key, ssize_t key_len);

// Set values
void set_item_int8(BSP_OBJECT_ITEM *item, const int8_t value);
void set_item_int16(BSP_OBJECT_ITEM *item, const int16_t value);
void set_item_int32(BSP_OBJECT_ITEM *item, const int32_t value);
void set_item_int64(BSP_OBJECT_ITEM *item, const int64_t value);
void set_item_boolean(BSP_OBJECT_ITEM *item, const int value);
void set_item_float(BSP_OBJECT_ITEM *item, const float value);
void set_item_double(BSP_OBJECT_ITEM *item, const double value);
void set_item_string(BSP_OBJECT_ITEM *item, const char *value, ssize_t len);
void set_item_pointer(BSP_OBJECT_ITEM *item, const void *p);
void set_item_array(BSP_OBJECT_ITEM *item);
void set_item_object(BSP_OBJECT_ITEM *item, BSP_OBJECT *obj);
void set_item_null(BSP_OBJECT_ITEM *item);

// Serialize && unserialize
int object_serialize(BSP_OBJECT *obj, BSP_STRING *output, int length_mark);
size_t object_unserialize(const char *, size_t len, BSP_OBJECT *obj, int length_mark);
void object_to_lua(BSP_OBJECT *obj, lua_State *s);
void object_from_lua(BSP_OBJECT *obj, lua_State *s, int idx);
//void debug_object(BSP_OBJECT *obj);

#endif  /* _LIB_BSP_CORE_OBJECT_H */
