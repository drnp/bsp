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
 */

#include "bsp.h"

/* Set values */
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

void set_item_boolean(BSP_VALUE *val, const int value)
{
    if (val)
    {
        set_int8((value == 0) ? BSP_BOOLEAN_FALSE : BSP_BOOLEAN_TRUE, val->lval);
        val->type = BSP_VAL_BOOLEAN;
    }

    return;
}

void set_item_float(BSP_VALUE *val, const float value)
{
    if (val)
    {
        set_float(value, val->lval);
        val->type = BSP_VAL_FLOAT;
    }

    return;
}

void set_item_double(BSP_VALUE *val, const double value)
{
    if (val)
    {
        set_double(value, val->lval);
        val->type = BSP_VAL_DOUBLE;
    }

    return;
}

void set_item_pointer(BSP_OBJECT_ITEM *val, const void *p)
{
    if (val)
    {
        val->rval = (void *) p;
        val->type = BSP_VAL_POINTER;
    }

    return;
}

void set_item_string(BSP_VALUE *val, BSP_STRING *str)
{
    if (val & str)
    {
        val->rval = (void *) str;
        val->type = BSP_VAL_STRING;
    }

    return;
}

void set_item_object(BSP_VALUE *val, BSP_OBJECT *obj)
{
    if (val && obj)
    {
        val->rval = (void *) obj;
        val->type = BSP_VAL_OBJECT;
    }

    return;
}

void set_item_null(BSP_OBJECT_ITEM *val)
{
    if (val)
    {
        val->type = BSP_VAL_NULL;
    }

    return;
}

/*
void set_item_int8(BSP_OBJECT_ITEM *item, const int8_t value)
{
    if (item)
    {
        set_int8(value, item->value.lval);
        item->value.type = BSP_VAL_INT8;
    }
    
    return;
}

void set_item_int16(BSP_OBJECT_ITEM *item, const int16_t value)
{
    if (item)
    {
        set_int16(value, item->value.lval);
        item->value.type = BSP_VAL_INT16;
    }
    
    return;
}

void set_item_int32(BSP_OBJECT_ITEM *item, const int32_t value)
{
    if (item)
    {
        set_int32(value, item->value.lval);
        item->value.type = BSP_VAL_INT32;
    }

    return;
}

void set_item_int64(BSP_OBJECT_ITEM *item, const int64_t value)
{
    if (item)
    {
        set_int64(value, item->value.lval);
        item->value.type = BSP_VAL_INT64;
    }

    return;
}

void set_item_boolean(BSP_OBJECT_ITEM *item, const int value)
{
    if (item)
    {
        set_int8((value == 0) ? BSP_BOOLEAN_FALSE : BSP_BOOLEAN_TRUE, item->value.lval);
        item->value.type = BSP_VAL_BOOLEAN;
    }

    return;
}

void set_item_float(BSP_OBJECT_ITEM *item, const float value)
{
    if (item)
    {
        set_float(value, item->value.lval);
        item->value.type = BSP_VAL_FLOAT;
    }
    
    return;
}

void set_item_double(BSP_OBJECT_ITEM *item, const double value)
{
    if (item)
    {
        set_double(value, item->value.lval);
        item->value.type = BSP_VAL_DOUBLE;
    }
}

void set_item_string(BSP_OBJECT_ITEM *item, const char *value, ssize_t len)
{
    if (item)
    {
        if (!value)
        {
            value = "";
        }
        
        if (len < 0)
        {
            len = strlen(value);
        }
        
        item->value.rval = bsp_malloc(len);
        if (item->value.rval)
        {
            memcpy(item->value.rval, value, len);
            item->value.rval_len = len;
        }
        
        item->value.type = BSP_VAL_STRING;
    }
    
    return;
}

void set_item_array(BSP_OBJECT_ITEM *item)
{
    if (item)
    {
        item->value.rval = bsp_calloc(ARRAY_SIZE_INITIAL, sizeof(BSP_OBJECT_ITEM *));
        if (item->value.rval)
        {
            // Set length
            item->value.rval_len = ARRAY_SIZE_INITIAL;
        }

        item->value.type = BSP_VAL_ARRAY;
    }
}

void set_item_object(BSP_OBJECT_ITEM *item, BSP_OBJECT *obj)
{
    if (item && obj)
    {
        item->value.rval = (void *) obj;
        item->value.type = BSP_VAL_OBJECT;
    }
    
    return;
}
废物
void set_item_pointer(BSP_OBJECT_ITEM *item, const void *p)
{
    if (item)
    {
        item->value.rval = (void *) p;
        item->value.type = BSP_VAL_POINTER;
    }
}

void set_item_null(BSP_OBJECT_ITEM *item)
{
    if (item)
    {
        item->value.type = BSP_VAL_NULL;
    }
    
    return;
}
*/

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

    bsp_spin_lock(&obj->lock);
    bsp_spin_unlock(&obj->lock);

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
BSP_VALUE * curr_item(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return NULL;
    }

    BSP_VALUE *curr;
    switch (obj->type)
    {
        case OBJECT_TYPE_SINGLE : 
            curr = (BSP_VALUE *) obj->node;
            break;
        case OBJECT_TYPE_ARRAY : 
            struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
            if (array)
            {
                size_t bucket = (array->curr / ARRAY_BUCKET_SIZE);
                if (array->curr < array->nitems && bucket < array->nbuckets && array->items[bucket])
                {
                    curr = array->items[bucket][curr % ARRAY_BUCKET_SIZE];
                }
            }
            break;
        case OBJECT_TYPE_HASH : 
            struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
            if (hash)
            {
                if (hash->curr)
                {
                    curr = hash->curr->value;
                }
            }
            break;
        default : 
            curr = NULL;
            break;
    }

    return curr;
}

