/*
 * bsp_db_sqlite.h
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
 * SQLite3 handler header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 09/12/2012
 * @changelog 
 *      [09/12/2012] - Creation
 */
#ifndef _LIB_BSP_CORE_DB_SQLITE_H
#define _LIB_BSP_CORE_DB_SQLITE_H
/* Headers */
#include "sqlite3.h"

/* Definations */

/* Macros */

/* Structs */
typedef struct bsp_db_sqlite_t
{
    sqlite3             *conn;
    size_t              queries;
    int                 query_errno;
    BSP_SPINLOCK        query_lock;
} BSP_DB_SQLITE;

typedef struct bsp_db_sqlite_res_t
{
    sqlite3_stmt        *stmt;
    sqlite3             *handler;
    struct bsp_db_sqlite_res_t
                        *prev;
    struct bsp_db_sqlite_res_t
                        *next;
} BSP_DB_SQLITE_RES;

/* Functions */
BSP_DB_SQLITE * db_sqlite_open(const char *dbfile);
void db_sqlite_close(BSP_DB_SQLITE *l);
void db_sqlite_free_result(BSP_DB_SQLITE_RES *r);
BSP_DB_SQLITE_RES * db_sqlite_query(BSP_DB_SQLITE *l, const char *query, ssize_t len);
const char * db_sqlite_error(BSP_DB_SQLITE *l);
BSP_OBJECT * db_sqlite_fetch_row(BSP_DB_SQLITE_RES *r);

#endif  /* _LIB_BSP_CORE_DB_SQLITE_H */
