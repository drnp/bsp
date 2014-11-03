/*
 * db_mongodb.c
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
 * MongoDB extension
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/30/2014
 * @changelog 
 *      [10/30/2014] - Creation
 */

#include "bsp.h"

// Connect to server
BSP_DB_MONGODB * db_mongodb_connect(
                                    const char *host, 
                                    int port)
{
    mongo_sync_connection *conn = NULL;
    const char *conn_host = NULL;
    const char *conn_sock = NULL;

    FILE *tfp = fopen(host, "r");
    if (tfp)
    {
        conn_sock = host;
        fclose(tfp);
        trace_msg(TRACE_LEVEL_NOTICE, "Mongo  : Try to connect to MongoDB server through local sock <%s>", host);
        conn = mongo_sync_connect(conn_sock, MONGO_CONN_LOCAL, TRUE);
    }
    else
    {
        conn_host = host;
        trace_msg(TRACE_LEVEL_NOTICE, "Mongo  : Try to connect to MongoDB server <%s>", host);
        conn = mongo_sync_connect(conn_host, port, TRUE);
    }

    if (!conn)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Mongo  : MongoDB connection to <%s> error", host);
        return NULL;
    }

    mongo_sync_conn_set_auto_reconnect(conn, TRUE);
    BSP_DB_MONGODB *ret = bsp_calloc(1, sizeof(BSP_DB_MONGODB));
    ret->conn = conn;
    bsp_spin_init(&ret->query_lock);

    status_op_db_mongodb(STATUS_OP_DB_MONGODB_CONNECT);

    return ret;
}

// Close and free connection
void db_mongodb_close(BSP_DB_MONGODB *m)
{
    if (!m)
    {
        return;
    }

    bsp_spin_lock(&m->query_lock);
    // Clear results
    BSP_DB_MONGODB_RES *curr = m->result_list, *next = NULL;
    while (curr)
    {
        next = curr->next;
        if (curr->packet)
        {
            mongo_wire_packet_free(curr->packet);
        }

        if (curr->cursor)
        {
            mongo_sync_cursor_free(curr->cursor);
        }

        bsp_free(curr);
        curr = next;
    }
    trace_msg(TRACE_LEVEL_DEBUG, "Mongo  : Try to close MongoDB connection");
    mongo_sync_disconnect(m->conn);
    status_op_db_mongodb(STATUS_OP_DB_MONGODB_DISCONNECT);
    bsp_spin_unlock(&m->query_lock);
    bsp_spin_destroy(&m->query_lock);
    bsp_free(m);

    return;
}

// Free query result
void db_mongodb_free_result(BSP_DB_MONGODB_RES *r)
{
    if (!r)
    {
        return;
    }

    if (r->packet)
    {
        mongo_wire_packet_free(r->packet);
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_FREE);
    }

    if (r->cursor)
    {
        mongo_sync_cursor_free(r->cursor);
    }

    if (r->next)
    {
        r->next->prev = r->prev;
    }

    if (r->prev)
    {
        r->prev->next = r->next;
    }
    else
    {
        // Head
        r->handler->result_list = r->next;
    }
    bsp_free(r);
    r = NULL;

    return;
}

/* Bson converter */
#include <libmongo-private.h>
static bson * _obj_to_bson(BSP_OBJECT *obj)
{
    if (!obj || OBJECT_TYPE_HASH != obj->type)
    {
        return NULL;
    }
    bson *ret = bsp_malloc(sizeof(bson));
    BSP_STRING *b = bson_nd_encode(obj);
    ret->data = g_byte_array_new();
    g_byte_array_append(ret->data, (const guint8 *) STR_STR(b), STR_LEN(b));
    ret->finished = TRUE;
    del_string(b);

    return ret;
}

static BSP_OBJECT * _bson_to_obj(bson *bsn)
{
    if (!bsn || !bsn->finished)
    {
        return NULL;
    }
    BSP_STRING *b = new_string_const((const char *) bsn->data->data, bsn->data->len);
    BSP_OBJECT *ret = bson_nd_decode(b);
    del_string(b);

    return ret;
}

