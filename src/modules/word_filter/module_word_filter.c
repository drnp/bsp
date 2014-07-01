/*
 * module_word_filter.c
 *
 * Copyright (C) 2013 - Dr.NP
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
 * Keyword filter with finite automation
 * 
 * @package modules::word_filter
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/23/2013
 * @changelog
 *      [05/23/2013] - Creation
 */

#include "bsp.h"

#include "module_word_filter.h"

static int _get_new_node(struct word_filter_t *flt)
{
    if (!flt)
    {
        return -1;
    }

    bsp_spin_lock(&flt->node_lock);
    int ret = flt->node_used, i;
    int block_id = ret / BLOCK_NODE_SIZE;
    struct hash_tree_node_t **tmp;
    while (block_id >= flt->node_list_size)
    {
        int nlen = (flt->node_list_size) ? flt->node_list_size * 2 : BLOCK_LIST_INITIAL;
        tmp = bsp_realloc(flt->node_list, nlen * sizeof(struct hash_tree_node_t *));
        if (tmp)
        {
            for (i = flt->node_list_size; i < nlen; i ++)
            {
                tmp[i] = NULL;
            }
            
            flt->node_list = tmp;
            flt->node_list_size = nlen;
        }

        else
        {
            bsp_spin_unlock(&flt->node_lock);
            return -1;
        }
    }

    // If block
    if (!flt->node_list[block_id])
    {
        struct hash_tree_node_t *block = bsp_calloc(BLOCK_NODE_SIZE, sizeof(struct hash_tree_node_t));
        if (block)
        {
            flt->node_list[block_id] = block;
        }

        else
        {
            bsp_spin_unlock(&flt->node_lock);
            return -1;
        }
    }

    flt->node_used ++;
    bsp_spin_unlock(&flt->node_lock);
    
    return ret;
}

static struct hash_tree_node_t * _get_node(struct word_filter_t *flt, int idx)
{
    if (!flt || idx < 0)
    {
        return NULL;
    }

    int block_id = idx / BLOCK_NODE_SIZE;
    if (block_id >= flt->node_list_size)
    {
        return NULL;
    }

    if (!flt->node_list[block_id])
    {
        return NULL;
    }

    return &flt->node_list[block_id][idx % BLOCK_NODE_SIZE];
}

static int new_word_filter(lua_State *s)
{
    if (!s)
    {
        return 0;
    }

    struct word_filter_t *flt = bsp_malloc(sizeof(struct word_filter_t));
    if (!flt)
    {
        trace_msg(TRACE_LEVEL_FATAL, "Filter : Alloc new word filter error");
        return 0;
    }

    flt->node_used = 0;
    bsp_spin_init(&flt->node_lock);
    flt->node_list = NULL;
    flt->node_list_size = 0;
    // Make root
    flt->root = _get_new_node(flt);
    lua_pushlightuserdata(s, (void *) flt);
    
    return 1;
}

static int del_word_filter(lua_State *s)
{
    if (!s || lua_gettop(s) < 1 || !lua_islightuserdata(s, -1))
    {
        return 0;
    }

    struct word_filter_t *flt = (struct word_filter_t *) lua_touserdata(s, -1);
    if (!flt)
    {
        return 0;
    }
    // Clean all block
    bsp_free(flt);
    
    return 0;
}

static int add_word(lua_State *s)
{
    const char *word = NULL;
    size_t word_len = 0;
    int dsp_len = 0;
    int value = 1;
    
    if (!s || lua_gettop(s) < 2 || !lua_istable(s, -1) || !lua_islightuserdata(s, -2))
    {
        return 0;
    }

    struct word_filter_t *flt = (struct word_filter_t *) lua_touserdata(s, -2);
    if (!flt)
    {
        return 0;
    }
    lua_getfield(s, -1, "word");
    if (lua_isstring(s, -1))
    {
        word = lua_tolstring(s, -1, &word_len);
        dsp_len = word_len;
    }
    lua_pop(s, 1);

    lua_getfield(s, -1, "len");
    if (lua_isnumber(s, -1))
    {
        dsp_len = lua_tointeger(s, -1);
    }
    lua_pop(s, 1);

    lua_getfield(s, -1, "value");
    if (lua_isnumber(s, -1))
    {
        value = lua_tointeger(s, -1);
    }
    lua_pop(s, 1);
    if (word && value)
    {
        // Insert into tree
        int n;
        unsigned char code;
        struct hash_tree_node_t *curr = _get_node(flt, flt->root);
        
        for (n = 0; n < word_len; n ++)
        {
            if (!curr)
            {
                return 0;
            }
            
            code = (unsigned char) word[n];
            if (!curr->path[code])
            {
                // New node
                curr->path[code] = _get_new_node(flt);
            }

            curr = _get_node(flt, curr->path[code]);
        }

        if (curr)
        {
            // Set values
            curr->value = value;
            curr->length = dsp_len;
        }
    }

    return 0;
}

