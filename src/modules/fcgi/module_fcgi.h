/*
 * module_fcgi.h
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
 * FastCGI implementation header
 * 
 * @package modules::fcgi
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/30/2014
 * @changelog
 *      [06/30/2014] - Creation
 */

#ifndef _MODULE_FCGI_H
#define _MODULE_FCGI_H
/* Header */
#include "lua.h"

/* Definations */

/* Macros */

/* Structs */

/* Functions */
int bsp_module_fcgi(lua_State *s);

#endif  /* _MODULE_FCGI_H */
