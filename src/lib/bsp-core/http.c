/*
 * http.c
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
 * HTTP request/response generator and parser
 * 
 * @package bsp::libbsp-ext
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/23/2012
 * @changelog 
 *      [08/23/2012] - Creation
 */

#include "bsp.h"

#include <openssl/sha.h>
#include <openssl/evp.h>

static void clear_http_request(BSP_HTTP_REQUEST *req)
{
    if (req)
    {
        if (req->version)
        {
            bsp_free(req->version);
        }

        if (req->method)
        {
            bsp_free(req->method);
        }

        if (req->user_agent)
        {
            bsp_free(req->user_agent);
        }
        
        if (req->host)
        {
            bsp_free(req->host);
        }

        if (req->request_uri)
        {
            bsp_free(req->request_uri);
        }

        if (req->upgrade)
        {
            bsp_free(req->upgrade);
        }

        if (req->origin)
        {
            bsp_free(req->origin);
        }

        if (req->sec_websocket_protocol)
        {
            bsp_free(req->sec_websocket_protocol);
        }

        if (req->sec_websocket_key)
        {
            bsp_free(req->sec_websocket_key);
        }

        if (req->sec_websocket_key1)
        {
            bsp_free(req->sec_websocket_key1);
        }

        if (req->sec_websocket_key2)
        {
            bsp_free(req->sec_websocket_key2);
        }

        if (req->raw_post_data)
        {
            bsp_free(req->raw_post_data);
        }

        memset(req, 0, sizeof(BSP_HTTP_REQUEST));
    }

    return;
}

BSP_HTTP_REQUEST * new_http_request()
{
    BSP_HTTP_REQUEST *req = bsp_calloc(1, sizeof(BSP_HTTP_REQUEST));
    if (!req)
    {
        return NULL;
    }
    
    // Default values;
    http_request_set_version(req, HTTP_VERSION_1_1, -1);
    http_request_set_method(req, HTTP_METHOD_GET, -1);
    http_request_set_user_agent(req, HTTP_DEFAULT_USER_AGENT, -1);
    req->port = HTTP_DEFAULT_PORT;
    
    return req;
}

void del_http_request(BSP_HTTP_REQUEST *req)
{
    if (req)
    {
        clear_http_request(req);
    }

    bsp_free(req);

    return;
}

void http_request_set_version(BSP_HTTP_REQUEST *req, const char *version, ssize_t len)
{
    if (req)
    {
        if (req->version)
        {
            bsp_free(req->version);
        }
        
        req->version = bsp_strndup(version, len);
    }

    return;
}

void http_request_set_method(BSP_HTTP_REQUEST *req, const char *method, ssize_t len)
{
    if (req)
    {
        if (req->method)
        {
            bsp_free(req->method);
        }
        
        req->method = bsp_strndup(method, len);
    }

    return;
}

void http_request_set_user_agent(BSP_HTTP_REQUEST *req, const char *agent, ssize_t len)
{
    if (req)
    {
        if (req->user_agent)
        {
            bsp_free(req->user_agent);
        }
        
        req->user_agent = bsp_strndup(agent, len);
    }

    return;
}

void http_request_set_host(BSP_HTTP_REQUEST *req, const char *host, ssize_t len)
{
    if (req)
    {
        if (req->host)
        {
            bsp_free(req->host);
        }

        req->host = bsp_strndup(host, len);
    }

    return;
}

void http_request_set_request_uri(BSP_HTTP_REQUEST *req, const char *uri, ssize_t len)
{
    if (req)
    {
        if (req->request_uri)
        {
            bsp_free(req->request_uri);
        }

        req->request_uri = bsp_strndup(uri, len);
    }

    return;
}

void http_request_set_upgrade(BSP_HTTP_REQUEST *req, const char *upgrade, ssize_t len)
{
    if (req)
    {
        if (req->upgrade)
        {
            bsp_free(req->upgrade);
        }

        req->upgrade = bsp_strndup(upgrade, len);
    }

    return;
}

void http_request_set_origin(BSP_HTTP_REQUEST *req, const char *origin, ssize_t len)
{
    if (req)
    {
        if (req->origin)
        {
            bsp_free(req->origin);
        }

        req->origin = bsp_strndup(origin, len);
    }

    return;
}

void http_request_set_connection(BSP_HTTP_REQUEST *req, const char *connection, ssize_t len)
{
    if (req)
    {
        if (req->connection)
        {
            bsp_free(req->connection);
        }

        req->connection = bsp_strndup(connection, len);
    }

    return;
}

