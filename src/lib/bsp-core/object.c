/*
 * object.c
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
 * Universal hash array object
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/21/2013
 * @changelog 
 *      [06/11/2012] - Creation
 *      [08/14/2012] - Float / Double byte order
 *      [09/28/2012] - Array support
 *      [05/21/2013] - Remove float / double byte-order reverse
 *      [12/17/2013] - Lightuserdata supported
 *      [08/05/2014] - Rebuild
 */

#include "bsp.h"

/* Set values */
/*
void set_item_int8(BSP_VALUE *val, const int8_t value)
{
    if (val)
    {
        set_int8(value, val->lval);
        val->type = BSP_VAL_INT8;
    }

    return;
}

void set_item_int16(BSP_VALUE *val, const int16_t value)
{
    if (val)
    {
        set_int16(value, val->lval);
        val->type = BSP_VAL_INT16;
    }

    return;
}

void set_item_int32(BSP_VALUE *val, const int32_t value)
{
    if (val)
    {
        set_int32(value, val->lval);
        val->type = BSP_VAL_INT32;
    }

    return;
}

void set_item_int64(BSP_VALUE *val, const int64_t value)
{
    if (val)
    {
        set_int64(value, val->lval);
        item->value.type = BSP_VAL_INT64;
    }

    return;
}
*/
// Set value to value
void value_set_int(BSP_VALUE *val, const int64_t value)
{
    if (val)
    {
        set_vint(value, val->lval);
        val->type = BSP_VAL_INT;
    }
}

void value_set_float(BSP_VALUE *val, const float value)
{
    if (val)
    {
        set_float(value, val->lval);
        val->type = BSP_VAL_FLOAT;
    }

    return;
}

void value_set_double(BSP_VALUE *val, const double value)
{
    if (val)
    {
        set_double(value, val->lval);
        val->type = BSP_VAL_DOUBLE;
    }

    return;
}

void value_set_boolean_true(BSP_VALUE *val)
{
    if (val)
    {
        set_vint(1, val->lval);
        val->type = BSP_VAL_BOOLEAN_TRUE;
    }

    return;
}

void value_set_boolean_false(BSP_VALUE *val)
{
    if (val)
    {
        set_vint(0, val->lval);
        val->type = BSP_VAL_BOOLEAN_FALSE;
    }

    return;
}

void value_set_pointer(BSP_VALUE *val, const void *p)
{
    if (val)
    {
        val->rval = (void *) p;
        val->type = BSP_VAL_POINTER;
    }

    return;
}

void value_set_string(BSP_VALUE *val, BSP_STRING *str)
{
    if (val && str)
    {
        val->rval = (void *) str;
        val->type = BSP_VAL_STRING;
    }

    return;
}

void value_set_object(BSP_VALUE *val, BSP_OBJECT *obj)
{
    if (val && obj)
    {
        val->rval = (void *) obj;
        val->type = BSP_VAL_OBJECT;
    }

    return;
}

void value_set_null(BSP_VALUE *val)
{
    if (val)
    {
        val->type = BSP_VAL_NULL;
    }

    return;
}

// Get value from value
int64_t value_get_int(BSP_VALUE *val)
{
    int64_t ret = 0;
    if (val)
    {
        if (BSP_VAL_INT == val->type)
        {
            // int64
            ret = get_vint(val->lval, NULL);
        }
        else if (BSP_VAL_INT29 == val->type)
        {
            // int29
            ret = get_vint29(val->lval, NULL);
        }
    }

    return ret;
}

int value_get_boolean(BSP_VALUE *val)
{
    int ret = BSP_BOOLEAN_FALSE;
    if (val && BSP_VAL_BOOLEAN_TRUE == val->type)
    {
        ret = BSP_BOOLEAN_TRUE;
    }

    return ret;
}

float value_get_float(BSP_VALUE *val)
{
    float ret = 0.0;
    if (val && BSP_VAL_FLOAT == val->type)
    {
        ret = get_float(val->rval);
    }

    return ret;
}

double value_get_double(BSP_VALUE *val)
{
    double ret = 0.0;
    if (val && BSP_VAL_DOUBLE == val->type)
    {
        ret = get_double(val->rval);
    }

    return ret;
}

void * value_get_pointer(BSP_VALUE *val)
{
    void *ret = NULL;
    if (val && BSP_VAL_POINTER == val->type)
    {
        ret = val->rval;
    }

    return ret;
}

BSP_STRING * value_get_string(BSP_VALUE *val)
{
    BSP_STRING *ret = NULL;
    if (val && BSP_VAL_STRING == val->type)
    {
        ret = (BSP_STRING *) val->rval;
    }

    return ret;
}

BSP_OBJECT * value_get_object(BSP_VALUE *val)
{
    BSP_OBJECT *ret = NULL;
    if (val && BSP_VAL_OBJECT == val->type)
    {
        ret = (BSP_OBJECT *) val->rval;
    }

    return ret;
}

/* Object & Value */
BSP_OBJECT * new_object(char type)
{
    BSP_OBJECT *obj = bsp_calloc(1, sizeof(BSP_OBJECT));
    if (obj)
    {
        obj->type = type;
        bsp_spin_init(&obj->lock);
    }

    return obj;
}

void del_object(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }

    BSP_VALUE *val = NULL;
    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    struct bsp_hash_item_t *item = NULL, *old = NULL;
    size_t idx, bucket, seq;
    bsp_spin_lock(&obj->lock);
    switch (obj->type)
    {
        case OBJECT_TYPE_SINGLE : 
            val = (BSP_VALUE *) obj->node;
            del_value(val);
            break;
        case OBJECT_TYPE_ARRAY : 
            array = (struct bsp_array_t *) obj->node;
            if (array)
            {
                for (idx = 0; idx < array->nitems; idx ++)
                {
                    bucket = (idx / ARRAY_BUCKET_SIZE);
                    seq = idx % ARRAY_BUCKET_SIZE;
                    if (bucket < array->nbuckets && array->items[bucket])
                    {
                        val = array->items[bucket][seq];
                        del_value(val);
                    }
                }
                bsp_free(array);
            }
            break;
        case OBJECT_TYPE_HASH : 
            hash = (struct bsp_hash_t *) obj->node;
            if (hash)
            {
                item = hash->head;
                while (item)
                {
                    del_string(item->key);
                    del_value(item->value);
                    old = item;
                    item = old->lnext;
                    bsp_free(old);
                }
                bsp_free(hash);
            }
            break;
        case OBJECT_TYPE_UNDETERMINED : 
        default : 
            break;
    }
    bsp_spin_unlock(&obj->lock);
    bsp_free(obj);

    return;
}

BSP_VALUE * new_value()
{
    return (BSP_VALUE *) bsp_calloc(1, sizeof(BSP_VALUE));
}

void del_value(BSP_VALUE *val)
{
    if (val)
    {
        if (BSP_VAL_POINTER == val->type)
        {
            // Just ignore, we cannot determine what you are
        }
        else if (BSP_VAL_STRING == val->type)
        {
            del_string((BSP_STRING *) val->rval);
        }
        else if (BSP_VAL_OBJECT == val->type)
        {
            del_object((BSP_OBJECT *) val->rval);
        }
        else
        {
            // Local value, Just free
        }
    }

    return bsp_free(val);
}

/* Item cursor operates */
// TODO : Not thread-safe, but maybe not neccessery
BSP_VALUE * curr_item(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return NULL;
    }

    BSP_VALUE *curr = NULL;
    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    switch (obj->type)
    {
        case OBJECT_TYPE_SINGLE : 
            curr = (BSP_VALUE *) obj->node;
            break;
        case OBJECT_TYPE_ARRAY : 
            array = (struct bsp_array_t *) obj->node;
            if (array)
            {
                size_t bucket = (array->curr / ARRAY_BUCKET_SIZE);
                if (array->curr < array->nitems && bucket < array->nbuckets && array->items[bucket])
                {
                    curr = array->items[bucket][array->curr % ARRAY_BUCKET_SIZE];
                }
            }
            break;
        case OBJECT_TYPE_HASH : 
            hash = (struct bsp_hash_t *) obj->node;
            if (hash && hash->curr)
            {
                // Just value
                // You can get current key with curr_item_key()
                curr = hash->curr->value;
            }
            break;
        default : 
            curr = NULL;
            break;
    }

    return curr;
}

