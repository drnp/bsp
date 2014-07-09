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

#define FCGI_PARAMS_DEFAULT_REQUEST_METHOD      "GET"
#define FCGI_PARAMS_DEFAULT_SERVER_PROTOCOL     "HTTP/1.1"
#define FCGI_PARAMS_DEFAULT_GATEWAY_INTERFACE   "CGI/1.1"
#define FCGI_PARAMS_DEFAULT_SERVER_SOFTWARE     "BS.Play_FCGI_Client"

/* Macros */

/* Structs */
typedef struct bsp_nv_t
{
    const char          *name;
    const char          *value;
} BSP_NV;

typedef struct bsp_fcgi_params_t
{
    const char          *query_string;
    const char          *request_method;
    const char          *content_type;
    const char          *content_length;
    const char          *script_filename;
    const char          *script_name;
    const char          *request_uri;
    const char          *document_uri;
    const char          *document_root;
    const char          *server_protocol;
    const char          *gateway_interface;
    const char          *server_software;
    const char          *remote_addr;
    const char          *remote_port;
    const char          *server_addr;
    const char          *server_port;
    const char          *server_name;
} BSP_FCGI_PARAMS;

typedef struct bsp_fcgi_response_t
{
    int                 request_id;                             
    int                 app_status;
    int                 protocol_status;
    int                 is_ended;
    BSP_STRING          *data_stdout;
    BSP_STRING          *data_stderr;
} BSP_FCGI_RESPONSE;

/* Functions */
// Build FastCGI request
BSP_STRING * build_fcgi_request(BSP_FCGI_PARAMS *params, const char *post_data, ssize_t post_len);

// Parse FCGI response
size_t parse_fcgi_response(BSP_FCGI_RESPONSE *resp, const char *data, size_t len);

#endif  /* _LIB_BSP_CORE_FCGI_H */
