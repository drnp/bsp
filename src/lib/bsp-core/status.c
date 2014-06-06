/*
 * status.c
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
 * BSP core status
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/18/2013
 * @chagelog
 *      [07/18/2013] - Creation
 */
#include "bsp.h"

BSP_STATUS s;
BSP_SPINLOCK slock;
struct bsp_status_socket_server_t *sck_s_list = NULL;

/* Initialization */
int status_init()
{
    bsp_spin_init(&slock);
    memset(&s, 0, sizeof(BSP_STATUS));
    s.start_time = time(NULL);
    
    return BSP_RTN_SUCCESS;
}

void status_close()
{
    return;
}

// Status ops
void status_op_core(int op, size_t value)
{
    switch (op)
    {
        case STATUS_OP_INSTANCE_ID : 
            s.instance_id = (int) value;
            break;
        case STATUS_OP_START_TIME : 
            s.start_time = (time_t) value;
            break;
        default : 
            break;
    }
}

void status_op_mempool(int slab_id, int op, size_t value)
{
#ifdef ENABLE_MEMPOOL
    // Huge memory block
    if (STATUS_OP_MEMPOOL_HUGE_ALLOC == op)
    {
        s.mempool.huge_item_alloc_times ++;
        s.mempool.huge_item_alloced += value;
        s.mempool.alloc_times ++;
        s.mempool.dissociative_space += value;
        
        return;
    }
    
    if (STATUS_OP_MEMPOOL_HUGE_FREE == op)
    {
        s.mempool.huge_item_free_times ++;
        s.mempool.huge_item_freed += value;
        s.mempool.free_times ++;
        s.mempool.dissociative_space -= value;
        
        return;
    }
    
    if (slab_id >= SLAB_MAX || slab_id < 0)
    {
        return;
    }
    struct bsp_status_mempool_slab_t *ms = &s.mempool.slab[slab_id];
    switch (op)
    {
        case STATUS_OP_MEMPOOL_ALLOC : 
            ms->alloc_times ++;
            ms->items_inuse ++;
            s.mempool.alloc_times ++;
            s.mempool.items_inuse ++;
            break;
        case STATUS_OP_MEMPOOL_FREE : 
            ms->free_times ++;
            ms->items_inuse --;
            s.mempool.free_times ++;
            s.mempool.items_inuse --;
            break;
        case STATUS_OP_MEMPOOL_BLOCK : 
            ms->block_alloced ++;
            s.mempool.blocks_total ++;
            s.mempool.blocks_space += value;
            break;
        case STATUS_OP_MEMPOOL_ITEM : 
            ms->items_total += value;
            s.mempool.items_total += value;
            break;
        default : 
            break;
    }
#endif
    return;
}

void status_op_fd(int op, size_t value)
{
    switch (op)
    {
        case STATUS_OP_FD_TOTAL : 
            s.fd.fds_total = value;
            break;
        case STATUS_OP_FD_REG : 
            s.fd.reg_times ++;
            break;
        case STATUS_OP_FD_UNREG : 
            s.fd.unreg_times ++;
            break;
        case STATUS_OP_FD_MAX : 
            s.fd.max_used_fd = value;
            break;
        default : 
            break;
    }
    
    return;
}