// Current array index
size_t curr_array_index(BSP_OBJECT *obj)
{
    size_t idx = 0;
    if (obj && OBJECT_TYPE_ARRAY == obj->type)
    {
        struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
        if (array)
        {
            idx = array->curr;
        }
    }

    return idx;
}
// Current hash key
BSP_STRING * curr_hash_key(BSP_OBJECT *obj)
{
    BSP_STRING *key = NULL;
    if (obj && OBJECT_TYPE_HASH == obj->type)
    {
        struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
        if (hash && hash->curr)
        {
            key = hash->curr->key;
        }
    }

    return key;
}

size_t object_size(BSP_OBJECT *obj)
{
    size_t ret = 0;
    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    if (obj)
    {
        switch (obj->type)
        {
            case OBJECT_TYPE_SINGLE : 
                if (obj->node)
                {
                    ret = 1;
                }
                break;
            case OBJECT_TYPE_ARRAY : 
                array = (struct bsp_array_t *) obj->node;
                if (array)
                {
                    ret = array->nitems;
                }
                break;
            case OBJECT_TYPE_HASH : 
                hash = (struct bsp_hash_t *) obj->node;
                if (hash)
                {
                    ret = hash->nitems;
                }
                break;
            default : 
                break;
        }
    }

    return ret;
}

void reset_object(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }

    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    switch (obj->type)
    {
        case OBJECT_TYPE_ARRAY : 
            array = (struct bsp_array_t *) obj->node;
            if (array)
            {
                array->curr = 0;
            }
            break;
        case OBJECT_TYPE_HASH : 
            hash = (struct bsp_hash_t *) obj->node;
            if (hash)
            {
                hash->curr = hash->head;
            }
            break;
        case OBJECT_TYPE_SINGLE : 
        default : 
            // Nothing to do
            break;
    }

    return;
}

void next_item(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }

    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    switch (obj->type)
    {
        case OBJECT_TYPE_ARRAY : 
            array = (struct bsp_array_t *) obj->node;
            if (array && array->curr < array->nitems)
            {
                array->curr ++;
            }
            break;
        case OBJECT_TYPE_HASH : 
            hash = (struct bsp_hash_t *) obj->node;
            if (hash && hash->curr)
            {
                hash->curr = hash->curr->lnext;
            }
            break;
        case OBJECT_TYPE_SINGLE : 
        default : 
            // Nothing to NEXT
            break;
    }

    return;
}

void prev_item(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }

    struct bsp_array_t *array = NULL;
    struct bsp_hash_t *hash = NULL;
    switch (obj->type)
    {
        case OBJECT_TYPE_ARRAY : 
            array = (struct bsp_array_t *) obj->node;
            if (array && array->curr > 0)
            {
                array->curr --;
            }
            break;
        case OBJECT_TYPE_HASH : 
            hash = (struct bsp_hash_t *) obj->node;
            if (hash && hash->curr)
            {
                hash->curr = hash->curr->lprev;
            }
            break;
        case OBJECT_TYPE_SINGLE : 
        default : 
            // Nothing to PREV
            break;
    }
}

/* Sort */
/*
inline static int __sort_compare(const char *v1, 
                                size_t v1_len, 
                                long long int v1_int, 
                                const char *v2, 
                                size_t v2_len, 
                                long long int v2_int)
{
    register int is_d1 = 1, is_d2 = 1;

    if (v1_int == 0)
    {
        if (0 != strcmp(v1, "0"))
        {
            is_d1 = 0;
        }
    }
    
    if (v2_int == 0)
    {
        if (0 != strcmp(v2, "0"))
        {
            is_d2 = 0;
        }
    }

    if (is_d1 && is_d2)
    {
        if (v1_int > v2_int)
        {
            return 1;
        }

        else
        {
            return -1;
        }
    }

    if (!is_d1 && !is_d2)
    {
        // Strings
        if (v1_len > v2_len)
        {
            return memcmp(v1, v2, v2_len);
        }

        else
        {
            return memcmp(v1, v2, v1_len);
        }
    }

    return (is_d1 > is_d2) ? -1 : 1;
}

inline static int _sort_compare(const char *v1, 
                                size_t v1_len, 
                                long long int v1_int, 
                                const char *v2, 
                                size_t v2_len, 
                                long long int v2_int)
{
    return (v1_len > v2_len) ? memcmp(v1, v2, v2_len) : memcmp(v1, v2, v1_len);
}

inline static void _sort_qsort(BSP_OBJECT_ITEM **list, size_t nitems)
{
    int64_t begin_stack[256];
    int64_t end_stack[256];
    register int64_t begin;
    register int64_t end;
    register int64_t middle;
    register int64_t pivot;
    register int64_t seg1;
    register int64_t seg2;
    register int loop, c1, c2, c3;
    register BSP_OBJECT_ITEM *tmp;

    begin_stack[0] = 0;
    end_stack[0] = nitems - 1;

    for (loop = 0; loop >= 0; loop --)
    {
        begin = begin_stack[loop];
        end = end_stack[loop];

        while (begin < end)
        {
            // Find pivot
            middle = (end - begin) >> 1;
            
            c1 = _sort_compare(
                               list[begin]->key, 
                               list[begin]->key_len, 
                               list[begin]->key_int, 
                               list[middle]->key, 
                               list[middle]->key_len, 
                               list[middle]->key_int);
            c2 = _sort_compare(
                               list[middle]->key, 
                               list[middle]->key_len, 
                               list[middle]->key_int, 
                               list[end]->key, 
                               list[end]->key_len, 
                               list[end]->key_int);
            c3 = _sort_compare(
                               list[end]->key, 
                               list[end]->key_len, 
                               list[end]->key_int, 
                               list[begin]->key, 
                               list[begin]->key_len, 
                               list[begin]->key_int);

            if (c1 >= 0)
            {
                pivot = (c2 >= 0) ? middle : ((c3 >= 0) ? begin : end);
            }

            else
            {
                pivot = (c2 < 0) ? middle : ((c3 < 0) ? begin : end);
            }
            
            if (pivot != begin)
            {
                // Swap pivot & begin
                tmp = list[begin]; list[begin] = list[pivot]; list[pivot] = tmp;
            }

            seg1 = begin + 1;
            seg2 = end;

            while (1)
            {
                for (
                     ; 
                     seg1 < seg2 && _sort_compare(
                                                  list[begin]->key, 
                                                  list[begin]->key_len, 
                                                  list[begin]->key_int, 
                                                  list[seg1]->key, 
                                                  list[seg1]->key_len, 
                                                  list[seg1]->key_int) > 0; 
                     seg1 ++);
                for (
                     ; 
                     seg2 >= seg1 && _sort_compare(
                                                   list[seg2]->key, 
                                                   list[seg2]->key_len, 
                                                   list[seg2]->key_int, 
                                                   list[begin]->key, 
                                                   list[begin]->key_len, 
                                                   list[begin]->key_int) > 0; 
                     seg2 --);

                if (seg1 >= seg2)
                {
                    break;
                }

                tmp = list[seg1]; list[seg1] = list[seg2]; list[seg2] = tmp;

                seg1 ++;
                seg2 --;
            }

            // Swap begin & seg2
            tmp = list[begin]; list[begin] = list[seg2]; list[seg2] = tmp;

            if ((seg2 - begin) <= (end - seg2))
            {
                if ((seg2 + 1) < end)
                {
                    begin_stack[loop] = seg2 + 1;
                    end_stack[loop ++] = end;
                }

                end = seg2 - 1;
            }

            else
            {
                if ((seg2 - 1) > begin)
                {
                    begin_stack[loop] = begin;
                    end_stack[loop ++] = seg2 - 1;
                }

                begin = seg2 + 1;
            }
        }
    }
    
    return;
}

// Quicksort
int sort_object(BSP_OBJECT *obj)
{
    if (!obj || 2 > obj->nitems )
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    // Make a copy of item link
    BSP_OBJECT_ITEM **list = bsp_calloc(obj->nitems, sizeof(BSP_OBJECT_ITEM *));
    if (!list)
    {
        return BSP_RTN_ERROR_MEMORY;
    }
    
    bsp_spin_lock(&obj->obj_lock);
    BSP_OBJECT_ITEM *curr = NULL;
    int i = 0;
    
    reset_object(obj);
    curr = curr_item(obj);
    while (curr)
    {
        if (i > obj->nitems - 1)
        {
            break;
        }
        
        list[i ++] = curr;
        next_item(obj);
        curr = curr_item(obj);
    }

    _sort_qsort(list, obj->nitems);

    // Relink item link
    for (i = 1; i < obj->nitems - 1; i ++)
    {
        list[i]->lnext = list[i + 1];
        list[i]->lprev = list[i - 1];
    }

    list[0]->lnext = list[1];
    list[0]->lprev = NULL;
    obj->head = list[0];

    list[obj->nitems - 1]->lnext = NULL;
    list[obj->nitems - 1]->lprev = list[obj->nitems - 2];
    obj->tail = list[obj->nitems - 1];
    bsp_spin_unlock(&obj->obj_lock);

    bsp_free(list);

    return BSP_RTN_SUCCESS;
}
*/
// Find item from hash
static inline struct bsp_hash_item_t * _find_hash(struct bsp_hash_t *hash, BSP_STRING *key)
{
    struct bsp_hash_item_t *ret = NULL;
    if (hash && key && hash->hash_table)
    {
        uint32_t hash_key = bsp_hash(STR_STR(key), STR_LEN(key));
        struct bsp_hash_item_t *bucket = &hash->hash_table[hash_key % hash->hash_size];
        struct bsp_hash_item_t *curr = bucket->next;
        while (curr)
        {
            if (STR_IS_EQUAL(key, curr->key))
            {
                ret = curr;
                break;
            }
            curr = curr->next;
        }
    }

    return ret;
}