void reset_obj(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }

    switch (obj->type)
    {
        case OBJECT_TYPE_ARRAY : 
            struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
            if (array)
            {
                array->curr = 0;
            }
            break;
        case OBJECT_TYPE_HASH : 
            struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
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

    switch (obj->type)
    {
        case OBJECT_TYPE_ARRAY : 
            struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
            if (array && array->curr < array->nitems)
            {
                array->curr ++;
            }
            break;
        case OBJECT_TYPE_HASH : 
            struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
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

    break;
}

void prev_item(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }

    switch (obj->type)
    {
        case OBJECT_TYPE_ARRAY : 
            struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
            if (array && array->curr > 0)
            {
                array->curr --;
            }
            break;
        case OBJECT_TYPE_HASH : 
            struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
            if (hash && hash->curr)
            {
                hash->curr = hash->curr->lprev;
            }
            break;
        case OBJECT_TYPE_SINGLE : 
        default : 
            // Nothing to NEXT
            break;
    }
}

/*
BSP_OBJECT_ITEM * curr_item(BSP_OBJECT *obj)
{
    if (obj && obj->curr)
    {
        return obj->curr;
    }
    
    return NULL;
}

void next_item(BSP_OBJECT *obj)
{
    if (obj && obj->curr)
    {
        obj->curr = obj->curr->lnext;
    }
    
    return;
}

void prev_item(BSP_OBJECT *obj)
{
    if (obj && obj->curr)
    {
        if (obj->curr->lprev != obj->head)
        {
            obj->curr = obj->curr->lprev;
        }
        
        else
        {
            obj->curr = NULL;
        }
    }
    
    return;
}

void reset_object(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }
    
    obj->curr = obj->head;
    
    return;
}

// Alloc a new item
BSP_OBJECT_ITEM * new_object_item(const char *key, ssize_t key_len)
{
    BSP_OBJECT_ITEM *item = NULL;
    if (key_len < 0 && key)
    {
        key_len = strlen(key);
    }
    
    item = (BSP_OBJECT_ITEM *) bsp_calloc(1, sizeof(BSP_OBJECT_ITEM));
    if (!item)
    {
        return NULL;
    }
    
    if (key)
    {
        item->key = (char *) bsp_malloc(key_len);
        memcpy(item->key, key, key_len);
        item->key_len = key_len;
    }
    item->key_int = 0;
    
    return item;
}

// Empty an item
void free_object_item(BSP_OBJECT_ITEM *item)
{
    if (!item)
    {
        return;
    }

    if (item->value.type == BSP_VAL_OBJECT)
    {
        del_object((BSP_OBJECT *) item->value.rval);
    }

    else if (item->value.type == BSP_VAL_ARRAY)
    {
        size_t i;
        BSP_OBJECT_ITEM **list = (BSP_OBJECT_ITEM **) item->value.rval;
        for (i = 0; i < item->value.rval_len; i ++)
        {
            if (list[i])
            {
                del_object_item(list[i]);
            }
        }
    }

    if (item->next)
    {
        item->next->prev = item->prev;
    }

    if (item->prev)
    {
        item->prev->next = item->next;
    }

    if (item->lnext)
    {
        item->lnext->lprev = item->lprev;
    }

    if (item->lprev)
    {
        item->lprev->lnext = item->lnext;
    }
    
    item->next = NULL;
    item->prev = NULL;
    item->lnext = NULL;
    item->lprev = NULL;
    
    if (item->key)
    {
        bsp_free(item->key);
    }
    
    if (item->value.rval && (item->value.type == BSP_VAL_STRING || item->value.type == BSP_VAL_ARRAY))
    {
        bsp_free(item->value.rval);
    }
    
    memset(item, 0, sizeof(BSP_OBJECT_ITEM));
    
    return;
}

// Delete an item
void del_object_item(BSP_OBJECT_ITEM *item)
{
    if (!item)
    {
        return;
    }
    
    free_object_item(item);
    bsp_free(item);
    
    return;
}

// Create a new object
BSP_OBJECT * new_object()
{
    BSP_OBJECT *obj = (BSP_OBJECT *) bsp_calloc(1, sizeof(BSP_OBJECT));
    if (!obj)
    {
        return NULL;
    }
    
    obj->hash_size = HASH_SIZE_INITIAL;
    obj->hash_table[0] = (BSP_OBJECT_ITEM *) bsp_calloc(obj->hash_size, sizeof(BSP_OBJECT_ITEM));
    obj->hash_table[1] = (BSP_OBJECT_ITEM *) bsp_calloc(obj->hash_size, sizeof(BSP_OBJECT_ITEM));
    
    if (!obj->hash_table[0] || !obj->hash_table[1])
    {
        _exit(BSP_RTN_ERROR_MEMORY);
    }
    
    obj->curr_table = 0;
    bsp_spin_init(&obj->obj_lock);
    
    return obj;
}

// empty an object
void free_object(BSP_OBJECT *obj)
{
    BSP_OBJECT_ITEM *curr;
    
    if (!obj)
    {
        return;
    }
    
    // Recycle every item
    reset_object(obj);
    
    while ((curr = curr_item(obj)))
    {
        next_item(obj);
        del_object_item(curr);
    }
    
    obj->nitems = 0;
    obj->head = obj->tail = NULL;
    
    // Empty hash table
    memset(obj->hash_table[0], 0, sizeof(BSP_OBJECT_ITEM) * obj->hash_size);
    memset(obj->hash_table[1], 0, sizeof(BSP_OBJECT_ITEM) * obj->hash_size);
    
    return;
}

// Delete an object
void del_object(BSP_OBJECT *obj)
{
    if (!obj)
    {
        return;
    }
    
    free_object(obj);
    bsp_spin_destroy(&obj->obj_lock);
    bsp_free((void *) obj->hash_table[0]);
    bsp_free((void *) obj->hash_table[1]);
    bsp_free((void *) obj);
    
    return;
}
*/
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
static struct bsp_hash_item_t * _find_hash(struct bsp_hash_t *hash, BSP_STRING *key)
{
    struct bsp_hash_item_t *ret = NULL;
    if (hash && key && hash->hash_table)
    {
        uint32_t hash_key = hash(STR_STR(key), STR_LEN(key));
        struct bsp_hash_item_t * curr = hash->hash_table[hash_key % hash->hash_size];
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

// Remove item from hash
static int _remove_hash(struct bsp_hash_t *hash, BSP_STRING *key)
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
            uint32_t hash_key = hash(STR_STR(key), STR_LEN(key));
            struct bsp_hash_item_t *bucket = &hash->hash_table[hash_key % hash_size];
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
            bsp_free(hash_table);
        }
        hash->hash_table = bsp_calloc(new_hash_size, sizeof(struct bsp_hash_item_t));
        hash->hash_size = new_hash_size;

