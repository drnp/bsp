/*
 * bsp_fcgi.h
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
 * FastCGI header
 * 
 * @packet libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/08/2014
 * @chagelog 
 *      [05/08/2014] - Creation
 */

#ifndef _LIB_BSP_CORE_FCGI_H

#define _LIB_BSP_CORE_FCGI_H
/* Headers */

/* Definations */
#define FCGI_BEGIN_REQUEST                      0x1
#define FCGI_ABORT_REQUEST                      0x2
#define FCGI_END_REQUEST                        0x3
#define FCGI_PARAMS                             0x4
#define FCGI_STDIN                              0x5
#define FCGI_STDOUT                             0x6
#define FCGI_STDERR                             0x7
#define FCGI_DATA                               0x8
#define FCGI_GET_VALUES                         0x9
#define FCGI_GET_VALUES_RESULT                  0xa
#define FCGI_UNKNOWN_TYPE                       0xb
#define FCGI_MAXTYPE                            (FCGI_UNKNOWN_TYPE)

#define FCGI_RESPONDER                          0x1
#define FCGI_AUTHORIZER                         0x2
#define FCGI_FILTER                             0x3

#define FCGI_REQUEST_COMPLETE                   0x0
#define FCGI_CANT_MPX_CONN                      0x1
#define FCGI_OVERLOADED                         0x2
#define FCGI_UNKNOWN_ROLE                       0x3

/* Macros */

/* Structs */
typedef struct bsp_nv_t
{
    const char          *name;
    const char          *value;
} BSP_NV;

/* Functions */
// Build FastCGI request
BSP_STRING * build_fcgi_request(BSP_NV params[], const char *post_data, ssize_t post_len);

#endif  /* _LIB_BSP_CORE_FCGI_H */