static inline struct bsp_hash_item_t * _find_hash_str(struct bsp_hash_t *hash, const char *key)
{
    struct bsp_hash_item_t *ret = NULL;
    if (hash && key && hash->hash_table)
    {
        uint32_t hash_key = bsp_hash(key, -1);
        struct bsp_hash_item_t *bucket = &hash->hash_table[hash_key % hash->hash_size];
        struct bsp_hash_item_t *curr = bucket->next;
        while (curr)
        {
            if (0 == strncmp(key, STR_STR(curr->key), STR_LEN(curr->key)))
            {
                ret = curr;
                break;
            }
            curr = curr->next;
        }
    }

    return ret;
}

// Remove item from hash
static inline int _remove_hash(struct bsp_hash_t *hash, BSP_STRING *key)
{
    int ret = 0;
    struct bsp_hash_item_t *item = _find_hash(hash, key);
    if (item)
    {
        if (hash->head == item)
        {
            hash->head = item->lnext;
        }
        if (hash->tail == item)
        {
            hash->tail = item->lprev;
        }
        if (hash->curr == item)
        {
            hash->curr = item->lnext;
        }
        if (item->lnext)
        {
            item->lnext->lprev = item->lprev;
        }
        if (item->next)
        {
            item->next->prev = item->prev;
        }
        if (item->lprev)
        {
            item->lprev->lnext = item->lnext;
        }
        if (item->prev)
        {
            item->prev->next = item->next;
        }

        ret = 1;
        // Delete item
        del_value(item->value);
        del_string(item->key);
        bsp_free(item);
        item = NULL;
    }

    return ret;
}

static inline int _remove_hash_str(struct bsp_hash_t *hash, const char *key)
{
    int ret = 0;
    struct bsp_hash_item_t *item = _find_hash_str(hash, key);
    if (item)
    {
        if (hash->head == item)
        {
            hash->head = item->lnext;
        }
        if (hash->tail == item)
        {
            hash->tail = item->lprev;
        }
        if (hash->curr == item)
        {
            hash->curr = item->lnext;
        }
        if (item->lnext)
        {
            item->lnext->lprev = item->lprev;
        }
        if (item->next)
        {
            item->next->prev = item->prev;
        }
        if (item->lprev)
        {
            item->lprev->lnext = item->lnext;
        }
        if (item->prev)
        {
            item->prev->next = item->next;
        }

        ret = 1;
        // Delete item
        del_value(item->value);
        del_string(item->key);
        bsp_free(item);
        item = NULL;
    }

    return ret;
}

// Insert an item into hash
static int _insert_hash(struct bsp_hash_t *hash, BSP_STRING *key, BSP_VALUE *val)
{
    int ret = 0;
    if (hash && hash->hash_table && key && val)
    {
        struct bsp_hash_item_t *item = _find_hash(hash, key);
        if (item)
        {
            // Just overwrite value
            if (item->value)
            {
                del_value(item->value);
            }
            item->value = val;
        }
        else
        {
            item = bsp_calloc(1, sizeof(struct bsp_hash_item_t));
            item->key = key;
            item->value = val;
            // Insert into link
            if (!hash->head)
            {
                hash->head = item;
            }
            if (hash->tail)
            {
                hash->tail->lnext = item;
                item->lprev = hash->tail;
            }
            hash->tail = item;

            // Insert into hash table
            uint32_t hash_key = bsp_hash(STR_STR(key), STR_LEN(key));
            struct bsp_hash_item_t *bucket = &hash->hash_table[hash_key % hash->hash_size];
            item->next = bucket->next;
            item->prev = bucket;
            bucket->next = item;

            ret = 1;
        }
    }

    return ret;
}

static int _insert_hash_str(struct bsp_hash_t *hash, const char *key, BSP_VALUE *val)
{
    int ret = 0;
    if (hash && hash->hash_table && key && val)
    {
        struct bsp_hash_item_t *item = _find_hash_str(hash, key);
        if (item)
        {
            // Just overwrite value
            if (item->value)
            {
                del_value(item->value);
            }
            item->value = val;
        }
        else
        {
            item = bsp_calloc(1, sizeof(struct bsp_hash_item_t));
            item->key = new_string(key, -1);
            item->value = val;
            // Insert into link
            if (!hash->head)
            {
                hash->head = item;
            }
            if (hash->tail)
            {
                hash->tail->lnext = item;
                item->lprev = hash->tail;
            }
            hash->tail = item;

            // Insert into hash table
            uint32_t hash_key = bsp_hash(key, -1);
            struct bsp_hash_item_t *bucket = &hash->hash_table[hash_key % hash->hash_size];
            item->next = bucket->next;
            item->prev = bucket;
            bucket->next = item;

            ret = 1;
        }
    }

    return ret;
}

// Enlarge hash table, rebuild hash link
static void _rebuild_hash(struct bsp_hash_t *hash, size_t new_hash_size)
{
    if (hash && new_hash_size)
    {
        if (new_hash_size == hash->hash_size)
        {
            return;
        }

        if (hash->hash_table)
        {
            bsp_free(hash->hash_table);
        }
        hash->hash_table = bsp_calloc(new_hash_size, sizeof(struct bsp_hash_item_t));
        hash->hash_size = new_hash_size;

        // Insert every item from link
        struct bsp_hash_item_t *curr = hash->head;
        struct bsp_hash_item_t *bucket;
        uint32_t hash_key;
        while (curr && curr->key)
        {
            hash_key = bsp_hash(STR_STR(curr->key), STR_LEN(curr->key));
            bucket = &hash->hash_table[hash_key % hash->hash_size];
            curr->next = bucket->next;
            curr->prev = bucket;
            bucket->next = curr;
            curr = curr->lnext;
        }
    }

    return;
}

// Evalute a single (Number / String / Boolean etc) to a object
void object_set_single(BSP_OBJECT *obj, BSP_VALUE *val)
{
    if (obj && OBJECT_TYPE_SINGLE == obj->type)
    {
        bsp_spin_lock(&obj->lock);
        if (obj->node)
        {
            BSP_VALUE *old = (BSP_VALUE *) obj->node;
            del_value(old);
        }
        obj->node = (BSP_VALUE *) val;
        bsp_spin_unlock(&obj->lock);
    }

    return;
}

// Add an item into array
void object_set_array(BSP_OBJECT *obj, ssize_t idx, BSP_VALUE *val)
{
    if (obj && OBJECT_TYPE_ARRAY == obj->type)
    {
        bsp_spin_lock(&obj->lock);
        struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
        if (!array)
        {
            // Make a new array
            array = bsp_calloc(1, sizeof(struct bsp_array_t));
            obj->node = (void *) array;
        }

        if (idx < 0)
        {
            idx = array->nitems;
        }

        size_t bucket = (idx / ARRAY_BUCKET_SIZE);
        size_t seq = idx % ARRAY_BUCKET_SIZE;
        size_t nbuckets = array->nbuckets;
        if (bucket >= nbuckets)
        {
            // Enlarge buckets
            nbuckets = 2 << (int) (log2(bucket + 1));
            array->items = bsp_realloc(array->items, nbuckets * sizeof(struct BSP_VAL **));
            size_t i;
            for (i = array->nbuckets; i < nbuckets; i ++)
            {
                array->items[i] = NULL;
            }
            array->nbuckets = nbuckets;
        }

        if (!array->items[bucket])
        {
            // New bucket
            array->items[bucket] = bsp_calloc(ARRAY_BUCKET_SIZE, sizeof(struct BSP_VAL *));
        }

        if (array->items[bucket][seq])
        {
            // Remove old value
            del_value(array->items[bucket][seq]);
        }
        array->items[bucket][seq] = val;
        if (idx >= array->nitems)
        {
            array->nitems = idx + 1;
        }
        bsp_spin_unlock(&obj->lock);
    }

    return;
}