void http_request_set_sec_websocket_version(BSP_HTTP_REQUEST *req, int version)
{
    if (req)
    {
        req->sec_websocket_version = version;
    }

    return;
}

void http_request_set_sec_websocket_protocol(BSP_HTTP_REQUEST *req, const char *protocol, ssize_t len)
{
    if (req)
    {
        if (req->sec_websocket_protocol)
        {
            bsp_free(req->sec_websocket_protocol);
        }

        req->sec_websocket_protocol = bsp_strndup(protocol, len);
    }

    return;
}

void http_request_set_sec_websocket_key(BSP_HTTP_REQUEST *req, const char *key, ssize_t len)
{
    if (req)
    {
        if (req->sec_websocket_key)
        {
            bsp_free(req->sec_websocket_key);
        }

        req->sec_websocket_key = bsp_strndup(key, len);
    }

    return;
}

void http_request_set_sec_websocket_key1(BSP_HTTP_REQUEST *req, const char *key, ssize_t len)
{
    if (req)
    {
        if (req->sec_websocket_key1)
        {
            bsp_free(req->sec_websocket_key1);
        }

        req->sec_websocket_key1 = bsp_strndup(key, len);
    }

    return;
}

void http_request_set_sec_websocket_key2(BSP_HTTP_REQUEST *req, const char *key, ssize_t len)
{
    if (req)
    {
        if (req->sec_websocket_key2)
        {
            bsp_free(req->sec_websocket_key2);
        }

        req->sec_websocket_key2 = bsp_strndup(key, len);
    }

    return;
}

void http_request_set_post_data(BSP_HTTP_REQUEST *req, const char *data, ssize_t len)
{
    if (len < 0)
    {
        len = strlen(data);
    }

    if (req)
    {
        if (req->raw_post_data)
        {
            bsp_free(req->raw_post_data);
        }

        req->raw_post_data = bsp_malloc(len);
        memcpy(req->raw_post_data, data, len);
        req->raw_post_data_size = len;
        http_request_set_method(req, HTTP_METHOD_POST, -1);
    }

    return;
}

// ===
void clear_http_response(BSP_HTTP_RESPONSE *resp)
{
    if (resp)
    {
        if (resp->version)
        {
            bsp_free(resp->version);
        }

        if (resp->status_description)
        {
            bsp_free(resp->status_description);
        }

        if (resp->content_type)
        {
            bsp_free(resp->content_type);
        }

        if (resp->content)
        {
            bsp_free(resp->content);
        }

        if (resp->transfer_encoding)
        {
            bsp_free(resp->transfer_encoding);
        }

        if (resp->upgrade)
        {
            bsp_free(resp->upgrade);
        }

        if (resp->connection)
        {
            bsp_free(resp->connection);
        }

        if (resp->sec_websocket_accept)
        {
            bsp_free(resp->sec_websocket_accept);
        }

        if (resp->sec_websocket_protocol)
        {
            bsp_free(resp->sec_websocket_protocol);
        }

        memset(resp, 0, sizeof(BSP_HTTP_RESPONSE));
    }

    return;
}

BSP_HTTP_RESPONSE * new_http_response()
{
    BSP_HTTP_RESPONSE *resp = bsp_malloc(sizeof(BSP_HTTP_RESPONSE));
    if (!resp)
    {
        return NULL;
    }
    
    memset(resp, 0, sizeof(BSP_HTTP_RESPONSE));

    return resp;
}

void del_http_response(BSP_HTTP_RESPONSE *resp)
{
    if (resp)
    {
        clear_http_response(resp);
    }
    
    bsp_free(resp);

    return;
}

void http_response_set_version(BSP_HTTP_RESPONSE *resp, const char *version, ssize_t len)
{
    if (resp)
    {
        if (resp->version)
        {
            bsp_free(resp->version);
        }

        resp->version = bsp_strndup(version, len);
    }

    return;
}

void http_response_set_status_code(BSP_HTTP_RESPONSE *resp, int code)
{
    if (resp)
    {
        resp->status_code = code;
    }

    return;
}

void http_response_set_status_description(BSP_HTTP_RESPONSE *resp, const char *description, ssize_t len)
{
    if (resp)
    {
        if (resp->status_description)
        {
            bsp_free(resp->status_description);
        }

        resp->status_description = bsp_strndup(description, len);
    }

    return;
}