        // Insert every item from link
        struct bsp_hash_item_t *curr = hash->head;
        struct bsp_hash_item_t *bucket;
        uint32_t hash_key;
        while (curr)
        {
            hash_key = hash(STR_STR(key), STR_LEN(key));
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
        obj->node = (BSP_VALUE *) val;
    }

    return;
}

// Add an item into array
void object_set_array(BSP_OBJECT *obj, ssize_t idx, BSP_VALUE *val)
{
    if (obj && OBJECT_TYPE_ARRAY == obj->type)
    {
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
            nbuckets = 2 << (log2(bucket + 1));
            array->items = bsp_realloc(array->item, nbuckets * sizeof(struct BSP_VAL **));
            array->nbuckets = nbuckets;
        }

        if (!array->item[bucket])
        {
            // New bucket
            array->item[bucket] = bsp_calloc(ARRAY_BUCKET_SIZE, sizeof(struct BSP_VAL *));
        }

        if (array->item[bucket][seq])
        {
            // Remove old value
            del_value(array->item[bucket][seq]);
        }
        array->item[bucket][seq] = val;
        if (idx >= array->nitems)
        {
            array->nitems = idx + 1;
        }
    }

    return;
}

// Insert / Remove an item into / from a hash
void object_set_hash(BSP_OBJECT_*obj, BSP_STRING *key, BSP_VALUE *val)
{
    if (obj && OBJECT_TYPE_HASH == obj->type && key)
    {
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
        }
        else
        {
            // Remove
            hash->nitems -= _remove_hash(hash, key);
        }
    }

    return;
}

