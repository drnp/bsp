/*
 * memdb.c
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
 * Object based in-memory db / cache
 * Used for runtime cache
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/22/2014
 * @changelog 
 *      [10/22/2014] - Creation
 */

#include "bsp.h"

BSP_OBJECT *memdb_base = NULL;

int memdb_init()
{
    memdb_base = new_object(OBJECT_TYPE_HASH);
    if (!memdb_base)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "Create memdb base object error");
    }

    trace_msg(TRACE_LEVEL_DEBUG, "MemDB  : Base object initialized");

    return BSP_RTN_SUCCESS;
}

BSP_VALUE *memdb_get(const char *key)
{
    BSP_VALUE *ret = object_get_value(memdb_base, key);

    return ret;
}
