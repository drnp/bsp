/*
 * bsp_status.h
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
 * BSP core status header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/18/2013
 * @chagelog
 *      [07/18/2013] - Creation
 */

#ifndef _LIB_BSP_CORE_STATUS_H

#define _LIB_BSP_CORE_STATUS_H
/* Headers */

/* Definations */
#define STATUS_OP_APPID                         0x0001
#define STATUS_OP_START_TIME                    0x0002

#define STATUS_OP_MEMPOOL_ALLOC                 0x1001
#define STATUS_OP_MEMPOOL_FREE                  0x1002
#define STATUS_OP_MEMPOOL_HUGE_ALLOC            0x1003
#define STATUS_OP_MEMPOOL_HUGE_FREE             0x1004
#define STATUS_OP_MEMPOOL_BLOCK                 0x1011
#define STATUS_OP_MEMPOOL_ITEM                  0x1012

#define STATUS_OP_FD_TOTAL                      0x1101
#define STATUS_OP_FD_REG                        0x1102
#define STATUS_OP_FD_UNREG                      0x1103
#define STATUS_OP_FD_MAX                        0x1104

#define STATUS_OP_SOCKET_SERVER_ADD             0x1201
#define STATUS_OP_SOCKET_SERVER_CONNECT         0x1202
#define STATUS_OP_SOCKET_SERVER_DISCONNECT      0x1203
#define STATUS_OP_SOCKET_SERVER_READ            0x1204
#define STATUS_OP_SOCKET_SERVER_SENT            0x1205
#define STATUS_OP_SOCKET_CONNECTOR_CONNECT      0x1211
#define STATUS_OP_SOCKET_CONNECTOR_DISCONNECT   0x1212
#define STATUS_OP_SOCKET_CONNECTOR_READ         0x1213
#define STATUS_OP_SOCKET_CONNECTOR_SENT         0x1214

#define STATUS_OP_TIMER_ADD                     0x1301
#define STATUS_OP_TIMER_DEL                     0x1302
#define STATUS_OP_TIMER_TRIGGER                 0x1311

#define STATUS_OP_SCRIPT_STATE_ADD              0x1401
#define STATUS_OP_SCRIPT_STATE_DEL              0x1402
#define STATUS_OP_SCRIPT_MEMORY_ALLOC           0x1403
#define STATUS_OP_SCRIPT_MEMORY_FREE            0x1404
#define STATUS_OP_SCRIPT_COMPILE                0x1411
#define STATUS_OP_SCRIPT_CALL                   0x1412
#define STATUS_OP_SCRIPT_FAILURE                0x1413

#define STATUS_OP_DB_MYSQL_CONNECT              0x1501
#define STATUS_OP_DB_MYSQL_DISCONNECT           0x1502
#define STATUS_OP_DB_MYSQL_QUERY                0x1511
#define STATUS_OP_DB_MYSQL_ERROR                0x1512
#define STATUS_OP_DB_MYSQL_RESULT               0x1513
#define STATUS_OP_DB_MYSQL_FREE                 0x1514

#define STATUS_OP_DB_SQLITE_OPEN                0x1601
#define STATUS_OP_DB_SQLITE_CLOSE               0x1602
#define STATUS_OP_DB_SQLITE_QUERY               0x1611
#define STATUS_OP_DB_SQLITE_ERROR               0x1612
#define STATUS_OP_DB_SQLITE_RESULT              0x1613
#define STATUS_OP_DB_SQLITE_FREE                0x1614

#define STATUS_OP_HTTP_REQUEST                  0x1701
#define STATUS_OP_HTTP_RESPONSE                 0x1702

/* Macros */

/* Structs */
struct bsp_status_mempool_slab_t
{
    size_t              block_alloced;
    size_t              alloc_times;
    size_t              free_times;
    size_t              items_total;
    size_t              items_inuse;
};

struct bsp_status_mempool_t
{
    struct bsp_status_mempool_slab_t
                        slab[SLAB_MAX];
    size_t              huge_item_alloc_times;
    size_t              huge_item_free_times;
    size_t              huge_item_alloced;
    size_t              huge_item_freed;
    size_t              alloc_times;
    size_t              free_times;
    size_t              items_total;
    size_t              items_inuse;
    size_t              blocks_total;
    size_t              blocks_space;
    size_t              dissociative_space;
};

struct bsp_status_fd_t
{
    size_t              fds_total;
    size_t              reg_times;
    size_t              unreg_times;
    size_t              max_used_fd;
};

struct bsp_status_socket_server_t
{
    size_t              fd;
    size_t              connect_times;
    size_t              disconnect_times;
    size_t              bytes_read;
    size_t              bytes_sent;
};

struct bsp_status_socket_connector_t
{
    size_t              connect_times;
    size_t              disconnect_times;
    size_t              bytes_read;
    size_t              bytes_sent;
};

struct bsp_status_socket_t
{
    size_t              servers_total;
    struct bsp_status_socket_connector_t
                        connector;
};

struct bsp_status_timer_t
{
    size_t              timers_total;
    size_t              trigger_times;
};

struct bsp_status_script_t
{
    size_t              states_total;
    size_t              compile_times;
    size_t              call_times;
    size_t              failure_times;
    size_t              memory_alloc_times;
    size_t              memory_alloced;
    size_t              memory_free_times;
    size_t              memory_freed;
};

struct bsp_status_db_mysql_t
{
    size_t              connect_times;
    size_t              disconnect_times;
    size_t              query_times;
    size_t              error_times;
    size_t              results_total;
    size_t              free_times;
};

struct bsp_status_db_sqlite_t
{
    size_t              open_times;
    size_t              close_times;
    size_t              query_times;
    size_t              error_times;
    size_t              results_total;
    size_t              free_times;
};

struct bsp_status_http_t
{
    size_t              request_times;
    size_t              response_times;
};

typedef struct bsp_status_t
{
    int                 app_id;
    time_t              start_time;
    struct bsp_status_mempool_t
                        mempool;
    struct bsp_status_fd_t
                        fd;
    struct bsp_status_socket_t
                        socket;
    struct bsp_status_timer_t
                        timer;
    struct bsp_status_script_t
                        script;
    struct bsp_status_db_mysql_t
                        db_mysql;
    struct bsp_status_db_sqlite_t
                        db_sqlite;
    struct bsp_status_http_t
                        http;
} BSP_STATUS;

/* Functions */
int status_init();
void status_close();

// Ops
void status_op_core(int op, size_t value);
void status_op_mempool(int slab_id, int op, size_t value);
void status_op_fd(int op, size_t value);
void status_op_socket(int fd, int op, size_t value);
void status_op_timer(int op);
void status_op_script(int op, size_t value);
void status_op_db_mysql(int op);
void status_op_db_sqlite(int op);
void status_op_http(int op);

// Debug apis
void debug_status();

#endif  /* _LIB_BSP_CORE_STATUS_H */