// Insert / Remove an item into / from a hash
void object_set_hash(BSP_OBJECT *obj, BSP_STRING *key, BSP_VALUE *val)
{
    if (obj && OBJECT_TYPE_HASH == obj->type && key)
    {
        bsp_spin_lock(&obj->lock);
        struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
        if (!hash)
        {
            // Make a new hash
            hash = bsp_calloc(1, sizeof(struct bsp_hash_t));
            hash->hash_size = HASH_SIZE_INITIAL;
            hash->hash_table = bsp_calloc(HASH_SIZE_INITIAL, sizeof(struct bsp_hash_item_t));
            obj->node = (void *) hash;
        }

        if (val)
        {
            // Insert
            hash->nitems += _insert_hash(hash, key, val);
            if (hash->nitems > 8 * hash->hash_size)
            {
                // Rehash
                _rebuild_hash(hash, hash->hash_size * 8);
            }
        }
        else
        {
            // Remove
            hash->nitems -= _remove_hash(hash, key);
        }
        bsp_spin_unlock(&obj->lock);
    }

    return;
}

void object_set_hash_str(BSP_OBJECT *obj, const char *key, BSP_VALUE *val)
{
    if (obj && OBJECT_TYPE_HASH == obj->type && key)
    {
        bsp_spin_lock(&obj->lock);
        struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
        if (!hash)
        {
            // Make a new hash
            hash = bsp_calloc(1, sizeof(struct bsp_hash_t));
            hash->hash_size = HASH_SIZE_INITIAL;
            hash->hash_table = bsp_calloc(HASH_SIZE_INITIAL, sizeof(struct bsp_hash_item_t));
            obj->node = (void *) hash;
        }

        if (val)
        {
            // Insert
            hash->nitems += _insert_hash_str(hash, key, val);
            if (hash->nitems > 8 * hash->hash_size)
            {
                // Rehash
                _rebuild_hash(hash, hash->hash_size * 8);
            }
        }
        else
        {
            // Remove
            hash->nitems -= _remove_hash_str(hash, key);
        }
        bsp_spin_unlock(&obj->lock);
    }

    return;
}

// Get single value from object
BSP_VALUE * object_get_single(BSP_OBJECT *obj)
{
    BSP_VALUE *ret = NULL;
    if (obj && OBJECT_TYPE_SINGLE == obj->type)
    {
        bsp_spin_lock(&obj->lock);
        ret = (BSP_VALUE *) obj->node;
        bsp_spin_unlock(&obj->lock);
    }

    return ret;
}

// Get value from array by given index
BSP_VALUE * object_get_array(BSP_OBJECT *obj, size_t idx)
{
    BSP_VALUE *ret = NULL;
    if (obj && OBJECT_TYPE_ARRAY == obj->type)
    {
        bsp_spin_lock(&obj->lock);
        struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
        size_t bucket = idx / ARRAY_BUCKET_SIZE;
        size_t seq = idx % ARRAY_BUCKET_SIZE;
        if (array && idx < array->nitems && bucket < array->nbuckets && array->items[bucket])
        {
            ret = array->items[bucket][seq];
        }
        bsp_spin_unlock(&obj->lock);
    }

    return ret;
}

// Get value from hash table
BSP_VALUE * object_get_hash(BSP_OBJECT *obj, BSP_STRING *key)
{
    BSP_VALUE *ret = NULL;
    if (obj && key && OBJECT_TYPE_HASH == obj->type)
    {
        bsp_spin_lock(&obj->lock);
        struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
        if (hash)
        {
            struct bsp_hash_item_t *item = _find_hash(hash, key);
            if (item)
            {
                ret = item->value;
            }
        }
        bsp_spin_unlock(&obj->lock);
    }

    return ret;
}

BSP_VALUE * object_get_hash_str(BSP_OBJECT *obj, const char *key)
{
    BSP_VALUE *ret = NULL;
    if (obj && key && OBJECT_TYPE_HASH == obj->type)
    {
        bsp_spin_lock(&obj->lock);
        struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
        if (hash)
        {
            struct bsp_hash_item_t *item = _find_hash_str(hash, key);
            if (item)
            {
                ret = item->value;
            }
        }
        bsp_spin_unlock(&obj->lock);
    }

    return ret;
}

/* Serializer & Unserializer */
static void _pack_key(BSP_STRING *str, BSP_STRING *key);
static void _pack_value(BSP_STRING *str, BSP_VALUE *val);
static void _pack_object(BSP_STRING *str, BSP_OBJECT *obj);

static void _pack_key(BSP_STRING *str, BSP_STRING *key)
{
    if (str)
    {
        char buf[9];
        if (key)
        {
            string_append(str, buf, set_vint(STR_LEN(key), buf));
            string_append(str, STR_STR(key), STR_LEN(key));
        }
        else
        {
            string_append(str, buf, set_vint(strlen(NO_HASH_KEY), buf));
            string_append(str, NO_HASH_KEY, -1);
        }
    }

    return;
}

static void _pack_value(BSP_STRING *str, BSP_VALUE *val)
{
    char tmp[9];
    int tmp_len;
    BSP_STRING *sub_str = NULL;
    BSP_OBJECT *sub_obj = NULL;
    if (val && str)
    {
        string_append(str, &val->type, 1);
        switch (val->type)
        {
            case BSP_VAL_INT : 
                string_append(str, val->lval, vintlen(val->lval, -1));
                break;
            case BSP_VAL_FLOAT : 
                string_append(str, val->lval, sizeof(float));
                break;
            case BSP_VAL_DOUBLE : 
                string_append(str, val->lval, sizeof(float));
                break;
            case BSP_VAL_STRING : 
                sub_str = (BSP_STRING *) val->rval;
                if (sub_str)
                {
                    tmp_len = set_vint(STR_LEN(sub_str), tmp);
                    string_append(str, tmp, tmp_len);
                    string_append(str, STR_STR(sub_str), STR_LEN(sub_str));
                }
                else
                {
                    tmp[0] = 0;
                    string_append(str, tmp, 1);
                }
                break;
            case BSP_VAL_POINTER : 
                set_pointer(val->rval, tmp);
                string_append(str, tmp, sizeof(void *));
                break;
            case BSP_VAL_OBJECT : 
                sub_obj = (BSP_OBJECT *) val->rval;
                if (sub_obj)
                {
                    _pack_object(str, sub_obj);
                }
                break;
            case BSP_VAL_BOOLEAN_TRUE : 
            case BSP_VAL_BOOLEAN_FALSE : 
            case BSP_VAL_NULL : 
            case BSP_VAL_UNKNOWN : 
            default : 
                // No value
                break;
        }
    }

    return;
}

static void _pack_object(BSP_STRING *str, BSP_OBJECT *obj)
{
    char value_type[1];
    BSP_VALUE *val = NULL;
    if (obj && str)
    {
        bsp_spin_lock(&obj->lock);
        switch (obj->type)
        {
            case OBJECT_TYPE_SINGLE : 
                // A single value
                value_type[0] = BSP_VAL_OBJECT_SINGLE;
                string_append(str, value_type, 1);
                // Push value
                val = (BSP_VALUE *) obj->node;
                _pack_value(str, val);
                value_type[0] = BSP_VAL_OBJECT_SINGLE_END;
                string_append(str, value_type, 1);
                break;
            case OBJECT_TYPE_ARRAY : 
                // Array
                value_type[0] = BSP_VAL_OBJECT_ARRAY;
                string_append(str, value_type, 1);
                // Every item
                struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
                size_t idx, bucket, seq;
                if (array)
                {
                    for (idx = 9; idx < array->nitems; idx ++)
                    {
                        bucket = idx / ARRAY_BUCKET_SIZE;
                        seq = idx % ARRAY_BUCKET_SIZE;
                        val = NULL;
                        if (bucket < array->nbuckets && array->items[bucket])
                        {
                            val = array->items[bucket][seq];
                            _pack_value(str, val);
                        }
                        else
                        {
                            // Empty
                            value_type[0] = BSP_VAL_NULL;
                            string_append(str, value_type, 1);
                        }
                    }
                }
                value_type[0] = BSP_VAL_OBJECT_ARRAY_END;
                string_append(str, value_type, 1);
                break;
            case OBJECT_TYPE_HASH : 
                // Dict
                value_type[0] = BSP_VAL_OBJECT_HASH;
                string_append(str, value_type, 1);
                // Traverse items
                BSP_STRING *key;
                reset_object(obj);
                val = curr_item(obj);
                while (val)
                {
                    key = curr_hash_key(obj);
                    // Set key and value
                    _pack_key(str, key);
                    _pack_value(str, val);
                    next_item(obj);
                    val = curr_item(obj);
                }
                value_type[0] = BSP_VAL_OBJECT_HASH_END;
                string_append(str, value_type, 1);
                break;
            case OBJECT_TYPE_UNDETERMINED : 
            default : 
                // Yaaaahhh~~~?
                break;
        }
        bsp_spin_unlock(&obj->lock);
    }

    return;
}

