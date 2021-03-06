/*
 * bsp_bootstrap.h
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
 * Bootstrap loader header
 * 
 * @package libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/08/2014
 * @changelog 
 *      [05/08/2014] - Creation
 */

#ifndef _LIB_BSP_CORE_BOOTSTRAP_H

#define _LIB_BSP_CORE_BOOTSTRAP_H
/* Headers */

/* Definations */

/* Macros */

/* Structs */

/* Functions */
// Start bootstrap
int start_bootstrap(BSP_STRING *bs);

#endif  /* _LIB_BSP_CORE_BOOTSTRAP_H */