void http_response_set_content_type(BSP_HTTP_RESPONSE *resp, const char *type, ssize_t len)
{
    if (resp)
    {
        if (resp->content_type)
        {
            bsp_free(resp->content_type);
        }

        resp->content_type = bsp_strndup(type, len);
    }

    return;
}

void http_response_set_content_length(BSP_HTTP_RESPONSE *resp, size_t len)
{
    if (resp)
    {
        resp->content_length = len;
    }

    return;
}

void http_response_set_content(BSP_HTTP_RESPONSE *resp, const char *content, ssize_t len)
{
    if (len < 0)
    {
        len = strlen(content);
    }

    if (resp)
    {
        if (resp->content)
        {
            bsp_free(resp->content);
            resp->content_language = 0;
        }

        resp->content = bsp_malloc(len);
        if (resp->content)
        {
            memcpy(resp->content, content, len);
            resp->content_length = len;
        }
    }

    return;
}

// Append data to response, used for chunked transfer
void http_response_append_content(BSP_HTTP_RESPONSE *resp, const char *content, ssize_t len)
{
    if (len < 0)
    {
        len = strlen(content);
    }

    if (resp)
    {
        resp->content = bsp_realloc(resp->content, resp->content_length + len);
        if (resp->content)
        {
            memcpy(resp->content + resp->content_length, content, len);
            resp->content_length += len;
        }
    }
    
    return;
}

void http_response_set_transfer_encoding(BSP_HTTP_RESPONSE *resp, const char *encoding, ssize_t len)
{
    if (resp)
    {
        if (resp->transfer_encoding)
        {
            bsp_free(resp->transfer_encoding);
        }

        resp->transfer_encoding = bsp_strndup(encoding, len);
    }

    return;
}

void http_response_set_upgrade(BSP_HTTP_RESPONSE *resp, const char *upgrade, ssize_t len)
{
    if (resp)
    {
        if (resp->upgrade)
        {
            bsp_free(resp->upgrade);
        }

        resp->upgrade = bsp_strndup(upgrade, len);
    }

    return;
}

void http_response_set_connection(BSP_HTTP_RESPONSE *resp, const char *connection, ssize_t len)
{
    if (resp)
    {
        if (resp->connection)
        {
            bsp_free(resp->connection);
        }

        resp->connection = bsp_strndup(connection, len);
    }

    return;
}

void http_response_set_sec_websocket_accept(BSP_HTTP_RESPONSE *resp, const char *accept, ssize_t len)
{
    if (resp)
    {
        if (resp->sec_websocket_accept)
        {
            bsp_free(resp->sec_websocket_accept);
        }

        resp->sec_websocket_accept = bsp_strndup(accept, len);
    }

    return;
}

void http_response_set_sec_websocket_protocol(BSP_HTTP_RESPONSE *resp, const char *protocol, ssize_t len)
{
    if (resp)
    {
        if (resp->sec_websocket_protocol)
        {
            bsp_free(resp->sec_websocket_protocol);
        }

        resp->sec_websocket_protocol = bsp_strndup(protocol, len);
    }

    return;
}

void http_response_set_sec_websocket_origin(BSP_HTTP_RESPONSE *resp, const char *origin, ssize_t len)
{
    if (resp)
    {
        if (resp->sec_websocket_origin)
        {
            bsp_free(resp->sec_websocket_origin);
        }

        resp->sec_websocket_origin = bsp_strndup(origin, len);
    }

    return;
}

/* HTTP request genrator */
BSP_STRING * generate_http_request(BSP_HTTP_REQUEST *req)
{
    BSP_STRING *ret = NULL;
    if (req && req->host && req->request_uri)
    {
        ret = new_string(NULL, 0);
        string_printf(ret, "%s %s %s\r\n", req->method, req->request_uri, req->version);
        string_printf(ret, "Host: %s\r\n", req->host);

        if (req->user_agent)
        {
            string_printf(ret, "User-Agent: %s\r\n", req->user_agent);
        }

        if (req->raw_post_data)
        {
            // Content-Type hack
            string_printf(ret, "Content-Type: application/x-www-form-urlencoded\r\n");
            string_printf(ret, "Content-Length: %d\r\n\r\n", (int) req->raw_post_data_size);
            string_append(ret, req->raw_post_data, req->raw_post_data_size);
        }

        string_append(ret, "\r\n", -1);
        status_op_http(STATUS_OP_HTTP_REQUEST);
    }

    return ret;
}

