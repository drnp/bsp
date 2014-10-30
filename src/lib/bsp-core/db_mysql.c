/*
 * db_mysql.c
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
 * MySQL database extension
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/31/2012
 * @changelog 
 *      [08/31/2012] - Creation
 *      [05/22/2014] - Remove result list
 */

#include "bsp.h"

// Make a new MySQL connection
BSP_DB_MYSQL * db_mysql_connect(
                                const char *host, 
                                const char *user, 
                                const char *pass, 
                                const char *db)
{
    const char *conn_host = NULL;
    const char *conn_sock = NULL;
    const char *conn_user = user ? user : "";
    const char *conn_pass = pass ? pass : NULL;
    const char *conn_db = db ? db : NULL;

    FILE *tfp = fopen(host, "r");
    if (tfp)
    {
        conn_sock = host;
        fclose(tfp);
        trace_msg(TRACE_LEVEL_NOTICE, "MySQL  : Try to connect to MySQL server through local sock <%s>", host);
    }
    else
    {
        conn_host = host;
        trace_msg(TRACE_LEVEL_NOTICE, "MySQL  : Try to connect to MySQL server <%s>", host);
    }

    MYSQL *conn = mysql_init(NULL);
    if (!conn)
    {
        return NULL;
    }

    if (!mysql_real_connect(conn, conn_host, conn_user, conn_pass, conn_db, 0, conn_sock, 0))
    {
        trace_msg(TRACE_LEVEL_ERROR, "MySQL  : MySQL connection failed");
        mysql_close(conn);
        return NULL;
    }

    BSP_DB_MYSQL *m = bsp_malloc(sizeof(BSP_DB_MYSQL));
    if (!m)
    {
        mysql_close(conn);
        return NULL;
    }

    m->conn = conn;
    m->queries = 0;
    m->query_errno = 0;
    m->result_list = NULL;
    bsp_spin_init(&m->query_lock);
    reg_fd(m->conn->net.fd, FD_TYPE_SOCKET_MYSQL, NULL);
    my_bool my_true = 0x1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &my_true);
    trace_msg(TRACE_LEVEL_VERBOSE, "MySQL  : Set MySQL connection auto-reconnect");

    // Default charset
    mysql_query(conn, "SET NAMES 'utf8'");
    trace_msg(TRACE_LEVEL_VERBOSE, "MySQL  : Set MySQL default charset as UTF-8");
    if (conn_db)
    {
        mysql_select_db(conn, conn_db);
    }

    status_op_db_mysql(STATUS_OP_DB_MYSQL_CONNECT);

    return m;
}

// Close a MySQL connection and free all query results
void db_mysql_close(BSP_DB_MYSQL *m)
{
    if (!m)
    {
        return;
    }

    // Clear results
    bsp_spin_lock(&m->query_lock);
    BSP_DB_MYSQL_RES *curr = m->result_list, *next = NULL;
    while (curr)
    {
        next = curr->next;
        if (curr->res)
        {
            mysql_free_result(curr->res);
            status_op_db_mysql(STATUS_OP_DB_MYSQL_FREE);
        }

        bsp_free(curr);
        curr = next;
    }

    trace_msg(TRACE_LEVEL_NOTICE, "MySQL  : Try to close MySQL connection");
    unreg_fd(m->conn->net.fd);
    mysql_close(m->conn);
    status_op_db_mysql(STATUS_OP_DB_MYSQL_DISCONNECT);
    bsp_spin_unlock(&m->query_lock);
    bsp_spin_destroy(&m->query_lock);
    bsp_free(m);

    return;
}

// Free a query result
void db_mysql_free_result(BSP_DB_MYSQL_RES *r)
{
    if (!r)
    {
        return;
    }

    if (r->res)
    {
        mysql_free_result(r->res);
        status_op_db_mysql(STATUS_OP_DB_MYSQL_FREE);
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

// Query a string, if no result available, null pointor returned
BSP_DB_MYSQL_RES * db_mysql_query(BSP_DB_MYSQL *m, const char *query, ssize_t len)
{
    if (!m || !query)
    {
        return 0;
    }

    if (len < 0)
    {
        len = strlen(query);
    }

    MYSQL_RES *res = NULL;
    BSP_DB_MYSQL_RES *res_node = NULL;
    
    bsp_spin_lock(&m->query_lock);
    trace_msg(TRACE_LEVEL_VERBOSE, "MySQL  : MySQL query : %s", query);
    int qr = mysql_real_query(m->conn, query, len);
    status_op_db_mysql(STATUS_OP_DB_MYSQL_QUERY);

    if (0 == qr)
    {
        m->queries ++;
        res = mysql_store_result(m->conn);
        if (res)
        {
            // Store result
            res_node = bsp_malloc(sizeof(BSP_DB_MYSQL_RES));
            if (res_node)
            {
                res_node->res = res;
                res_node->handler = m;
                res_node->num_rows = mysql_num_rows(res);
                res_node->next = m->result_list;
                res_node->prev = NULL;
                if (m->result_list)
                {
                    m->result_list->prev = res_node;
                }
                m->result_list = res_node;
                status_op_db_mysql(STATUS_OP_DB_MYSQL_RESULT);
                trace_msg(TRACE_LEVEL_VERBOSE, "MySQL  : MySQL query result storaged");
            }
        }
    }

    else
    {
        status_op_db_mysql(STATUS_OP_DB_MYSQL_ERROR);
        trace_msg(TRACE_LEVEL_ERROR, "MySQL  : MySQL query error : %s", db_mysql_error(m));
    }
    
    bsp_spin_unlock(&m->query_lock);
    m->query_errno = mysql_errno(m->conn);

    return res_node;
}

// Mysql error message
const char * db_mysql_error(BSP_DB_MYSQL *m)
{
    return (!m || !m->conn) ? "" : mysql_error(m->conn);
}

// Fetch one line from query result
BSP_OBJECT * db_mysql_fetch_row(BSP_DB_MYSQL_RES *r)
{
    if (!r || !r->res)
    {
        return NULL;
    }

    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    unsigned long *lengths;
    unsigned int num_fields, i;
    
    row = mysql_fetch_row(r->res);
    if (!row)
    {
        // Maybe empty
        return NULL;
    }

    fields = mysql_fetch_fields(r->res);
    lengths = mysql_fetch_lengths(r->res);
    num_fields = mysql_num_fields(r->res);
    
    BSP_OBJECT *ret = NULL;
    BSP_VALUE *item = NULL;

    if (fields && lengths)
    {
        ret = new_object(OBJECT_TYPE_HASH);
        for (i = 0; i < num_fields; i ++)
        {
            item = new_value();
            value_set_string(item, new_string((const char *) row[i], lengths[i]));
            object_set_hash(ret, new_string((const char *) fields[i].name, -1), item);
        }
    }

    return ret;
}