void status_op_socket(int fd, int op, size_t value)
{
    int i;
    bsp_spin_lock(&slock);
    switch (op)
    {
        case STATUS_OP_SOCKET_SERVER_ADD : 
            // Add a new server object
            s.socket.servers_total ++;
            struct bsp_status_socket_server_t *tmp = realloc(sck_s_list, sizeof(struct bsp_status_socket_server_t) * s.socket.servers_total);
            if (!tmp)
            {
                trigger_exit(BSP_RTN_FATAL, "Cannot alloc memory space for status service !!!");
            }
            sck_s_list = tmp;
            tmp = &sck_s_list[s.socket.servers_total - 1];
            tmp->fd = value;
            tmp->connect_times = 0;
            tmp->disconnect_times = 0;
            tmp->bytes_read = 0;
            tmp->bytes_sent = 0;
            break;
        case STATUS_OP_SOCKET_CONNECTOR_CONNECT : 
            s.socket.connector.connect_times ++;
            break;
        case STATUS_OP_SOCKET_CONNECTOR_DISCONNECT : 
            s.socket.connector.disconnect_times ++;
            break;
        case STATUS_OP_SOCKET_CONNECTOR_READ : 
            s.socket.connector.bytes_read += value;
            break;
        case STATUS_OP_SOCKET_CONNECTOR_SENT : 
            s.socket.connector.bytes_sent += value;
            break;
        default : 
            for (i = 0; i < s.socket.servers_total; i ++)
            {
                tmp = &sck_s_list[i];
                if (fd == tmp->fd)
                {
                    switch (op)
                    {
                        case STATUS_OP_SOCKET_SERVER_CONNECT : 
                            tmp->connect_times ++;
                            break;
                        case STATUS_OP_SOCKET_SERVER_DISCONNECT : 
                            tmp->disconnect_times ++;
                            break;
                        case STATUS_OP_SOCKET_SERVER_READ : 
                            tmp->bytes_read += value;
                            break;
                        case STATUS_OP_SOCKET_SERVER_SENT : 
                            tmp->bytes_sent += value;
                            break;
                        default : 
                            break;
                    }
                    
                    break;
                }
            }
            break;
    }
    bsp_spin_unlock(&slock);
    
    return;
}

void status_op_timer(int op)
{
    bsp_spin_lock(&slock);
    switch (op)
    {
        case STATUS_OP_TIMER_ADD : 
            s.timer.timers_total ++;
            break;
        case STATUS_OP_TIMER_DEL : 
            s.timer.timers_total --;
            break;
        case STATUS_OP_TIMER_TRIGGER : 
            s.timer.trigger_times ++;
            break;
        default : 
            break;
    }
    bsp_spin_unlock(&slock);
    
    return;
}

void status_op_script(int op, size_t value)
{
    bsp_spin_lock(&slock);
    switch (op)
    {
        case STATUS_OP_SCRIPT_STATE_ADD : 
            s.script.states_total ++;
            break;
        case STATUS_OP_SCRIPT_STATE_DEL : 
            s.script.states_total --;
            break;
        case STATUS_OP_SCRIPT_COMPILE : 
            s.script.compile_times ++;
            break;
        case STATUS_OP_SCRIPT_CALL : 
            s.script.call_times ++;
            break;
        case STATUS_OP_SCRIPT_FAILURE : 
            s.script.failure_times ++;
            break;
        case STATUS_OP_SCRIPT_MEMORY_ALLOC : 
            s.script.memory_alloc_times ++;
            s.script.memory_alloced += value;
            break;
        case STATUS_OP_SCRIPT_MEMORY_FREE : 
            s.script.memory_free_times ++;
            s.script.memory_freed += value;
            break;
        default : 
            break;
    }
    bsp_spin_unlock(&slock);
    
    return;
}

void status_op_db_mysql(int op)
{
    bsp_spin_lock(&slock);
    switch (op)
    {
        case STATUS_OP_DB_MYSQL_CONNECT : 
            s.db_mysql.connect_times ++;
            break;
        case STATUS_OP_DB_MYSQL_DISCONNECT : 
            s.db_mysql.disconnect_times ++;
            break;
        case STATUS_OP_DB_MYSQL_QUERY : 
            s.db_mysql.query_times ++;
            break;
        case STATUS_OP_DB_MYSQL_ERROR : 
            s.db_mysql.error_times ++;
            break;
        case STATUS_OP_DB_MYSQL_RESULT : 
            s.db_mysql.results_total ++;
            break;
        case STATUS_OP_DB_MYSQL_FREE : 
            s.db_mysql.free_times ++;
            break;
        default : 
            break;
    }
    bsp_spin_unlock(&slock);
    
    return;
}