static int del_word(lua_State *s)
{
    if (!s || lua_gettop(s) < 2 || !lua_isstring(s, -1) || !lua_islightuserdata(s, -2))
    {
        return 0;
    }

    struct word_filter_t *flt = (struct word_filter_t *) lua_touserdata(s, -2);
    if (!flt)
    {
        return 0;
    }

    size_t word_len;
    const char *word = lua_tolstring(s, -1, &word_len);

    if (word)
    {
        int n;
        unsigned char code;
        struct hash_tree_node_t *curr = _get_node(flt, flt->root);
        
        for (n = 0; n < word_len; n ++)
        {
            if (!curr)
            {
                return 0;
            }
            
            code = (unsigned char) word[n];
            if (!curr->path[code])
            {
                // Word not exists
                return 0;
            }

            curr = _get_node(flt, curr->path[code]);
        }

        if (curr)
        {
            // Empty value
            curr->value = 0;
            curr->length = 0;
        }
    }

    return 0;
}

static int do_filter(lua_State *s)
{
    if (!s || lua_gettop(s) < 2 || !lua_istable(s, -1) || !lua_islightuserdata(s, -2))
    {
        return 0;
    }

    struct word_filter_t *flt = (struct word_filter_t *) lua_touserdata(s, -2);
    if (!flt)
    {
        return 0;
    }

    struct hash_tree_node_t *root = _get_node(flt, flt->root), *curr = NULL;
    if (!root)
    {
        // No root node
        return 0;
    }

    const char *content = NULL, *replacement = NULL;
    size_t content_len = 0, n, j, seg_len = 0;
    int all = 0, matched = 0, start = 0, replace = 0;
    unsigned char code;
    BSP_STRING *res = NULL;

    lua_getfield(s, -1, "content");
    if (lua_isstring(s, -1))
    {
        content = lua_tolstring(s, -1, &content_len);
    }
    lua_pop(s, 1);

    lua_getfield(s, -1, "all");
    if (lua_isboolean(s, -1))
    {
        all = lua_toboolean(s, -1);
    }
    lua_pop(s, 1);

    lua_getfield(s, -1, "replace");
    if (lua_isboolean(s, -1))
    {
        replace = lua_toboolean(s, -1);
    }
    lua_pop(s, 1);

    lua_getfield(s, -1, "replacement");
    if (lua_isstring(s, -1))
    {
        replacement = lua_tostring(s, -1);
    }
    lua_pop(s, 1);

    lua_newtable(s);
    if (content && content_len)
    {
        if (replace)
        {
            res = new_string(NULL, 0);
            if (!replacement || !strlen(replacement))
            {
                replacement = DEFAULT_REPLACEMENT;
            }
        }
        
        for (n = 0; n < content_len; n ++)
        {
            if (!all && matched)
            {
                break;
            }
            
            curr = root;
            start = n;
            for (j = n; j <= content_len; j ++)
            {
                if (curr->value)
                {
                    // Get word
                    matched ++;
                    lua_pushinteger(s, matched);
                    lua_newtable(s);
                    lua_pushstring(s, "word");
                    lua_pushlstring(s, content + start, j - start);
                    lua_settable(s, -3);
                    lua_pushstring(s, "offset");
                    lua_pushinteger(s, start);
                    lua_settable(s, -3);
                    lua_pushstring(s, "length");
                    lua_pushinteger(s, j - start);
                    lua_settable(s, -3);
                    lua_pushstring(s, "dsp_length");
                    lua_pushinteger(s, curr->length);
                    lua_settable(s, -3);
                    lua_pushstring(s, "value");
                    lua_pushinteger(s, curr->value);
                    lua_settable(s, -3);
                    lua_settable(s, -3);

                    seg_len = j - start;
                    if (!all)
                    {
                        break;
                    }
                }

                if (j == content_len)
                {
                    break;
                }
                
                code = (unsigned char) content[j];
                if (!curr->path[code])
                {
                    // No match
                    break;
                }
                curr = _get_node(flt, curr->path[code]);
                if (!curr)
                {
                    break;
                }
            }

            if (replace)
            {
                if (seg_len > 0)
                {
                    string_printf(res, "%s", replacement);
                    seg_len --;
                }

                else
                {
                    string_printf(res, "%c", (unsigned char) content[n]);
                }
            }
        }

        if (replace)
        {
            lua_pushstring(s, "replaced");
            lua_pushlstring(s, STR_STR(res), STR_LEN(res));
            lua_settable(s, -3);
            del_string(res);
        }
    }

    return 1;
}

int bsp_module_word_filter(lua_State *s)
{
    if (!s || !lua_checkstack(s, 1))
    {
        return 0;
    }
    lua_pushcfunction(s, new_word_filter);
    lua_setglobal(s, "bsp_new_word_filter");

    lua_pushcfunction(s, del_word_filter);
    lua_setglobal(s, "bsp_del_word_filter");

    lua_pushcfunction(s, add_word);
    lua_setglobal(s, "bsp_word_filter_insert");

    lua_pushcfunction(s, del_word);
    lua_setglobal(s, "bsp_word_filter_remove");

    lua_pushcfunction(s, do_filter);
    lua_setglobal(s, "bsp_word_filter_do");

    lua_settop(s, 0);
    
    return 0;
}
