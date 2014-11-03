/*
 * bsp_db_mongodb.h
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
 * MongoDB extension header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/30/2014
 * @changelog 
 *      [10/30/2014] - Creation
 */

#ifndef _LIB_BSP_CORE_DB_MONGODB_H
#define _LIB_BSP_CORE_DB_MONGODB_H
/* Headers */
#include "mongo.h"

/* Definations */
#define MONGODB_QUERY_DOCUMENTS_MAX             1024

/* Macros */

/* Structs */
typedef struct bsp_db_mongodb_t
{
    mongo_sync_connection
                        *conn;
    size_t              queries;
    BSP_SPINLOCK        query_lock;
    struct bsp_db_mongodb_res_t
                        *result_list;
} BSP_DB_MONGODB;

typedef struct bsp_db_mongodb_res_t
{
    mongo_packet        *packet;
    mongo_sync_cursor   *cursor;
    const char          *ns;
    struct bsp_db_mongodb_t
                        *handler;
    struct bsp_db_mongodb_res_t
                        *prev;
    struct bsp_db_mongodb_res_t
                        *next;
} BSP_DB_MONGODB_RES;

/* Functions */
BSP_DB_MONGODB * db_mongodb_connect(const char *host, int port);
void db_mongodb_close(BSP_DB_MONGODB *m);
void db_mongodb_free_result(BSP_DB_MONGODB_RES *r);
int db_mongodb_update(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *obj, BSP_OBJECT *query);
int db_mongodb_delete(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *query);
BSP_DB_MONGODB_RES * db_mongodb_query(BSP_DB_MONGODB *m, const char *namespace, BSP_OBJECT *query);
const char * db_mongodb_error(BSP_DB_MONGODB *m);
BSP_OBJECT * db_mongodb_fetch_doc(BSP_DB_MONGODB_RES *r);

#endif  /* _LIB_BSP_CORE_DB_MONGODB_H */