void status_op_db_sqlite(int op)
{
    bsp_spin_lock(&slock);
    switch (op)
    {
        case STATUS_OP_DB_SQLITE_OPEN : 
            s.db_sqlite.open_times ++;
            break;
        case STATUS_OP_DB_SQLITE_CLOSE : 
            s.db_sqlite.close_times ++;
            break;
        case STATUS_OP_DB_SQLITE_QUERY : 
            s.db_sqlite.query_times ++;
            break;
        case STATUS_OP_DB_SQLITE_ERROR : 
            s.db_sqlite.error_times ++;
            break;
        case STATUS_OP_DB_SQLITE_RESULT : 
            s.db_sqlite.results_total ++;
            break;
        case STATUS_OP_DB_SQLITE_FREE : 
            s.db_sqlite.free_times ++;
            break;
        default : 
            break;
    }
    bsp_spin_unlock(&slock);
    
    return;
}

void status_op_http(int op)
{
    bsp_spin_lock(&slock);
    switch (op)
    {
        case STATUS_OP_HTTP_REQUEST : 
            s.http.request_times ++;
            break;
        case STATUS_OP_HTTP_RESPONSE : 
            s.http.response_times ++;
            break;
        default : 
            break;
    }
    bsp_spin_unlock(&slock);
    
    return;
}