BSP_STRING * object_serialize(BSP_OBJECT *obj)
{
    BSP_STRING * ret = new_string(NULL, 0);
    if (obj && ret)
    {
        _pack_object(ret, obj);
    }

    return ret;
}

static BSP_STRING * _unpack_key(BSP_STRING *str);
static BSP_VALUE * _unpack_value(BSP_STRING *str);
static BSP_OBJECT * _unpack_object(BSP_STRING *str);

static BSP_STRING * _unpack_key(BSP_STRING *str)
{
    BSP_STRING *ret = NULL;
    char *data;
    if (str)
    {
        size_t left = STR_LEN(str) - str->cursor;
        size_t vlen = 0;
        if (left > 0)
        {
            data = STR_STR(str);
            vlen = vintlen(data, left);
            if (left >= vlen)
            {
                int nouse = 0;
                int64_t str_len = get_vint(data, &nouse);
                if (left >= vlen + str_len)
                {
                    ret = new_string(data + vlen, str_len);
                    vlen += str_len;
                }
            }
        }
        str->cursor += vlen;
    }

    return ret;
}

static BSP_VALUE * _unpack_value(BSP_STRING *str)
{
    BSP_VALUE *ret = NULL;
    char *data;
    char type;
    if (str)
    {
        size_t left = STR_LEN(str) - str->cursor;
        size_t vlen = 0;
        BSP_OBJECT *obj = NULL;
        if (left > 0)
        {
            // 1st byte : Type
            type = STR_STR(str)[str->cursor];
            data = STR_STR(str) + 1;
            ret = new_value();
            ret->type = type;
            switch (type)
            {
                case BSP_VAL_INT : 
                    vlen = vintlen(data, left - 1);
                    memcpy(ret->lval, data, vlen);
                    break;
                case BSP_VAL_FLOAT : 
                    vlen = (left > sizeof(float)) ? sizeof(float) : (left - 1);
                    memcpy(ret->lval, data, vlen);
                    break;
                case BSP_VAL_DOUBLE : 
                    vlen = (left > sizeof(double)) ? sizeof(double) : (left - 1);
                    memcpy(ret->lval, data, vlen);
                    break;
                case BSP_VAL_STRING : 
                    vlen = vintlen(data, left - 1);
                    if (left >= vlen)
                    {
                        int nouse = 0;
                        int64_t str_len = get_vint(data, &nouse);
                        if (left >= vlen + str_len)
                        {
                            ret->rval = (void *) new_string(data + vlen, str_len);
                            vlen += str_len;
                        }
                    }
                    break;
                case BSP_VAL_POINTER : 
                    vlen = (left > sizeof(void *)) ? sizeof(void *) : (left - 1);
                    memcpy(&ret->rval, data, vlen);
                    break;
                case BSP_VAL_OBJECT : 
                    obj = _unpack_object(str);
                    ret->rval = (void *) obj;
                    break;
                case BSP_VAL_BOOLEAN_TRUE : 
                case BSP_VAL_BOOLEAN_FALSE : 
                case BSP_VAL_NULL : 
                case BSP_VAL_UNKNOWN : 
                default : 
                    // Single values
                    break;
            }
            str->cursor += vlen + 1;
        }
    }

    return ret;
}

static BSP_OBJECT * _unpack_object(BSP_STRING *str)
{
    BSP_OBJECT *obj = NULL;
    if (str)
    {
        bsp_spin_lock(&str->lock);
        if (str->cursor < STR_LEN(str))
        {
            // Read first
            BSP_VALUE *val = NULL;
            BSP_STRING *key = NULL;
            char type = STR_STR(str)[str->cursor];
            obj = new_object(type);
            str->cursor ++;
            switch (type)
            {
                case BSP_VAL_OBJECT_SINGLE : 
                    val = _unpack_value(str);
                    object_set_single(obj, val);
                    // Pass endding
                    str->cursor ++;
                    break;
                case BSP_VAL_OBJECT_ARRAY : 
                    while (1)
                    {
                        val = _unpack_value(str);
                        if (!val)
                        {
                            break;
                        }
                        if (str->cursor < STR_LEN(str) && BSP_VAL_OBJECT_ARRAY_END == STR_STR(str)[str->cursor])
                        {
                            // Endding
                            str->cursor ++;
                            break;
                        }
                        object_set_array(obj, -1, val);
                    }
                    break;
                case BSP_VAL_OBJECT_HASH : 
                    while (1)
                    {
                        // Read key first
                        key = _unpack_key(str);
                        if (!key)
                        {
                            break;
                        }
                        val = _unpack_value(str);
                        if (!val)
                        {
                            break;
                        }
                        if (str->cursor < STR_LEN(str) && BSP_VAL_OBJECT_HASH_END == STR_STR(str)[str->cursor])
                        {
                            // Endding
                            str->cursor ++;
                            break;
                        }
                        object_set_hash(obj, key, val);
                    }
                    break;
                default : 
                    break;
            }
        }
        bsp_spin_unlock(&str->lock);
    }

    return obj;
}