/* HTTP request parser */
size_t parse_http_request(const char *data, ssize_t len, BSP_HTTP_REQUEST *req)
{
    if (len < 0)
    {
        len = strlen(data);
    }

    int i = 0;
    size_t head_len = 0;
    for (i = 0; i < len - 3; i ++)
    {
        // Check CRLF/CRLF
        if (data[i] == 0x0d && data[i + 1] == 0x0a && data[i + 2] == 0x0d && data[i + 3] == 0x0a)
        {
            head_len = i;
            break;
        }
    }

    if (!head_len)
    {
        // Broken package
        return 0;
    }

    if (!req)
    {
        return 0;
    }

    char *brk = NULL;
    char *curr = strstr(data, "HTTP/");
    if (!curr || (curr - data) > head_len)
    {
        // Not a valid HTTP response
        return 0;
    }

    brk = strpbrk(curr, " \t\r\n");
    if (!brk)
    {
        return 0;
    }

    http_request_set_version(req, curr, brk - curr);

    int new_line = 0;
    int is_value = 0;
    char *req_key = NULL, *req_value = NULL;
    int req_value_len = 0;

    for (i = curr - data; i <= head_len; i ++)
    {
        if (new_line)
        {
            if (data[i] > 32)
            {
                if (!req_key && !is_value)
                {
                    req_key = (char *) data + i;
                }

                if (!req_value && is_value)
                {
                    req_value = (char *) data + i;
                }
            }

            if (data[i] == ':' && !is_value)
            {
                is_value = 1;
            }
        }

        if (data[i] == 0x0d || i == head_len)
        {
            new_line = 0;
            if (req_value)
            {
                req_value_len = (data + i) - req_value;
            }
        }
        
        if (data[i] == 0x0a)
        {
            new_line = 1;
        }

        if (!new_line && req_key && req_value)
        {
            // Set values
            if (0 == strncasecmp("method", req_key, 6))
            {
                http_request_set_method(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("request_uri", req_key, 11))
            {
                http_request_set_request_uri(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("host", req_key, 4))
            {
                http_request_set_host(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("user-agent", req_key, 10))
            {
                http_request_set_user_agent(req, req_value, req_value_len);
            }
            
            else if (0 == strncasecmp("upgrade", req_key, 7))
            {
                http_request_set_upgrade(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("origin", req_key, 6))
            {
                http_request_set_origin(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("connection", req_key, 10))
            {
                http_request_set_connection(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("sec-websocket-version", req_key, 21))
            {
                http_request_set_sec_websocket_version(req, atoi(req_value));
            }

            else if (0 == strncasecmp("sec-websocket-protocol", req_key, 22))
            {
                http_request_set_sec_websocket_protocol(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("sec-websocket-key1", req_key, 18))
            {
                http_request_set_sec_websocket_key1(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("sec-websocket-key2", req_key, 18))
            {
                http_request_set_sec_websocket_key2(req, req_value, req_value_len);
            }

            else if (0 == strncasecmp("sec-websocket-key", req_key, 17))
            {
                http_request_set_sec_websocket_key(req, req_value, req_value_len);
            }
            
            req_key = NULL;
            req_value = NULL;
            req_value_len = 0;
            is_value = 0;
        }
    }

    return head_len + 4;
}

/* HTTP response generator */
BSP_STRING * generate_http_response(BSP_HTTP_RESPONSE *resp)
{
    if (!resp)
    {
        return NULL;
    }

    if (resp->version && resp->status_code)
    {
        BSP_STRING *ret = new_string(NULL, 0);
        string_printf(ret, "%s %d %s\r\n", resp->version, resp->status_code, resp->status_description ? resp->status_description : "OK");

        if (resp->server)
        {
            string_printf(ret, "Server: %s\r\n", resp->server);
        }

        if (resp->connection)
        {
            string_printf(ret, "Connection: %s\r\n", resp->connection);
        }

        if (resp->upgrade)
        {
            string_printf(ret, "Upgrade: %s\r\n", resp->upgrade);
        }

        if (resp->sec_websocket_accept)
        {
            string_printf(ret, "Sec-WebSocket-Accept: %s\r\n", resp->sec_websocket_accept);
        }

        if (resp->sec_websocket_protocol)
        {
            string_printf(ret, "Sec-WebSocket-Protocol: %s\r\n", resp->sec_websocket_protocol);
        }

        if (resp->sec_websocket_origin)
        {
            string_printf(ret, "Sec-WebSocket-Origin: %s\r\n", resp->sec_websocket_origin);
        }

        string_append(ret, "\r\n", -1);
        status_op_http(STATUS_OP_HTTP_RESPONSE);

        return ret;
    }

    return NULL;
}

/* HTTP response parser */
size_t parse_http_response(const char *data, ssize_t len, BSP_HTTP_RESPONSE *resp)
{
    // Find head terminated tag
    int i = 0;
    size_t head_len = 0;

    if (!resp || !data)
    {
        return 0;
    }
    
    if (len < 0)
    {
        len = strlen(data);
    }

    for (i = 0; i < len - 3; i ++)
    {
        // Check CRLF/CRLF
        if (data[i] == 0x0d && data[i + 1] == 0x0a && data[i + 2] == 0x0d && data[i + 3] == 0x0a)
        {
            head_len = i;
            break;
        }
    }

    if (!head_len)
    {
        // Broken package
        return 0;
    }

    http_response_set_content_length(resp, 0);
    // Walk stream, the first line must be the HTTP version and status
    char *brk = NULL;
    char *curr = strstr(data, "HTTP/");
    if (!curr || (curr - data) > head_len)
    {
        // Not a valid HTTP response
        return 0;
    }

    brk = strchr(curr, 0x20);
    if (!brk)
    {
        return 0;
    }

    http_response_set_version(resp, curr, (brk - curr));

    // Find status code
    curr = strpbrk(brk, "0123456789");
    int status_code = atoi(curr);
    if (status_code)
    {
        http_response_set_status_code(resp, status_code);
    }

    // Loop each line
    int new_line = 0;
    int is_value = 0;
    char *resp_key = NULL, *resp_value = NULL;
    int resp_value_len = 0;

    for (i = curr - data; i <= head_len; i ++)
    {
        if (new_line)
        {
            if (data[i] > 32)
            {
                if (!resp_key && !is_value)
                {
                    resp_key = (char *) data + i;
                }

                if (!resp_value && is_value)
                {
                    resp_value = (char *) data + i;
                }
            }

            if (data[i] == ':' && !is_value)
            {
                is_value = 1;
            }
        }

        if (data[i] == 0x0d || i == head_len)
        {
            new_line = 0;
            if (resp_value)
            {
                resp_value_len = (data + i) - resp_value;
            }
        }

        if (data[i] == 0x0a)
        {
            new_line = 1;
        }

        if (!new_line && resp_key && resp_value)
        {
            // Set values
            if (0 == strncasecmp("content-type", resp_key, 12))
            {
                http_response_set_content_type(resp, resp_value, resp_value_len);
            }

            else if (0 == strncasecmp("content-length", resp_key, 14))
            {
                http_response_set_content_length(resp, (size_t) strtoll(resp_value, NULL, 10));
            }

            else if (0 == strncasecmp("transfer-encoding", resp_key, 17))
            {
                http_response_set_transfer_encoding(resp, resp_value, resp_value_len);
            }

            else if (0 == strncasecmp("connection", resp_key, 10))
            {
                http_response_set_connection(resp, resp_value, resp_value_len);
            }

            resp_key = NULL;
            resp_value = NULL;
            resp_value_len = 0;
            is_value = 0;
        }
    }

    return head_len + 4;    // 2 CRLF additional
}

// Generate HTML5 websocket handshake reesponse
// Support version 8 & 13
int websocket_handshake(BSP_HTTP_REQUEST *req, BSP_HTTP_RESPONSE *resp)
{
    BSP_STRING *key_accept = NULL;
    SHA_CTX sha;
    static unsigned char sha_hash[SHA_DIGEST_LENGTH];

    if (!req || !resp)
    {
        return BSP_RTN_ERROR_GENERAL;
    }
    
    if (req->upgrade && req->connection && 0 == strncasecmp(req->upgrade, "websocket", 9))
    {
        // WebSocket request
        http_response_set_version(resp, req->version, -1);
        http_response_set_status_code(resp, HTTP_STATUS_SWITCHING);
        http_response_set_status_description(resp, "WebSocket HandShake", -1);
        http_response_set_upgrade(resp, "WebSocket", -1);
        http_response_set_connection(resp, "Upgrade", -1);

        if (req->sec_websocket_protocol)
        {
            http_response_set_sec_websocket_protocol(resp, req->sec_websocket_protocol, -1);
        }

        if (req->origin)
        {
            http_response_set_sec_websocket_origin(resp, req->origin, -1);
        }

        // Secure key
        if (req->sec_websocket_key)
        {
            // RFC6455
            // Version.10+ (Chrome / Safari / Firefox)
            if (1 == SHA1_Init(&sha) && 
                1 == SHA1_Update(&sha, req->sec_websocket_key, strlen(req->sec_websocket_key)) && 
                1 == SHA1_Update(&sha, RFC4122_GUID, 36))
            {
                SHA1_Final(sha_hash, &sha);

                // Base64_encode
                key_accept = string_base64_encode((const char *) sha_hash, SHA_DIGEST_LENGTH);
                http_response_set_sec_websocket_accept(resp, key_accept->str, -1);
                del_string(key_accept);
            }

            else
            {
                // Sha1 error
                http_response_set_sec_websocket_accept(resp, req->sec_websocket_key, -1);
            }
        }

        else if(req->sec_websocket_key1 && req->sec_websocket_key2)
        {
            // Version.7.5 7.6
        }
    }

    return BSP_RTN_SUCCESS;
}

// Parse websocket data
size_t parse_websocket_data(const char *data, ssize_t len, int *opcode, BSP_STRING *data_str)
{
    if (!data || !data_str)
    {
        *opcode = 0xF;
        return 0;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    if (len < 2)
    {
        *opcode = 0xF;
        return 0;
    }

    if ((unsigned char) data[0] >> 4 != 0x8)
    {
        // Multi frament and RSV not support now
        *opcode = 0xF;
        return 0;
    }

    // Fake value now
    *opcode = (unsigned char) data[0] & 0xF;
    int mask = (unsigned char) data[1] >> 7;
    size_t data_len = (unsigned char) data[1] & 0x7F;
    size_t remaining = len;
    char mask_data[4];
    if (126 == data_len)
    {
        // Next two bytes
        if (len < 4)
        {
            *opcode = 0xF;
            return 0;
        }

        data_len = (size_t) get_int16(data + 2);
        remaining -= 4;
    }

    else if (127 == data_len)
    {
        // Next eight bytes
        if (len < 10)
        {
            *opcode = 0xF;
            return 0;
        }

        data_len = (size_t) get_int64(data + 2);
        remaining -= 10;
    }

    else
    {
        remaining -= 2;
    }

    if (mask)
    {
        // Four bytes mask data
        if (remaining < 4)
        {
            *opcode = 0xF;
            return 0;
        }

        memcpy(mask_data, data + (len - remaining), 4);
        remaining -= 4;
    }

    if (remaining < data_len)
    {
        // Data not complete
        *opcode = 0xF;
        return 0;
    }
    
    const char *start = data + (len - remaining);
    if (mask)
    {
        size_t i;
        // Ensure space
        string_fill(data_str, -1, data_len);
        for (i = 0; i < data_len; i ++)
        {
            data_str->str[i] = start[i] ^ mask_data[i & 3];
        }
    }

    else
    {
        string_append(data_str, start, data_len);
    }

    return (len - remaining + data_len);
}

// Generate WebSocket data
BSP_STRING * generate_websocket_data(const char *data, ssize_t len, int opcode, int mask)
{
    char head[14];
    int head_len = 0;

    if (!data)
    {
        len = 0;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    mask = mask ? 1 : 0;
    BSP_STRING *ret = new_string(NULL, 0);
    head[0] = 0x80 | opcode;
    if (len > 65535)
    {
        head[1] = mask << 7 | 0x7F;
        set_int64(len, &head[2]);
        head_len = 10;
    }

    else if (len > 126)
    {
        head[1] = mask << 7 | 0x7E;
        set_int16(len, &head[2]);
        head_len = 4;
    }

    else
    {
        head[1] = mask << 7 | len;
        head_len = 2;
    }

    if (mask)
    {
        get_rand(&head[head_len], 4);
        head_len += 4;
    }

    string_append(ret, head, head_len);
    // Write data
    if (!mask)
    {
        string_append(ret, data, len);
    }

    else
    {
        // XOR with mask
        string_fill(ret, -1, len);
        size_t i;
        for (i = 0; i < len; i ++)
        {
            ret->str[head_len + i] = data[i] ^ head[head_len - 4 + (i & 3)];
        }
    }

    return ret;
}

