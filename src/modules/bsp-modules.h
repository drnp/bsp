/*
 * bsp-modules.h
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
 * Modules initial list
 * 
 * @package modules
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/07/2012
 * @changelog 
 *      [08/07/2012] - Creation
 */

#include "standard/module_standard.h"
#include "http/module_http.h"
#include "json/module_json.h"
#include "mysql/module_mysql.h"
#include "sqlite/module_sqlite.h"
#include "test/module_test.h"

#define load_bsp_modules(scr)                   bsp_module_standard(scr->state); \
                                                bsp_module_http(scr->state); \
                                                bsp_module_json(scr->state); \
                                                bsp_module_mysql(scr->state); \
                                                bsp_module_sqlite(scr->state); \
                                                bsp_module_test(scr->state)