BSP_OBJECT * object_unserialize(BSP_STRING *str)
{
    BSP_OBJECT * ret = NULL;
    if (str)
    {
        str->cursor = 0;
        ret = _unpack_object(str);
    }

    return ret;
}
/*
// Serialize an object into stream
static void _insert_item_to_string(BSP_OBJECT_ITEM *item, BSP_STRING *str, int mode, int length_mark)
{
    if (!item || !str)
    {
        return;
    }

    char len[8];
    char key_type[1] = {BSP_VAL_STRING};
    char end_tags[2] = {BSP_VAL_ARRAY_END, BSP_VAL_OBJECT_END};
    BSP_OBJECT *next_obj;
    
    // Write key first
    if (SERIALIZE_OBJECT == mode && item->key)
    {
        string_append(str, key_type, 1);
        if (LENGTH_TYPE_32B == length_mark)
        {
            set_int32((int32_t) item->key_len, len);
            string_append(str, len, 4);
        }

        else
        {
            set_int64((int64_t) item->key_len, len);
            string_append(str, len, 8);
        }
        
        string_append(str, item->key, item->key_len);
    }
    
    switch (item->value.type)
    {
        case BSP_VAL_INT8 : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 1);
            break;
        case BSP_VAL_INT16 : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 2);
            break;
        case BSP_VAL_INT32 : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 4);
            break;
        case BSP_VAL_INT64 : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 8);
            break;
        case BSP_VAL_BOOLEAN : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 1);
            break;
        case BSP_VAL_FLOAT : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 4);
            break;
        case BSP_VAL_DOUBLE : 
            string_append(str, &item->value.type, 1);
            string_append(str, item->value.lval, 8);
            break;
        case BSP_VAL_STRING : 
            string_append(str, &item->value.type, 1);
            if (LENGTH_TYPE_32B == length_mark)
            {
                set_int32((int32_t) item->value.rval_len, len);
                string_append(str, len, 4);
            }
            else
            {
                // Write string length, size_t recognized as uint64_t
                set_int64((int64_t) item->value.rval_len, len);
                string_append(str, len, 8);
            }
            string_append(str, item->value.rval, item->value.rval_len);
            break;
        case BSP_VAL_ARRAY : 
            // Loop for each item
            string_append(str, &item->value.type, 1);
            size_t i;
            BSP_OBJECT_ITEM **list = (BSP_OBJECT_ITEM **) item->value.rval;
            if (list)
            {
                for (i = 0; i < item->value.rval_len; i ++)
                {
                    // Append each line
                    if (list[i])
                    {
                        _insert_item_to_string(list[i], str, SERIALIZE_ARRAY, length_mark);
                    }
                }
            }
            string_append(str, end_tags, 1);
            break;
        case BSP_VAL_OBJECT : 
            string_append(str, &item->value.type, 1);
            next_obj = (BSP_OBJECT *) item->value.rval;
            // Next recursion
            if (next_obj)
            {
                BSP_OBJECT_ITEM *curr = NULL;
                reset_object(next_obj);
                while ((curr = curr_item(next_obj)))
                {
                    _insert_item_to_string(curr, str, SERIALIZE_OBJECT, length_mark);
                    next_item(next_obj);
                }
                string_append(str, end_tags + 1, 1);
            }
            break;
        case BSP_VAL_NULL : 
        default : 
            // Null item
            item->value.type = BSP_VAL_NULL;
            string_append(str, &item->value.type, 1);
            break;
    }
}

int object_serialize(BSP_OBJECT *obj, BSP_STRING *output, int length_mark)
{
    if (!obj || !output)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_OBJECT_ITEM *curr = NULL;
    bsp_spin_lock(&obj->obj_lock);
    reset_object(obj);
    while ((curr = curr_item(obj)))
    {
        _insert_item_to_string(curr, output, SERIALIZE_OBJECT, length_mark);
        next_item(obj);
    }
    bsp_spin_unlock(&obj->obj_lock);

    return BSP_RTN_SUCCESS;
}

// Unserialize stream into object
size_t _parse_object(const char *input, size_t len, BSP_OBJECT *obj, int length_mark);
size_t _parse_array(const char *input, size_t len, BSP_OBJECT_ITEM *array, int length_mark);
size_t _parse_item(const char *input, size_t len, BSP_OBJECT_ITEM *item, char val_type, int length_mark)
{
    if (!input || !item)
    {
        return 0;
    }
    
    size_t remaining = len;
    char *curr = (char *) input;
    int64_t str_len;
    
    switch (val_type)
    {
        case BSP_VAL_INT8 : 
            if (remaining >= 1)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 1);
                remaining -= 1;
                curr += 1;
            }
            break;
        case BSP_VAL_INT16 : 
            if (remaining >= 2)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 2);
                remaining -= 2;
                curr += 2;
            }
            break;
        case BSP_VAL_INT32 : 
            if (remaining >= 4)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 4);
                remaining -= 4;
                curr += 4;
            }
            break;
        case BSP_VAL_INT64 : 
            if (remaining >= 8)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 8);
                remaining -= 8;
                curr += 8;
            }
            break;
        case BSP_VAL_BOOLEAN : 
            if (remaining >= 2)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 1);
                remaining -= 1;
                curr += 1;
            }
            break;
        case BSP_VAL_FLOAT : 
            if (remaining >= 4)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 4);
                remaining -= 4;
                curr += 4;
            }
            break;
        case BSP_VAL_DOUBLE : 
            if (remaining >= 8)
            {
                item->value.type = val_type;
                memcpy(item->value.lval, curr, 8);
                remaining -= 8;
                curr += 8;
            }
            break;
        case BSP_VAL_STRING : 
            if (LENGTH_TYPE_32B == length_mark)
            {
                if (remaining >= 4)
                {
                    str_len = get_int32(curr);
                    remaining -= 4;
                    curr += 4;
                    if (remaining >= str_len)
                    {
                        set_item_string(item, curr, str_len);
                        remaining -= str_len;
                        curr += str_len;
                    }
                }
            }
            else
            {
                if (remaining >= 8)
                {
                    str_len = get_int64(curr);
                    remaining -= 8;
                    curr += 8;
                    if (remaining >= str_len)
                    {
                        set_item_string(item, curr, str_len);
                        remaining -= str_len;
                        curr += str_len;
                    }
                }
            }
            break;
        case BSP_VAL_NULL : 
            set_item_null(item);
            break;
        default : 
            break;
    }
    
    return len - remaining;
}

size_t _parse_object(const char *input, size_t len, BSP_OBJECT *obj, int length_mark)
{
    if (!input || !obj)
    {
        return 0;
    }

    size_t remaining = len;
    size_t pn;
    char val_type;
    char *curr = (char *) input;
    char *key = NULL;
    int64_t key_len = 0;
    
    BSP_OBJECT *next_obj = NULL;
    BSP_OBJECT_ITEM *item = NULL;
    
    while (remaining > 0)
    {
        key = NULL;
        
        // Key
        val_type = *curr;
        remaining -= 1;
        curr += 1;
        
        if (val_type == BSP_VAL_OBJECT_END)
        {
            // Object ending
            return len - remaining;
        }
        
        if (val_type != BSP_VAL_STRING || remaining < 8)
        {
            // Stream broken
            break;
        }
        
        if (LENGTH_TYPE_32B == length_mark)
        {
            key_len = get_int32(curr);
            remaining -= 4;
            curr += 4;
        }
        
        else
        {
            key_len = get_int64(curr);
            remaining -= 8;
            curr += 8;
        }
        
        if (remaining < key_len)
        {
            // Key not enough
            break;
        }
        
        key = curr;
        remaining -= key_len;
        curr += key_len;
        
        // Value
        if (remaining > 0)
        {
            // Value type
            val_type = *curr;
            remaining -= 1;
            curr += 1;
            pn = 0;

            switch (val_type)
            {
                case BSP_VAL_OBJECT : 
                    next_obj = new_object();
                    pn = _parse_object(curr, remaining, next_obj, length_mark);
                    item = new_object_item(key, key_len);
                    set_item_object(item, next_obj);
                    break;
                case BSP_VAL_ARRAY : 
                    item = new_object_item(key, key_len);
                    set_item_array(item);
                    pn = _parse_array(curr, remaining, item, length_mark);
                    break;
                default : 
                    item = new_object_item(key, key_len);
                    pn = _parse_item(curr, remaining, item, val_type, length_mark);
                    break;
            }
            object_insert_item(obj, item);
            remaining -= pn;
            curr += pn;
        }
        // No value
        else
        {
            break;
        }
    }

    return len - remaining;
}

size_t _parse_array(const char *input, size_t len, BSP_OBJECT_ITEM *array, int length_mark)
{
    if (!input || !array || array->value.type != BSP_VAL_ARRAY)
    {
        return 0;
    }

    size_t remaining = len;
    size_t pn;
    char val_type;
    char *curr = (char *) input;
    size_t idx = 0;

    BSP_OBJECT *next_obj = NULL;
    BSP_OBJECT_ITEM *item = NULL;
    while (1)
    {
        if (remaining > 0)
        {
            // Value type
            val_type = *curr;
            remaining -= 1;
            curr += 1;
            pn = 0;

            switch (val_type)
            {
                case BSP_VAL_OBJECT : 
                    next_obj = new_object();
                    pn = _parse_object(curr, remaining, next_obj, length_mark);
                    item = new_object_item(NULL, 0);
                    set_item_object(item, next_obj);
                    break;
                case BSP_VAL_ARRAY : 
                    item = new_object_item(NULL, 0);
                    set_item_array(item);
                    pn = _parse_array(curr, remaining, item, length_mark);
                    break;
                case BSP_VAL_ARRAY_END : 
                    return len - remaining;
                    break;
                default : 
                    item = new_object_item(NULL, 0);
                    pn = _parse_item(curr, remaining, item, val_type, length_mark);
                    break;
            }

            if (item->value.type > 0)
            {
                array_set_item(array, item, idx ++);
            }
            else
            {
                del_object_item(item);
            }
            
            remaining -= pn;
            curr += pn;
        }
        else
        {
            break;
        }
    }

    return len - remaining;
}

size_t object_unserialize(const char *input, size_t len, BSP_OBJECT *obj, int length_mark)
{
    if (!obj || !input)
    {
        return 0;
    }
    bsp_spin_lock(&obj->obj_lock);
    size_t pn = _parse_object(input, len, obj, length_mark);
    bsp_spin_unlock(&obj->obj_lock);

    return pn;
}
*/
/* Object <-> LUA stack */
static void _push_value_to_lua(lua_State *s, BSP_VALUE *val);
static void _push_object_to_lua(lua_State *s, BSP_OBJECT *obj);

static void _push_value_to_lua(lua_State *s, BSP_VALUE *val)
{
    if (!s || !val)
    {
        return;
    }

    int v_intlen = 0;
    uint64_t v_int = 0;
    float v_float = 0.0;
    double v_double = 0.0;
    BSP_STRING *v_str = NULL;
    void *v_ptr = NULL;
    BSP_OBJECT *v_obj = NULL;
    switch (val->type)
    {
        case BSP_VAL_INT : 
            v_int = get_vint(val->lval, &v_intlen);
            lua_pushinteger(s, (lua_Integer) v_int);
            break;
        case BSP_VAL_FLOAT : 
            v_float = get_float(val->lval);
            lua_pushnumber(s, (lua_Number) v_float);
            break;
        case BSP_VAL_DOUBLE : 
            v_double = get_double(val->lval);
            lua_pushnumber(s, (lua_Number) v_double);
            break;
        case BSP_VAL_BOOLEAN_TRUE : 
            lua_pushboolean(s, BSP_BOOLEAN_TRUE);
            break;
        case BSP_VAL_BOOLEAN_FALSE : 
            lua_pushboolean(s, BSP_BOOLEAN_FALSE);
            break;
        case BSP_VAL_STRING : 
            v_str = (BSP_STRING *) val->rval;
            lua_pushlstring(s, STR_STR(v_str), STR_LEN(v_str));
            break;
        case BSP_VAL_POINTER : 
            v_ptr = val->rval;
            lua_pushlightuserdata(s, v_ptr);
            break;
        case BSP_VAL_OBJECT : 
            v_obj = (BSP_OBJECT *) val->rval;
            _push_object_to_lua(s, v_obj);
            break;
        case BSP_VAL_NULL : 
        case BSP_VAL_UNKNOWN : 
        default : 
            lua_pushnil(s);
            break;
    }

    return;
}