// Get single value from object
BSP_VALUE * object_get_single(BSP_OBJECT *obj)
{
    BSP_VALUE *ret = NULL;
    if (obj && OBJECT_TYPE_SINGLE == obj->type)
    {
        ret = (BSP_VALUE *) obj->node;
    }

    return ret;
}

// Get value from array by given index
BSP_VALUE * object_get_array(BSP_OBJECT *obj, size_t idx)
{
    BSP_VALUE *ret = NULL;
    if (obj && OBJECT_TYPE_ARRAY == obj->type)
    {
        struct bsp_array_t *array = (struct bsp_array_t *) obj->node;
        size_t bucket = idx / ARRAY_BUCKET_SIZE;
        size_t seq = idx % ARRAY_BUCKET_SIZE;
        if (array && idx < array->nitems && bucket < array->nbuckets && array->items[bucket])
        {
            ret = array->items[bucket][seq];
        }
    }

    return ret;
}

// Get value from hash table
BSP_VALUE * object_get_hash(BSP_OBJECT *obj, BSP_STRING *key)
{
    BSP_VALUE *ret = NULL;
    if (obj && OBJECT_TYPE_HASH == obj->type)
    {
        struct bsp_hash_t *hash = (struct bsp_hash_t *) obj->node;
        if (hash)
        {
            struct bsp_hash_item_t *item = _find_hash(hash, key);
            if (item)
            {
                ret = item->value;
            }
        }
    }

    return ret;
}

