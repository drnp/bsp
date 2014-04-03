/*
 * db_sqlite.c
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
 * SQLite3 handler
 * 
 * @package bsp::libbsp-ext
 * @author Dr.NP <np@bsgroup.org>
 * @update 09/12/2012
 * @changelog 
 *      [09/12/2012] - Creation
 */

#include "bsp.h"

BSP_DB_SQLITE_RES sqlite_query_res_list_head = {.next = NULL};
BSP_DB_SQLITE_RES *sqlite_query_res_list_tail = NULL;
BSP_SPINLOCK sqlite_res_list_lock;

static inline void _add_to_list(BSP_DB_SQLITE_RES *res)
{
    if (!res)
    {
        return;
    }

    res->next = NULL;
    bsp_spin_lock(&sqlite_res_list_lock);
    if (!sqlite_query_res_list_head.next)
    {
        sqlite_query_res_list_head.next = res;
        res->prev = &sqlite_query_res_list_head;
    }

    else
    {
        if (!sqlite_query_res_list_tail)
        {
            sqlite_query_res_list_tail = &sqlite_query_res_list_head;
        }

        res->prev = sqlite_query_res_list_tail;
        sqlite_query_res_list_tail->next = res;
    }

    sqlite_query_res_list_tail = res;
    
    bsp_spin_unlock(&sqlite_res_list_lock);

    return;
}

static inline void _remove_from_list(BSP_DB_SQLITE_RES *res)
{
    if (!res)
    {
        return;
    }

    // Remove from list
    bsp_spin_lock(&sqlite_res_list_lock);
    if (res->prev)
    {
        res->prev->next = res->next;
    }

    if (res->next)
    {
        res->next->prev = res->prev;
    }
    
    bsp_spin_unlock(&sqlite_res_list_lock);

    return;
}

// Open a sqlite database
BSP_DB_SQLITE * db_sqlite_open(const char *dbfile)
{
    if (!dbfile)
    {
        return NULL;
    }

    // Try open
    trace_msg(TRACE_LEVEL_NOTICE, "SQLite : Try to open SQLite database <%s>", dbfile);
    sqlite3 *db;
    int ret = sqlite3_open(dbfile, &db);
    if (SQLITE_OK != ret)
    {
        trace_msg(TRACE_LEVEL_ERROR, "SQLite : Cannot open SQLite database <%s> : %s", dbfile, sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    BSP_DB_SQLITE *l = mempool_alloc(sizeof(BSP_DB_SQLITE));
    if (l)
    {
        l->conn = db;
        l->queries = 0;
        bsp_spin_init(&l->query_lock);
    }

    bsp_spin_init(&sqlite_res_list_lock);
    status_op_db_sqlite(STATUS_OP_DB_SQLITE_OPEN);

    return l;
}

// Close SQLite object
void db_sqlite_close(BSP_DB_SQLITE *l)
{
    if (!l || !l->conn)
    {
        return;
    } 

    // Clear results
    BSP_DB_SQLITE_RES *curr = sqlite_query_res_list_head.next, *next = NULL;
    while (curr)
    {
        next = curr->next;
        if (curr->handler == l->conn)
        {
            _remove_from_list(curr);
            if (curr->stmt)
            {
                sqlite3_finalize(curr->stmt);
            }
            mempool_free(curr);
        }
        curr = next;
    }

    trace_msg(TRACE_LEVEL_NOTICE, "SQLite : Try to close SQLite handler");
    sqlite3_close(l->conn);
    status_op_db_sqlite(STATUS_OP_DB_SQLITE_CLOSE);
    mempool_free(l);
    l = NULL;

    bsp_spin_destroy(&sqlite_res_list_lock);

    return;
}

// Free a query result
void db_sqlite_free_result(BSP_DB_SQLITE_RES *r)
{
    if (!r)
    {
        return;
    }

    _remove_from_list(r);
    if (r->stmt)
    {
        sqlite3_finalize(r->stmt);
    }
    status_op_db_sqlite(STATUS_OP_DB_SQLITE_FREE);
    mempool_free(r);
    r = NULL;
    
    return;
}

// Query
BSP_DB_SQLITE_RES * db_sqlite_query(BSP_DB_SQLITE *l, const char *query, ssize_t len)
{
    if (!l || !l->conn)
    {
        return NULL;
    }

    if (len < 0)
    {
        len = strlen(query);
    }

    sqlite3_stmt *res = NULL;
    BSP_DB_SQLITE_RES *res_node = NULL;

    bsp_spin_lock(&l->query_lock);
    trace_msg(TRACE_LEVEL_VERBOSE, "SQLite : SQLite query : %s", query);
    int qr = sqlite3_prepare(l->conn, query, len, &res, NULL);
    status_op_db_sqlite(STATUS_OP_DB_SQLITE_QUERY);

    if (SQLITE_OK == qr)
    {
        l->queries ++;
        qr = sqlite3_step(res);
        if (SQLITE_ROW == qr)
        {
            // Storage statement
            sqlite3_reset(res);
            res_node = mempool_alloc(sizeof(BSP_DB_SQLITE_RES));
            res_node->stmt = res;
            res_node->handler = l->conn;
            _add_to_list(res_node);
            status_op_db_sqlite(STATUS_OP_DB_SQLITE_QUERY);
            trace_msg(TRACE_LEVEL_VERBOSE, "SQLite : SQLite query result storaged");
        }

        else
        {
            // Just finalize
            sqlite3_finalize(res);
        }
    }

    else
    {
        status_op_db_sqlite(STATUS_OP_DB_SQLITE_ERROR);
        trace_msg(TRACE_LEVEL_ERROR, "SQLite : SQLite query error : %s", sqlite3_errmsg(l->conn));
    }

    l->query_errno = sqlite3_errcode(l->conn);
    bsp_spin_unlock(&l->query_lock);
    
    return res_node;
}

// SQLite error message
const char * db_sqlite_error(BSP_DB_SQLITE *l)
{
    return (!l || !l->conn) ? "" : sqlite3_errmsg(l->conn);;
}

// Fetch one line from query result
BSP_OBJECT * db_sqlite_fetch_row(BSP_DB_SQLITE_RES *r)
{
    if (!r || !r->stmt)
    {
        return NULL;
    }

    BSP_OBJECT *ret = NULL;
    BSP_OBJECT_ITEM *item;
    int col, cols, bytes;

    int qr = sqlite3_step(r->stmt);
    if (SQLITE_ROW == qr)
    {
        ret = new_object();
        cols = sqlite3_column_count(r->stmt);

        for (col = 0; col < cols; col ++)
        {
            item = new_object_item(sqlite3_column_name(r->stmt, col), -1);
            bytes = sqlite3_column_bytes(r->stmt, col);
            set_item_string(item, (const char *) sqlite3_column_text(r->stmt, col), (ssize_t) bytes);
            object_insert_item(ret, item);
        }
    }
    
    return ret;
}