// Command : Update
int db_mongodb_update(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *obj, BSP_OBJECT *query)
{
    if (!m || !m->conn || !namespace || !obj || !query)
    {
        return BSP_RTN_ERROR_DB;
    }

    bson *doc = _obj_to_bson(obj);
    bson *sel = _obj_to_bson(query);
    bsp_spin_lock(&m->query_lock);
    int ret = mongo_sync_cmd_update(m->conn, namespace, MONGO_WIRE_FLAG_UPDATE_UPSERT, doc, sel);
    if (!ret)
    {
        // Insert error
        trace_msg(TRACE_LEVEL_ERROR, "Mongo  : Insert document error");
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_ERROR);
    }
    else
    {
        trace_msg(TRACE_LEVEL_VERBOSE, "Mongo  : Insert a document");
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_UPDATE);
    }
    bsp_spin_unlock(&m->query_lock);

    return BSP_RTN_SUCCESS;
}

// Command : delete
int db_mongodb_delete(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *query)
{
    if (!m || !m->conn || !namespace || !query)
    {
        return BSP_RTN_ERROR_DB;
    }

    bson *sel = _obj_to_bson(query);
    bsp_spin_lock(&m->query_lock);
    if (mongo_sync_cmd_delete(m->conn, namespace, 0, sel))
    {
        trace_msg(TRACE_LEVEL_VERBOSE, "Mongo  : Delete documents");
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_DELETE);
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Mongo  : Delete docuemnts error");
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_ERROR);
    }
    bsp_spin_unlock(&m->query_lock);

    return BSP_RTN_SUCCESS;
}

// Command : query
BSP_DB_MONGODB_RES * db_mongodb_query(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *query)
{
    if (!m || !m->conn || !namespace || !query)
    {
        return NULL;
    }

    mongo_packet *p = NULL;
    BSP_DB_MONGODB_RES *res_node = NULL;
    bson *sel = _obj_to_bson(query);
    bsp_spin_lock(&m->query_lock);
    p = mongo_sync_cmd_query(m->conn, namespace, 0, 0, MONGODB_QUERY_DOCUMENTS_MAX, sel, NULL);
    if (p)
    {
        res_node = bsp_malloc(sizeof(BSP_DB_MONGODB_RES));
        res_node->packet = p;
        res_node->cursor = NULL;
        res_node->ns = namespace;
        res_node->handler = m;
        res_node->next = m->result_list;
        res_node->prev = NULL;
        if (m->result_list)
        {
            m->result_list->prev = res_node;
        }
        m->result_list = res_node;
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_RESULT);
        trace_msg(TRACE_LEVEL_VERBOSE, "Mongo  : MongoDB query result stored");
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Mongo  : MongoDB query failed");
        status_op_db_mongodb(STATUS_OP_DB_MONGODB_ERROR);
    }

    bsp_spin_unlock(&m->query_lock);
    bson_free(sel);

    return res_node;
}

// Get error message
const char * db_mongodb_error(BSP_DB_MONGODB *m)
{
    return (!m || !m->conn) ? "" : (const char *) mongo_sync_conn_get_last_error(m->conn);
}

// Fetch one line from mongodb query result
BSP_OBJECT * db_mongodb_fetch_doc(BSP_DB_MONGODB_RES *r)
{
    if (!r || !r->packet || !r->ns || !r->handler)
    {
        return NULL;
    }

    BSP_OBJECT *ret = NULL;
    mongo_sync_cursor *c = r->cursor;
    if (!c)
    {
        c = mongo_sync_cursor_new(r->handler->conn, r->ns, r->packet);
        r->cursor = c;
    }

    if (c)
    {
        bson *data = mongo_sync_cursor_get_data(c);
        if (data)
        {
            ret = _bson_to_obj(data);
            mongo_sync_cursor_next(c);
        }
    }

    return ret;
}