/*
// Rehash object
static int rehash_object(BSP_OBJECT *obj, size_t new_hash_size)
{
    // Single slot used now
    // TODO : I will change this to dual-slot mode soon(maybe -_-)
    if (!obj)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    obj->hash_table[0] = (BSP_OBJECT_ITEM *) bsp_realloc(obj->hash_table[0], sizeof(BSP_OBJECT_ITEM) * new_hash_size);
    obj->hash_table[1] = (BSP_OBJECT_ITEM *) bsp_realloc(obj->hash_table[1], sizeof(BSP_OBJECT_ITEM) * new_hash_size);
    
    if (!obj->hash_table[0] || !obj->hash_table[1])
    {
        return BSP_RTN_ERROR_MEMORY;
    }

    BSP_OBJECT_ITEM *curr = NULL;
    uint32_t hash_key;
    int slot = 0;

    bsp_spin_lock(&obj->obj_lock);
    // Cleanup hash table
    memset(obj->hash_table[obj->curr_table], 0, sizeof(BSP_OBJECT_ITEM) * new_hash_size);
    
    // Re-link every item into table
    reset_object(obj);
    curr = curr_item(obj);
    while (curr)
    {
        hash_key = hash(curr->key, curr->key_len);
        slot = hash_key % obj->hash_size;

        if (!obj->hash_table[obj->curr_table][slot].next)
        {
            // Slot head
            obj->hash_table[obj->curr_table][slot].next = curr;
            curr->prev = &obj->hash_table[obj->curr_table][slot];
            curr->next = NULL;
        }

        else
        {
            // Insert item to the first position
            curr->next = obj->hash_table[obj->curr_table][slot].next;
            obj->hash_table[obj->curr_table][slot].next->prev = curr;
            obj->hash_table[obj->curr_table][slot].next = curr;
        }
        next_item(obj);
        curr = curr_item(obj);
    }
    bsp_spin_unlock(&obj->obj_lock);
    
    return BSP_RTN_SUCCESS;
}

// Insert an item to object
int object_insert_item(BSP_OBJECT *obj, BSP_OBJECT_ITEM *item)
{
    if (!obj || !item || !item->key)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    bsp_spin_lock(&obj->obj_lock);
    // Check duplication
    BSP_OBJECT_ITEM *check = object_get_item(obj, item->key, item->key_len);
    if (check)
    {
        // Duplicated, just replace value here
        memcpy(&check->value, &item->value, sizeof(struct bsp_object_item_val_t));

        del_object_item(item);
        bsp_spin_unlock(&obj->obj_lock);

        return BSP_RTN_SUCCESS;
    }
    
    if (!obj->head)
    {
        obj->head = item;
        item->lprev = NULL;
    }
    
    if (obj->tail)
    {
        obj->tail->lnext = item;
        item->lprev = obj->tail;
    }
    
    obj->tail = item;
    item->lnext = NULL;
    
    // Insert into hash table
    uint32_t hash_key = hash(item->key, item->key_len);
    int slot = hash_key % obj->hash_size;

    if (!obj->hash_table[obj->curr_table][slot].next)
    {
        // Slot head
        obj->hash_table[obj->curr_table][slot].next = item;
        item->prev = &obj->hash_table[obj->curr_table][slot];
        item->next = NULL;
    }

    else
    {
        // Insert item to the first position
        item->next = obj->hash_table[obj->curr_table][slot].next;
        obj->hash_table[obj->curr_table][slot].next->prev = item;
        obj->hash_table[obj->curr_table][slot].next = item;
    }

    obj->nitems ++;
    bsp_spin_unlock(&obj->obj_lock);

    if (obj->nitems > 4 * obj->hash_size)
    {
        rehash_object(obj, obj->hash_size * 8);
    }
    
    return BSP_RTN_SUCCESS;
}

// Remove item from hash table
BSP_OBJECT_ITEM * object_remove_item(BSP_OBJECT *obj, BSP_OBJECT_ITEM *item)
{
    if (item && obj)
    {
        // Remove from hash table
        if (item->prev)
        {
            item->prev->next = item->next;
        }
        
        if (item->next)
        {
            item->next->prev = item->prev;
        }

        bsp_spin_lock(&obj->obj_lock);
        // Remove from link
        if (obj->head == item)
        {
            // Remove head
            obj->head = item->next;
        }
        
        if (obj->tail == item)
        {
            // Remove tail
            obj->tail = item->prev;
        }
        bsp_spin_unlock(&obj->obj_lock);
        
        if (item->lprev)
        {
            item->lprev->lnext = item->lnext;
        }
        
        if (item->lnext)
        {
            item->lnext->lprev = item->lprev;
        }
        
        obj->nitems --;
    }
    
    return item;
}

// Find item from object by given key
BSP_OBJECT_ITEM * object_get_item(BSP_OBJECT *obj, const char *key, ssize_t key_len)
{
    if (!obj || !key)
    {
        return NULL;
    }
    
    BSP_OBJECT_ITEM *item = NULL, *tmp;
    if (key_len < 0)
    {
        key_len = strlen(key);
    }
    
    // Calc hash
    uint32_t hash_key = hash(key, key_len);
    int slot = hash_key % obj->hash_size;

    if (!obj->hash_table[obj->curr_table][slot].next)
    {
        item = NULL;
    }

    else
    {
        tmp = obj->hash_table[obj->curr_table][slot].next;
        do
        {
            if (tmp->key_len == key_len && 0 == memcmp(tmp->key, key, key_len))
            {
                item = tmp;
                break;
            }
            
            tmp = tmp->next;
        } while (tmp);
    }
    
    return item;
}

// Get item value
BSP_VAL * object_item_value(BSP_OBJECT *obj, BSP_VAL *val, const char *key, ssize_t key_len)
{
    memset(val, 0, sizeof(BSP_VAL));
    BSP_OBJECT_ITEM *item = object_get_item(obj, key, key_len);
    if (item)
    {
        val->type = item->value.type;
        switch (item->value.type)
        {
            case BSP_VAL_BOOLEAN : 
            case BSP_VAL_INT8 : 
                val->v_int = (int64_t) get_int8(item->value.lval);
                break;
            case BSP_VAL_INT16 : 
                val->v_int = (int64_t) get_int16(item->value.lval);
                break;
            case BSP_VAL_INT32 : 
                val->v_int = (int64_t) get_int32(item->value.lval);
                break;
            case BSP_VAL_INT64 : 
                val->v_int = (int64_t) get_int64(item->value.lval);
                break;
            case BSP_VAL_FLOAT : 
                val->v_float = (double) get_float(item->value.lval);
                break;
            case BSP_VAL_DOUBLE : 
                val->v_float = (double) get_double(item->value.lval);
                break;
            case BSP_VAL_STRING : 
                val->v_str = (char *) get_string(item->value.rval);
                val->v_str_len = (size_t) item->value.rval_len;
                break;
            case BSP_VAL_OBJECT : 
                val->v_obj = (BSP_OBJECT *) item->value.rval;
                break;
            case BSP_VAL_ARRAY : 
                val->v_arr = (BSP_OBJECT_ITEM **) item->value.rval;
                val->v_arr_size = (size_t) item->value.rval_len;
                break;
            default : 
                break;
        }

        return val;
    }
    else
    {
        return NULL;
    }
}

// Insert item into array
int array_set_item(BSP_OBJECT_ITEM *array, BSP_OBJECT_ITEM *item, size_t idx)
{
    if (!array || array->value.type != BSP_VAL_ARRAY || !item)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_OBJECT_ITEM **list = NULL;

    // Ensure space
    size_t array_size = array->value.rval_len;
    if (idx >= array_size)
    {
        int bit;
        size_t new_array_size, i;
        
        for (bit = 1; bit <= 62; bit ++)
        {
            if (1 == (idx >> bit))
            {
                break;
            }
        }

        // New size
        new_array_size = 2 << bit;
        void *tmp = bsp_realloc(array->value.rval, sizeof(BSP_OBJECT_ITEM *) * new_array_size);
        if (!tmp)
        {
            return BSP_RTN_ERROR_MEMORY;
        }

        array->value.rval_len = new_array_size;
        list = (BSP_OBJECT_ITEM **) tmp;
        for (i = array_size; i < new_array_size; i ++)
        {
            list[i] = NULL;
        }

        array->value.rval = tmp;
    }

    list = (BSP_OBJECT_ITEM **) array->value.rval;
    if (list[idx])
    {
        // Replace
        del_object_item(list[idx]);
    }

    // You can delete an item from item simply by set it to NULL
    list[idx] = item;
    item->key_int = (int64_t) idx;

    // We don't need string key any more
    if (item->key)
    {
        bsp_free(item->key);
        item->key = NULL;
        item->key_len = 0;
    }

    return BSP_RTN_SUCCESS;
}

// Get item from array
BSP_OBJECT_ITEM * array_get_item(BSP_OBJECT_ITEM *array, size_t idx)
{
    if (!array || array->value.type != BSP_VAL_ARRAY)
    {
        return NULL;
    }
    
    BSP_OBJECT_ITEM **list = (BSP_OBJECT_ITEM **) array->value.rval;
    
    return (idx >= array->value.rval_len) ? NULL : list[idx];
}
*/
/* Serializer & Unserializer */
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
    reset_object(obj);
    while ((curr = curr_item(obj)))
    {
        _insert_item_to_string(curr, output, SERIALIZE_OBJECT, length_mark);
        next_item(obj);
    }

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
    
    size_t pn = _parse_object(input, len, obj, length_mark);
    
    return pn;
}

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
*/

/* Lua Operator */
/*
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