static void _push_object_to_lua(lua_State *s, BSP_OBJECT *obj)
{
    if (!obj || !s)
    {
        return;
    }

    BSP_VALUE *val = NULL;
    BSP_STRING *key = NULL;
    bsp_spin_lock(&obj->lock);
    lua_checkstack(s, 1);
    switch (obj->type)
    {
        case OBJECT_TYPE_SINGLE : 
            // Single value
            val = (BSP_VALUE *) obj->node;
            _push_value_to_lua(s, val);
            break;
        case OBJECT_TYPE_ARRAY : 
            // Array
            lua_newtable(s);
            struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
            size_t idx;
            if (array)
            {
                for (idx = 0; idx < array->nitems; idx ++)
                {
                    size_t bucket = idx / ARRAY_BUCKET_SIZE;
                    size_t seq = idx % ARRAY_BUCKET_SIZE;
                    if (bucket < array->nbuckets && array->items[bucket])
                    {
                        val = array->items[bucket][seq];
                        if (val)
                        {
                            lua_checkstack(s, 2);
                            lua_pushinteger(s, (lua_Integer) idx + 1);
                            _push_value_to_lua(s, val);
                            lua_settable(s, -3);
                        }
                    }
                }
            }
            break;
        case OBJECT_TYPE_HASH : 
            // Hash
            lua_newtable(s);
            reset_object(obj);
            val = curr_item(obj);
            while (val)
            {
                key = curr_hash_key(obj);
                if (key)
                {
                    lua_checkstack(s, 2);
                    lua_pushlstring(s, STR_STR(key), STR_LEN(key));
                    _push_value_to_lua(s, val);
                    lua_settable(s, -3);
                }
                next_item(obj);
                val = curr_item(obj);
            }
            break;
        case OBJECT_TYPE_UNDETERMINED : 
        default : 
            break;
    }
    bsp_spin_unlock(&obj->lock);
    return;
}

void object_to_lua_stack(lua_State *s, BSP_OBJECT *obj)
{
    if (!obj || !s)
    {
        return;
    }

    _push_object_to_lua(s, obj);

    return;
}

static BSP_VALUE * _lua_value_to_value(lua_State *s);
static BSP_OBJECT * _lua_table_to_object(lua_State *s);

static BSP_VALUE * _lua_value_to_value(lua_State *s)
{
    if (!s)
    {
        return NULL;
    }

    BSP_VALUE *ret = new_value();
    if (ret)
    {
        lua_Number v_number = 0;
        int v_boolean = 0;
        size_t str_len = 0;
        const char *v_str = NULL;
        void *v_ptr = NULL;
        BSP_OBJECT *v_obj = NULL;
        switch (lua_type(s, -1))
        {
            case LUA_TNIL : 
                value_set_null(ret);
                break;
            case LUA_TNUMBER : 
                v_number = lua_tonumber(s, -1);
                if (v_number == (lua_Number)(int64_t) v_number)
                {
                    // Integer
                    value_set_int(ret, (const int64_t) v_number);
                }
                else
                {
                    // Double / Float
                    value_set_double(ret, (const double) v_number);
                }
                break;
            case LUA_TBOOLEAN : 
                v_boolean = lua_toboolean(s, -1);
                if (BSP_BOOLEAN_FALSE == v_boolean)
                {
                    value_set_boolean_false(ret);
                }
                else
                {
                    value_set_boolean_true(ret);
                }
                break;
            case LUA_TSTRING : 
                v_str = lua_tolstring(s, -1, &str_len);
                value_set_string(ret, new_string(v_str, str_len));
                break;
            case LUA_TUSERDATA : 
                v_ptr = lua_touserdata(s, -1);
                value_set_pointer(ret, (const void *) v_ptr);
                break;
            case LUA_TTABLE : 
                v_obj = _lua_table_to_object(s);
                value_set_object(ret, v_obj);
                break;
            default : 
                break;
        }
    }

    return ret;
}

static BSP_OBJECT * _lua_table_to_object(lua_State *s)
{
    if (!s || !lua_istable(s, -1))
    {
        return NULL;
    }

    BSP_OBJECT *ret = NULL;
    BSP_VALUE *val = NULL;
    // Is array or hash?
    if (luaL_len(s, -1) == lua_table_size(s, -1))
    {
        // Array
        ret = new_object(OBJECT_TYPE_ARRAY);
        size_t idx;
        for (idx = 1; idx <= luaL_len(s, -1); idx ++)
        {
            lua_rawgeti(s, -1, idx);
            val = _lua_value_to_value(s);
            object_set_array(ret, idx - 1, val);
        }
    }
    else
    {
        // Hash
        ret = new_object(OBJECT_TYPE_HASH);
        const char *key_str = NULL;
        size_t key_len = 0;
        BSP_STRING *key = NULL;
        lua_checkstack(s, 2);
        lua_pushnil(s);
        while (0 != lua_next(s, -2))
        {
            // Key
            key_str = lua_tolstring(s, -2, &key_len);
            key = new_string(key_str, key_len);
            // Value
            val = _lua_value_to_value(s);
            object_set_hash(ret, key, val);
            lua_pop(s, 1);
        }
    }
    debug_object(ret);

    return ret;
}

BSP_OBJECT * lua_stack_to_object(lua_State *s)
{
    if (!s)
    {
        return NULL;
    }

    BSP_OBJECT *ret = NULL;
    if (lua_istable(s, -1))
    {
        // Array or hash
        ret = _lua_table_to_object(s);
    }
    else
    {
        // Single
        ret = new_object(OBJECT_TYPE_SINGLE);
        object_set_single(ret, _lua_value_to_value(s));
    }

    return ret;
}

