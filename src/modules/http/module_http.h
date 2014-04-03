/*
 * module_http.h
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
 * HTTP request producer
 * 
 * @package modules::http
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/23/2012
 * @changelog 
 *      [08/23/2012] - Creation
 */

#ifndef _MODULE_HTTP_H
#define _MODULE_HTTP_H
/* Headers */
#include "bsp_http.h"
#include "lua.h"

/* Definations */

/* Macros */

/* Structs */
struct _http_callback_additional_t
{
    BSP_HTTP_RESPONSE   *resp;
    lua_State           *caller;
};

/* Functions */
int bsp_module_http(lua_State *s);

#endif  /* _MODULE_HTTP_H */
