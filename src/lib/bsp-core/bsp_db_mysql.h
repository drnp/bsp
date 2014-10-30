/*
 * bsp_db_mysql.h
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
 * MySQL database extension header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/31/2012
 * @changelog 
 *      [08/31/2012] - Creation
 */
#ifndef _LIB_BSP_CORE_DB_MYSQL_H
#define _LIB_BSP_CORE_DB_MYSQL_H
/* Headers */
#include <mysql.h>

/* Definations */
#define QUERY_RES_LIST_INITIAL                  1024

/* Macros */

/* Structs */
typedef struct bsp_db_mysql_t
{
    MYSQL               *conn;
    size_t              queries;
    unsigned int        query_errno;
    BSP_SPINLOCK        query_lock;
    struct bsp_db_mysql_res_t
                        *result_list;
} BSP_DB_MYSQL;

typedef struct bsp_db_mysql_res_t
{
    MYSQL_RES           *res;
    my_ulonglong        num_rows;
    struct bsp_db_mysql_t
                        *handler;
    struct bsp_db_mysql_res_t
                        *prev;
    struct bsp_db_mysql_res_t
                        *next;
} BSP_DB_MYSQL_RES;

/* Functions */
BSP_DB_MYSQL * db_mysql_connect(const char *host, const char *user, const char *pass, const char *db);
void db_mysql_close(BSP_DB_MYSQL *m);
void db_mysql_free_result(BSP_DB_MYSQL_RES *r);
BSP_DB_MYSQL_RES * db_mysql_query(BSP_DB_MYSQL *m, const char *query, ssize_t len);
const char * db_mysql_error(BSP_DB_MYSQL *m);
BSP_OBJECT * db_mysql_fetch_row(BSP_DB_MYSQL_RES *r);

#endif  /* _LIB_BSP_CORE_DB_MYSQL_H */
