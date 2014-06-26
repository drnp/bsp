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
    static unsigned char r[8];
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
BSP_STRING * build_fcgi_request(BSP_NV params[], const char *post_data, ssize_t post_len)
{
    BSP_STRING *req = new_string(NULL, 0);

    // Create begin
    _build_header(req, FCGI_BEGIN_REQUEST, 8);
    _build_role(req, FCGI_RESPONDER, 0);
    
    // Create params
    BSP_STRING *p = new_string(NULL, 0);
    BSP_NV *nv;
    int n = 0;
    while (params)
    {
        nv = &params[n];
        if (!nv->name)
        {
            break;
        }
        else
        {
            _build_nv_pair(p, nv->name, nv->value);
        }
        n ++;
    }
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
