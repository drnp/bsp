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
    trace_msg(TRACE_LEVEL_DEBUG, "Mongo  : Try to close MongoDB connection");
    mongo_sync_disconnect(m->conn);
    bsp_spin_unlock(&m->query_lock);
    bsp_spin_destroy(&m->query_lock);
    bsp_free(m);

    return;
}

// Command : Insert
int db_mongodb_insert(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *obj)
{
    if (!m || !m->conn || !namespace)
    {
        return BSP_RTN_ERROR_DB;
    }

    bson *doc = NULL;
    bsp_spin_lock(&m->query_lock);
    int ret = mongo_sync_cmd_insert(m->conn, namespace, doc, NULL);
    if (!ret)
    {
        // Insert error
        trace_msg(TRACE_LEVEL_ERROR, "Mongo  : Insert document error");
    }
    else
    {
        trace_msg(TRACE_LEVEL_VERBOSE, "Mongo  : Insert a document");
    }
    bsp_spin_unlock(&m->query_lock);

    return BSP_RTN_SUCCESS;
}

// Command : query
BSP_OBJECT * db_mongodb_query(BSP_DB_MONGODB *m, const char *namespace, const char *key)
{
    if (!m || !m->conn || !namespace)
    {
        return NULL;
    }

    BSP_OBJECT *ret = NULL;

    return ret;
}
