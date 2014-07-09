/*
 * fcgi.c
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
 * FastCGI implementation
 * 
 * @packet libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/08/2014
 * @chagelog 
 *      [05/08/2014] - Creation
 */

#include "bsp.h"

static void _build_header(BSP_STRING *req, int type, size_t length)
{
    static unsigned char hdr[8];
    if (req)
    {
        hdr[0] = 1;
        hdr[1] = (unsigned char) type;
        hdr[2] = 0;
        hdr[3] = 1;
        hdr[4] = (length >> 8) & 0xFF;
        hdr[5] = length & 0xFF;
        hdr[6] = (8 - (length & 7)) & 7;
        hdr[7] = 0;
        
        string_append(req, (const char *) hdr, 8);
    }
    
    return;
}

static void _build_role(BSP_STRING *req, int role, int flag)
{
    unsigned char r[8];
    if (req)
    {
        r[0] = (role >> 8) & 0XFF;
        r[1] = role & 0xFF;
        r[2] = (unsigned char) flag;
        r[3] = 0;
        r[4] = 0;
        r[5] = 0;
        r[6] = 0;
        r[7] = 0;
        
        string_append(req, (const char *) r, 8);
    }
    
    return;
}

static void _build_nv_pair(BSP_STRING *str, const char *name, const char *value)
{
    static unsigned char len[4];
    if (str && name)
    {
        size_t len_name = strlen(name);
        size_t len_value = (value) ? strlen(value) : 0;
        
        if (len_name > 127)
        {
            set_int32(len_name, (char *) len);
            string_append(str, (const char *) len, 4);
        }
        else
        {
            set_int8(len_name, (char *) len);
            string_append(str, (const char *) len, 1);
        }
        
        if (len_value > 127)
        {
            set_int32(len_value, (char *) len);
            string_append(str, (const char *) len, 4);
        }
        else
        {
            set_int8(len_value, (char *) len);
            string_append(str, (const char *) len, 1);
        }
        string_append(str, name, len_name);
        string_append(str, value, len_value);
    }
    
    return;
}

// Build FastCGI request
BSP_STRING * build_fcgi_request(BSP_FCGI_PARAMS *params, const char *post_data, ssize_t post_len)
{
    BSP_STRING *req = new_string(NULL, 0);

    // Create begin
    _build_header(req, FCGI_BEGIN_REQUEST, 8);
    _build_role(req, FCGI_RESPONDER, 0);
    
    // Create params
    BSP_STRING *p = new_string(NULL, 0);
    _build_nv_pair(p, "QUERY_STRING", params->query_string);
    _build_nv_pair(p, "REQUEST_METHOD", (params->request_method) ? (params->request_method) : FCGI_PARAMS_DEFAULT_REQUEST_METHOD);
    _build_nv_pair(p, "CONTENT_TYPE", params->content_type);
    _build_nv_pair(p, "CONTENT_LENGTH", params->content_length);
    _build_nv_pair(p, "SCRIPT_FILENAME", params->script_filename);
    _build_nv_pair(p, "SCRIPT_NAME", params->script_name);
    _build_nv_pair(p, "REQUEST_URI", params->request_uri);
    _build_nv_pair(p, "DOCUMENT_URI", params->document_uri);
    _build_nv_pair(p, "DOCUMENT_ROOT", params->document_root);
    _build_nv_pair(p, "SERVER_PROTOCOL", (params->server_protocol) ? (params->server_protocol) : FCGI_PARAMS_DEFAULT_SERVER_PROTOCOL);
    _build_nv_pair(p, "GATEWAY_INTERFACE", (params->gateway_interface) ? (params->gateway_interface) : FCGI_PARAMS_DEFAULT_GATEWAY_INTERFACE);
    _build_nv_pair(p, "SERVER_SOFTWARE", (params->server_software) ? (params->server_software) : FCGI_PARAMS_DEFAULT_SERVER_SOFTWARE);
    _build_nv_pair(p, "REMOTE_ADDR", params->remote_addr);
    _build_nv_pair(p, "REMOTE_PORT", params->remote_port);
    _build_nv_pair(p, "SERVER_ADDR", params->server_addr);
    _build_nv_pair(p, "SERVER_PORT", params->server_port);
    _build_nv_pair(p, "SERVER_NAME", params->server_name);
    
    // Additional HTTP params
    
    size_t len = STR_LEN(p);
    size_t curr = 0;
    size_t size = 0;
    for (curr = 0; curr < len; curr += 65536)
    {
        if (len - curr > 65536)
        {
            size = 65536;
        }
        else
        {
            size = (len - curr) % 65536;
        }
        
        _build_header(req, FCGI_PARAMS, size);
        string_append(req, STR_STR(p) + curr, size);
    }
    size = (8 - (len & 7)) & 7;
    if (size > 0)
    {
        string_fill(req, 0, size);
    }
    _build_header(req, FCGI_PARAMS, 0);
    del_string(p);

    // Create stdin
    if (post_data)
    {
        if (post_len < 0)
        {
            post_len = strlen(post_data);
        }
        
        for (curr = 0; curr < post_len; curr += 65536)
        {
            if (post_len - curr > 65536)
            {
                size = 65536;
            }
            else
            {
                size = (post_len - curr) % 65536;
            }
            
            _build_header(req, FCGI_STDIN, size);
            string_append(req, post_data + curr, size);
        }
        size = (8 - (post_len & 7)) & 7;
        if (size > 0)
        {
            string_fill(req, 0, size);
        }
    }
    _build_header(req, FCGI_STDIN, 0);
    
    return req;
}

// Parse FCGI response
size_t parse_fcgi_response(BSP_FCGI_RESPONSE *resp, const char *data, size_t len)
{
    if (!data || !resp)
    {
        return len;
    }
    size_t curr = 0;
    unsigned char hdr[8];
    unsigned char d[8];
    int content_length;
    int padding_length;
    //debug_hex(data, len);
    while ((len - curr) >= 8)
    {
        // First read header
        memcpy(hdr, data + curr, 8);
        content_length = (hdr[4] << 8) + hdr[5];
        padding_length = hdr[6];
        
        if (FCGI_END_REQUEST == hdr[1] && (len - curr) >= 8)
        {
            // Response complete
            // Read body
            memcpy(d, data + curr + 8, 8);
            resp->request_id = (hdr[2] << 8) + hdr[3];
            resp->app_status = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];
            resp->protocol_status = d[4];
            resp->is_ended = 1;
            curr += 16;
            break;
        }
        else if (FCGI_STDOUT == hdr[1] && (len - curr) >=  (content_length + padding_length + 8))
        {
            // Read data
            if (!resp->data_stdout)
            {
                resp->data_stdout = new_string(NULL, 0);
            }
            
            if (content_length > 0)
            {
                string_append(resp->data_stdout, data + curr + 8, content_length);
            }
            curr += (content_length + padding_length + 8);
        }
        else if (FCGI_STDERR == hdr[1] && (len - curr) >= (content_length + padding_length + 8))
        {
            // Read data
            if (!resp->data_stderr)
            {
                resp->data_stderr = new_string(NULL, 0);
            }

            if (content_length > 0)
            {
                string_append(resp->data_stderr, data + curr + 8, content_length);
            }
            curr += (content_length + padding_length + 8);
        }
        else
        {
            // Unsupported request type from FCGI server
            break;
        }
    }
    
    return curr;
}
