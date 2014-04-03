/*
 * module_json.h
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
 * JSON encoder / decoder header
 * 
 * @package modules::json
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/29/2012
 * @changelog 
 *      [08/29/2012] - Creation
 */

#ifndef _MODULE_JSON_H
#define _MODULE_JSON_H
/* Headers */

/* Definations */
#define JSON_LAYER_TOP                          0
#define JSON_LAYER_OBJECT                       1
#define JSON_LAYER_ARRAY                        2
#define JSON_INVALID_KEY                        "__JSON_INVALID_KEY__"

/* Macros */

/* Structs */

/* Functions */
int bsp_module_json(lua_State *s);

#endif  /* _MODULE_JSON_H */