// Local debug functions
void debug_status()
{
    BSP_STATUS *ds = &s;
    int i;
    
    fprintf(stderr, "\n\033[1;32m          * * * BSP.Core Statistics dump * * *\033[0m\n");
    fprintf(stderr, "=========================================================\n\n");
    
    // Core
    fprintf(stderr, "\033[1;33m  CORE STATUS :\033[0m\n");
    fprintf(stderr, "\033[1;36m    Instance ID                  :\033[0m %d\n"
                    "\033[1;36m    Start time (Unix timestamp)  :\033[0m %d\n", 
            ds->instance_id, 
            (int) ds->start_time);
    fprintf(stderr, "\n");
#ifdef ENABLE_MEMPOOL
    // Mempool
    fprintf(stderr, "\033[1;33m  MEMORY POOL :\033[0m\n");
    fprintf(stderr, "\033[1;34m    SUMMARY :\033[0m\n");
    fprintf(stderr, "\033[1;36m      Huge item alloc times      :\033[0m %llu\n"
                    "\033[1;36m      Huge item alloced          :\033[0m %llu\n"
                    "\033[1;36m      Huge item free times       :\033[0m %llu\n"
                    "\033[1;36m      Huge item freed            :\033[0m %llu\n"
                    "\033[1;36m      Item alloc times           :\033[0m %llu\n"
                    "\033[1;36m      Item free times            :\033[0m %llu\n"
                    "\033[1;36m      Items total                :\033[0m %llu\n"
                    "\033[1;36m      Items in use               :\033[0m %llu\n"
                    "\033[1;36m      Memory blocks total        :\033[0m %llu\n"
                    "\033[1;36m      Block memory space         :\033[0m %llu\n"
                    "\033[1;36m      Dissociative memory space  :\033[0m %llu\n", 
            (long long unsigned int) ds->mempool.huge_item_alloc_times, 
            (long long unsigned int) ds->mempool.huge_item_alloced, 
            (long long unsigned int) ds->mempool.huge_item_free_times, 
            (long long unsigned int) ds->mempool.huge_item_freed, 
            (long long unsigned int) ds->mempool.alloc_times, 
            (long long unsigned int) ds->mempool.free_times, 
            (long long unsigned int) ds->mempool.items_total, 
            (long long unsigned int) ds->mempool.items_inuse, 
            (long long unsigned int) ds->mempool.blocks_total, 
            (long long unsigned int) ds->mempool.blocks_space, 
            (long long unsigned int) ds->mempool.dissociative_space);
    fprintf(stderr, "\033[1;34m    SLABS :\033[0m\n");
    for (i = 0; i < SLAB_MAX; i ++)
    {
        fprintf(stderr, "\033[1;31m      Slab %2d ->\033[0m\n", i);
        fprintf(stderr, "\033[1;36m        Memory block alloced     :\033[0m %llu\n"
                        "\033[1;36m        Item alloc times         :\033[0m %llu\n"
                        "\033[1;36m        Item free times          :\033[0m %llu\n"
                        "\033[1;36m        Items total              :\033[0m %llu\n"
                        "\033[1;36m        Itmes in use             :\033[0m %llu\n", 
                (long long unsigned int) ds->mempool.slab[i].block_alloced, 
                (long long unsigned int) ds->mempool.slab[i].alloc_times, 
                (long long unsigned int) ds->mempool.slab[i].free_times, 
                (long long unsigned int) ds->mempool.slab[i].items_total, 
                (long long unsigned int) ds->mempool.slab[i].items_inuse);
    }
    fprintf(stderr, "\n");
#endif
    // Fds
    fprintf(stderr, "\033[1;33m  FILE DESCRIPTOR :\033[0m\n");
    fprintf(stderr, "\033[1;36m    Total fds                    :\033[0m %llu\n"
                    "\033[1;36m    Register times               :\033[0m %llu\n"
                    "\033[1;36m    Unregister times             :\033[0m %llu\n"
                    "\033[1;36m    Maximal used fd              :\033[0m %llu\n", 
            (long long unsigned int) ds->fd.fds_total, 
            (long long unsigned int) ds->fd.reg_times, 
            (long long unsigned int) ds->fd.unreg_times, 
            (long long unsigned int) ds->fd.max_used_fd);
    fprintf(stderr, "\n");
    
    // Socket
    fprintf(stderr, "\033[1;33m  SOCKET :\033[0m\n");
    fprintf(stderr, "\033[1;34m    SUMMARY :\033[0m\n");
    fprintf(stderr, "\033[1;36m      Servers total              :\033[0m %llu\n", 
            (long long unsigned int) ds->socket.servers_total);
    fprintf(stderr, "\033[1;34m    SOCKET SERVER :\033[0m\n");
    for (i = 0; i < ds->socket.servers_total; i ++)
    {
        fprintf(stderr, "\033[1;31m      Server %2d ->\033[0m\n", i);
        fprintf(stderr, "\033[1;36m        FD                       :\033[0m %llu\n"
                        "\033[1;36m        Connect times            :\033[0m %llu\n"
                        "\033[1;36m        Disconnect times         :\033[0m %llu\n"
                        "\033[1;36m        Bytes read               :\033[0m %llu\n"
                        "\033[1;36m        Bytes sent               :\033[0m %llu\n", 
                (long long unsigned int) sck_s_list[i].fd, 
                (long long unsigned int) sck_s_list[i].connect_times, 
                (long long unsigned int) sck_s_list[i].disconnect_times, 
                (long long unsigned int) sck_s_list[i].bytes_read, 
                (long long unsigned int) sck_s_list[i].bytes_sent);
    }
    fprintf(stderr, "\033[1;34m    SOCKET CONNECTOR :\033[0m\n");
    fprintf(stderr, "\033[1;36m      Connect times              :\033[0m %llu\n"
                    "\033[1;36m      Disconnect times           :\033[0m %llu\n"
                    "\033[1;36m      Bytes read                 :\033[0m %llu\n"
                    "\033[1;36m      Bytes sent                 :\033[0m %llu\n", 
            (long long unsigned int) ds->socket.connector.connect_times, 
            (long long unsigned int) ds->socket.connector.disconnect_times, 
            (long long unsigned int) ds->socket.connector.bytes_read, 
            (long long unsigned int) ds->socket.connector.bytes_sent);
    fprintf(stderr, "\n");
    
    // Timer
    fprintf(stderr, "\033[1;33m  TIMER :\033[0m\n");
    fprintf(stderr, "\033[1;36m    Timers total                 :\033[0m %llu\n"
                    "\033[1;36m    Trigger times                :\033[0m %llu\n", 
            (long long unsigned int) ds->timer.timers_total, 
            (long long unsigned int) ds->timer.trigger_times);
    fprintf(stderr, "\n");
    
    // Script
    fprintf(stderr, "\033[1;33m  SCRIPT :\033[0m\n");
    fprintf(stderr, "\033[1;36m    States total                 :\033[0m %llu\n"
                    "\033[1;36m    Compile times                :\033[0m %llu\n"
                    "\033[1;36m    Call times                   :\033[0m %llu\n"
                    "\033[1;36m    Failure times                :\033[0m %llu\n"
                    "\033[1;36m    Memory alloc times           :\033[0m %llu\n"
                    "\033[1;36m    Memory alloced               :\033[0m %llu\n"
                    "\033[1;36m    Memory free times            :\033[0m %llu\n"
                    "\033[1;36m    Memory freed                 :\033[0m %llu\n", 
            (long long unsigned int) ds->script.states_total, 
            (long long unsigned int) ds->script.compile_times, 
            (long long unsigned int) ds->script.call_times, 
            (long long unsigned int) ds->script.failure_times, 
            (long long unsigned int) ds->script.memory_alloc_times, 
            (long long unsigned int) ds->script.memory_alloced, 
            (long long unsigned int) ds->script.memory_free_times, 
            (long long unsigned int) ds->script.memory_freed);
    fprintf(stderr, "\n");
    
    // MySQL
    fprintf(stderr, "\033[1;33m  DB::MYSQL :\033[0m\n");
    fprintf(stderr, "\033[1;36m    Connect times                :\033[0m %llu\n"
                    "\033[1;36m    Disconnect times             :\033[0m %llu\n"
                    "\033[1;36m    Query times                  :\033[0m %llu\n"
                    "\033[1;36m    Error times                  :\033[0m %llu\n"
                    "\033[1;36m    Results total                :\033[0m %llu\n"
                    "\033[1;36m    Free result times            :\033[0m %llu\n", 
            (long long unsigned int) ds->db_mysql.connect_times, 
            (long long unsigned int) ds->db_mysql.disconnect_times, 
            (long long unsigned int) ds->db_mysql.query_times, 
            (long long unsigned int) ds->db_mysql.error_times, 
            (long long unsigned int) ds->db_mysql.results_total, 
            (long long unsigned int) ds->db_mysql.free_times);
    fprintf(stderr, "\n");
    
    // SQLite
    fprintf(stderr, "\033[1;33m  DB::SQLITE :\033[0m\n");
    fprintf(stderr, "\033[1;36m    Open times                   :\033[0m %llu\n"
                    "\033[1;36m    Close times                  :\033[0m %llu\n"
                    "\033[1;36m    Query times                  :\033[0m %llu\n"
                    "\033[1;36m    Error times                  :\033[0m %llu\n"
                    "\033[1;36m    Results total                :\033[0m %llu\n"
                    "\033[1;36m    Free result times            :\033[0m %llu\n", 
            (long long unsigned int) ds->db_sqlite.open_times, 
            (long long unsigned int) ds->db_sqlite.close_times, 
            (long long unsigned int) ds->db_sqlite.query_times, 
            (long long unsigned int) ds->db_sqlite.error_times, 
            (long long unsigned int) ds->db_sqlite.results_total, 
            (long long unsigned int) ds->db_sqlite.free_times);
    fprintf(stderr, "\n");
    
    // HTTP
    fprintf(stderr, "\033[1;33m  HTTP OPERATES :\033[0m\n");
    fprintf(stderr, "\033[1;36m    Request times                :\033[0m %llu\n"
                    "\033[1;36m    Response times               :\033[0m %llu\n", 
            (long long unsigned int) ds->http.request_times, 
            (long long unsigned int) ds->http.response_times);
    fprintf(stderr, "\n");
    
    fprintf(stderr, "\n");
    
    return;
}