/* Lua Operator */
/*
// Push object into LUA stack
static void _array_to_lua(BSP_OBJECT_ITEM *array, lua_State *s)
{
    if (!array || !s || array->value.type != BSP_VAL_ARRAY)
    {
        return;
    }

    BSP_OBJECT *next_obj;
    BSP_OBJECT_ITEM **list = (BSP_OBJECT_ITEM **) array->value.rval;
    BSP_OBJECT_ITEM *item = NULL;
    if (!list)
    {
        return;
    }

    size_t idx;
    lua_checkstack(s, 16);
    lua_newtable(s);
    
    for (idx = 0; idx < array->value.rval_len; idx ++)
    {
        item = list[idx];
        if (!item)
        {
            // Last ?
            continue;
        }
        
        switch (item->value.type)
        {
            case BSP_VAL_INT8 : 
                lua_checkstack(s, 1);
                lua_pushinteger(s, (lua_Integer) get_int8(item->value.lval));
                break;
            case BSP_VAL_INT16 : 
                lua_checkstack(s, 1);
                lua_pushinteger(s, (lua_Integer) get_int16(item->value.lval));
                break;
            case BSP_VAL_INT32 : 
                lua_checkstack(s, 1);
                lua_pushinteger(s, (lua_Integer) get_int32(item->value.lval));
                break;
            case BSP_VAL_INT64 : 
                lua_checkstack(s, 1);
                lua_pushinteger(s, (lua_Integer) get_int64(item->value.lval));
                break;
            case BSP_VAL_FLOAT : 
                lua_checkstack(s, 1);
                lua_pushnumber(s, (lua_Number) get_float(item->value.lval));
                break;
            case BSP_VAL_DOUBLE : 
                lua_checkstack(s, 1);
                lua_pushnumber(s, (lua_Number) get_double(item->value.lval));
                break;
            case BSP_VAL_STRING : 
                lua_checkstack(s, 1);
                lua_pushlstring(s, (const char *) item->value.rval, item->value.rval_len);
                break;
            case BSP_VAL_ARRAY : 
                _array_to_lua(item, s);
                break;
            case BSP_VAL_OBJECT : 
                next_obj = (BSP_OBJECT *) item->value.rval;
                if (next_obj)
                {
                    object_to_lua(next_obj, s);
                }
                break;
            case BSP_VAL_NULL : 
            default : 
                lua_checkstack(s, 1);
                lua_pushnil(s);
                break;
        }
        lua_rawseti(s, -2, idx + 1);
    }
    
    return;
}

void object_to_lua(BSP_OBJECT *obj, lua_State *s)
{
    if (!obj || !s)
    {
        return;
    }

    BSP_OBJECT *next_obj;
    BSP_OBJECT_ITEM *curr;

    lua_checkstack(s, 16);
    lua_checkstack(s, 3);
    lua_newtable(s);
    bsp_spin_lock(&obj->obj_lock);
    reset_object(obj);
    while ((curr = curr_item(obj)))
    {
        if (curr->key && curr->key_len)
        {
            lua_pushlstring(s, curr->key, curr->key_len);
            switch (curr->value.type)
            {
                case BSP_VAL_INT8 : 
                    lua_pushinteger(s, (lua_Integer) get_int8(curr->value.lval));
                    break;
                case BSP_VAL_INT16 : 
                    lua_pushinteger(s, (lua_Integer) get_int16(curr->value.lval));
                    break;
                case BSP_VAL_INT32 : 
                    lua_pushinteger(s, (lua_Integer) get_int32(curr->value.lval));
                    break;
                case BSP_VAL_INT64 : 
                    lua_pushinteger(s, (lua_Integer) get_int64(curr->value.lval));
                    break;
                case BSP_VAL_FLOAT : 
                    lua_pushnumber(s, (lua_Number) get_float(curr->value.lval));
                    break;
                case BSP_VAL_DOUBLE : 
                    lua_pushnumber(s, (lua_Number) get_double(curr->value.lval));
                    break;
                case BSP_VAL_STRING : 
                    lua_pushlstring(s, (const char *) curr->value.rval, curr->value.rval_len);
                    break;
                case BSP_VAL_POINTER : 
                    lua_pushlightuserdata(s, (void *) curr->value.rval);
                    break;
                case BSP_VAL_ARRAY : 
                    _array_to_lua(curr, s);
                    break;
                case BSP_VAL_OBJECT : 
                    next_obj = (BSP_OBJECT *) curr->value.rval;
                    if (next_obj)
                    {
                        object_to_lua(next_obj, s);
                    }
                    break;
                case BSP_VAL_NULL : 
                default : 
                    lua_pushnil(s);
                    break;
            }
        }
        if (lua_istable(s, -3))
        {
            lua_settable(s, -3);
        }
        else
        {
            break;
        }
        next_item(obj);
    }
    bsp_spin_unlock(&obj->obj_lock);

    return;
}

// Import items from lua state to object
static void _array_from_lua(BSP_OBJECT_ITEM *array, lua_State *s, int idx)
{
    if (!array || !s || array->value.type != BSP_VAL_ARRAY)
    {
        return;
    }
    
    if (!lua_istable(s, idx))
    {
        return;
    }
    
    int64_t v_longlong = 0;
    double v_double = 0.0f;
    int v_boolean = 0;
    const char *v_string = NULL;
    size_t v_string_len = 0;
    size_t n;
    BSP_OBJECT_ITEM *item = NULL;
    
    for (n = 1; n <= lua_rawlen(s, idx); n ++)
    {
        item = new_object_item(NULL, 0);
        lua_rawgeti(s, idx, n);
        
        if (lua_isnumber(s, -1))
        {
            v_longlong = (int64_t) lua_tointeger(s, -1);
            v_double = (double) lua_tonumber(s, -1);
            if (v_double == (double) v_longlong)
            {
                // Integer
                if (v_longlong == (int8_t) v_longlong)
                {
                    set_item_int8(item, (const int8_t) v_longlong);
                }
                else if (v_longlong == (int16_t) v_longlong)
                {
                    set_item_int16(item, (const int16_t) v_longlong);
                }
                else if (v_longlong == (int32_t) v_longlong)
                {
                    set_item_int32(item, (const int32_t) v_longlong);
                }
                else
                {
                    set_item_int64(item, (const int64_t) v_longlong);
                }
            }
            else
            {
                // Float
                set_item_double(item, (const double) v_double);
            }
        }
        else if (lua_isboolean(s, -1))
        {
            v_boolean = lua_toboolean(s, -1);
            set_item_boolean(item, (const int) v_boolean);
        }
        else if (lua_isstring(s, -1))
        {
            v_string = lua_tolstring(s, -1, &v_string_len);
            if (v_string)
            {
                set_item_string(item, (const char *) v_string, v_string_len);
            }
            else
            {
                set_item_string(item, "NULL", -1);
            }
        }
        else if (lua_istable(s, -1))
        {
            // Recursive
            // Whether array
            size_t n = 0;
            lua_checkstack(s, 1);
            lua_pushnil(s);
            while (lua_next(s, -2))
            {
                n ++;
                lua_pop(s, 1);
            }

            if (n == lua_rawlen(s, -1))
            {
                // Array
                set_item_array(item);
                _array_from_lua(item, s, -1);
            }
            else
            {
                // Object
                BSP_OBJECT *next_obj = new_object();
                object_from_lua(next_obj, s, -1);
                set_item_object(item, next_obj);
            }
        }
        else
        {
            set_item_null(item);
        }
        
        array_set_item(array, item, n - 1);
        lua_pop(s, 1);
    }

    return;
}

void object_from_lua(BSP_OBJECT *obj, lua_State *s, int idx)
{
    // idx, negative only
    if (!obj || !s)
    {
        return;
    }
    
    if (!lua_istable(s, idx))
    {
        // Not table
        return;
    }
    
    const char *key = NULL;
    size_t key_len = 0;
    int64_t v_longlong = 0;
    double v_double = 0.0f;
    const char *v_string = NULL;
    size_t v_string_len = 0;
    int v_boolean = 0;
    void *v_pointer = NULL;
    BSP_OBJECT_ITEM *item = NULL;
    
    lua_checkstack(s, 1);
    lua_pushnil(s);
    while (lua_next(s, idx - 1))
    {
        // Read key first
        lua_checkstack(s, 1);
        lua_pushvalue(s, -2);
        key = lua_tolstring(s, -1, &key_len);
        if (key && key_len > 0)
        {
            item = new_object_item(key, key_len);
            lua_pop(s, 1);
            // Then value
            if (lua_isnumber(s, -1))
            {
                v_longlong = (int64_t) lua_tointeger(s, -1);
                v_double = (double) lua_tonumber(s, -1);
                if (v_double == (double) v_longlong)
                {
                    // Integer
                    if (v_longlong == (int8_t) v_longlong)
                    {
                        set_item_int8(item, (const int8_t) v_longlong);
                    }
                    else if (v_longlong == (int16_t) v_longlong)
                    {
                        set_item_int16(item, (const int16_t) v_longlong);
                    }
                    else if (v_longlong == (int32_t) v_longlong)
                    {
                        set_item_int32(item, (const int32_t) v_longlong);
                    }
                    else
                    {
                        set_item_int64(item, (const int64_t) v_longlong);
                    }
                }
                else
                {
                    // Float
                    set_item_double(item, (const double) v_double);
                }
            }
            else if (lua_isboolean(s, -1))
            {
                v_boolean = lua_toboolean(s, -1);
                set_item_boolean(item, (const int) v_boolean);
            }
            else if (lua_isstring(s, -1))
            {
                v_string = lua_tolstring(s, -1, &v_string_len);
                if (v_string)
                {
                    set_item_string(item, (const char *) v_string, v_string_len);
                }
                else
                {
                    set_item_string(item, "NULL", -1);
                }
            }
            else if (lua_islightuserdata(s, -1))
            {
                v_pointer = lua_touserdata(s, -1);
                set_item_pointer(item, v_pointer);
            }
            else if (lua_istable(s, -1))
            {
                // Recursive
                // Whether array
                int n = 0;
                lua_checkstack(s, 1);
                lua_pushnil(s);
                while (lua_next(s, -2))
                {
                    n ++;
                    lua_pop(s, 1);
                }
                
                if (n == luaL_len(s, -1))
                {
                    // Array
                    set_item_array(item);
                    _array_from_lua(item, s, -1);
                }
                else
                {
                    // Object
                    BSP_OBJECT *next_obj = new_object();
                    object_from_lua(next_obj, s, -1);
                    set_item_object(item, next_obj);
                }
            }
            else
            {
                set_item_null(item);
            }

            object_insert_item(obj, item);
        }
        else
        {
            lua_pop(s, 1);
        }
        
        lua_pop(s, 1);
    }
    
    return;
}
*/
