/*
 * bsp_http.h
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
 * HTTP request generator and parser header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 08/23/2012
 * @changelog 
 *      [08/23/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_HTTP_H
#define _LIB_BSP_CORE_HTTP_H
/* Headers */

/* Definations */
// HTTP 1.1 codes
#define HTTP_STATUS_CONTINUE                    100
#define HTTP_STATUS_SWITCHING                   101
#define HTTP_STATUS_OK                          200
#define HTTP_STATUS_CREATED                     201
#define HTTP_STATUS_ACCEPTED                    202
#define HTTP_STATUS_NON_AUTHORITATIVE           203
#define HTTP_STATUS_NO_CONTENT                  204
#define HTTP_STATUS_RESET_CONTENT               205
#define HTTP_STATUS_PARTIAL_CONTENT             206
#define HTTP_STATUS_MULTIPLE_CHOICES            300
#define HTTP_STATUS_MOVED_PERMANENTLY           301
#define HTTP_STATUS_FOUND                       302
#define HTTP_STATUS_SEE_OTHER                   303
#define HTTP_STATUS_NOT_MODIFIED                304
#define HTTP_STATUS_USE_PROXY                   305
#define HTTP_STATUS_UNUSED                      306
#define HTTP_STATUS_TEMPORARY_REDIRECT          307
#define HTTP_STATUS_BAD_REQUEST                 400
#define HTTP_STATUS_UNAUTHORIZED                401
#define HTTP_STATUS_PAYMENT_REQUEST             402
#define HTTP_STATUS_FORBIDDEN                   403
#define HTTP_STATUS_NOT_FOUND                   404
#define HTTP_STATUS_METHOD_NOT_ALLOWED          405
#define HTTP_STATUS_NOT_ACCEPTABLE              406
#define HTTP_STATUS_PROXY_AUTHONTICATION        407
#define HTTP_STATUS_REQUEST_TIMEOUT             408
#define HTTP_STATUS_CONFLICT                    409
#define HTTP_STATUS_GONE                        410
#define HTTP_STATUS_LENGTH_REQUIRED             411
#define HTTP_STATUS_PRECONDITON_FAILED          412
#define HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE    413
#define HTTP_STATUS_REQUEST_URI_TOO_LONG        414
#define HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE      415
#define HTTP_STATUS_EXPECTATION_FAILED          416
#define HTTP_STATUS_INTERNAL_SERVER_ERROR       500
#define HTTP_STATUS_NOT_IMPLEMENTED             501
#define HTTP_STATUS_BAD_GATEWAY                 502
#define HTTP_STATUS_SERVICE_UNAVAILABLE         503
#define HTTP_STATUS_GATEWAY_TIMEOUT             504
#define HTTP_STATUS_VERSION_NOT_SUPPORTED       505

#define HTTP_METHOD_HEAD                        "HEAD"
#define HTTP_METHOD_GET                         "GET"
#define HTTP_METHOD_POST                        "POST"
#define HTTP_METHOD_PUT                         "PUT"
#define HTTP_METHOD_DELETE                      "DELETE"
#define HTTP_METHOD_TRACE                       "TRACE"
#define HTTP_METHOD_OPTIONS                     "OPTIONS"
#define HTTP_METHOD_CONNECT                     "CONNECT"
#define HTTP_METHOD_PATCH                       "PATCH"

#define HTTP_VERSION_0_9                        "HTTP/0.9"
#define HTTP_VERSION_1_0                        "HTTP/1.0"
#define HTTP_VERSION_1_1                        "HTTP/1.1"
#define HTTP_DEFAULT_USER_AGENT                 "BS.Play_HTTP_Client"
#define HTTP_DEFAULT_SERVER                     "BS.Play_HTTP_Server"

#define HTTP_DEFAULT_PORT                       80
#define HTTP_DEFAULT_PORT_SSL                   443

#define RFC4122_GUID                            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define WS_OPCODE_CONTINUATION                  0x0
#define WS_OPCODE_TEXT                          0x1
#define WS_OPCODE_BINARY                        0x2
#define WS_OPCODE_CLOSE                         0x8
#define WS_OPCODE_PING                          0x9
#define WS_OPCODE_PONG                          0xa

/* Macros */

/* Structs */
typedef struct bsp_http_request_t
{
    char                *version;
    char                *method;
    char                *request_uri;
    
    char                *cache;
    char                *connection;
    char                *date;
    char                *pragma;
    char                *range;
    
    char                *host;
    int                 port;
    char                *referer;
    char                *user_agent;
    char                *upgrade;
    char                *origin;
    
    char                *accept;
    char                *accept_charset;
    char                *accept_encoding;
    char                *accept_language;
    
    char                *cookie;
    char                *raw_post_data;
    size_t              raw_post_data_size;
    
    size_t              raw_post_data_recieved;
    
    int                 sec_websocket_version;
    char                *sec_websocket_protocol;
    char                *sec_websocket_extensions;
    char                *sec_websocket_key;
    char                *sec_websocket_key1;
    char                *sec_websocket_key2;
} BSP_HTTP_REQUEST;

typedef struct bsp_http_response_t
{
    char                *version;
    char                *status_description;
    int                 status_code;
    
    char                *date;
    char                *pragma;
    char                *server;
    char                *location;
    char                *connection;
    char                *content_base;
    char                *content_encoding;
    char                *content_language;
    size_t              content_length;
    char                *content_location;
    char                *content_md5;
    char                *content_range;
    char                *content_type;
    char                *transfer_encoding;
    char                *etag;
    char                *expires;
    char                *last_modified;
    char                *upgrade;
    
    char                *content;
    
    size_t              content_recieved;
    
    char                *sec_websocket_accept;
    char                *sec_websocket_origin;
    char                *sec_websocket_location;
    char                *sec_websocket_protocol;
} BSP_HTTP_RESPONSE;

/* Functions */
// == Request ==
BSP_HTTP_REQUEST * new_http_request();
void del_http_request(BSP_HTTP_REQUEST *req);

// Set values
void http_request_set_version(BSP_HTTP_REQUEST *req, const char *version, ssize_t len);
void http_request_set_method(BSP_HTTP_REQUEST *req, const char *method, ssize_t len);
void http_request_set_user_agent(BSP_HTTP_REQUEST *req, const char *agent, ssize_t len);
void http_request_set_host(BSP_HTTP_REQUEST *req, const char *host, ssize_t len);
void http_request_set_request_uri(BSP_HTTP_REQUEST *req, const char *uri, ssize_t len);
void http_request_set_upgrade(BSP_HTTP_REQUEST *req, const char *upgrade, ssize_t len);
void http_request_set_origin(BSP_HTTP_REQUEST *req, const char *origin, ssize_t len);
void http_request_set_connection(BSP_HTTP_REQUEST *req, const char *connection, ssize_t len);
void http_request_set_sec_websocket_version(BSP_HTTP_REQUEST *req, int version);
void http_request_set_sec_websocket_protocol(BSP_HTTP_REQUEST *req, const char *protocol, ssize_t len);
void http_request_set_sec_websocket_key(BSP_HTTP_REQUEST *req, const char *key, ssize_t len);
void http_request_set_sec_websocket_key1(BSP_HTTP_REQUEST *req, const char *key, ssize_t len);
void http_request_set_sec_websocket_key2(BSP_HTTP_REQUEST *req, const char *key, ssize_t len);
void http_request_set_post_data(BSP_HTTP_REQUEST *req, const char *data, ssize_t len);

// Generate a request stream
BSP_STRING * generate_http_request(BSP_HTTP_REQUEST *req);

// Parse a request to object
size_t parse_http_request(const char *data, ssize_t len, BSP_HTTP_REQUEST *req);

// == Response ==
BSP_HTTP_RESPONSE * new_http_response();
void del_http_response(BSP_HTTP_RESPONSE *resp);

// Set values
void http_response_set_version(BSP_HTTP_RESPONSE *resp, const char *version, ssize_t len);
void http_response_set_status_code(BSP_HTTP_RESPONSE *resp, int code);
void http_response_set_status_description(BSP_HTTP_RESPONSE *resp, const char *description, ssize_t len);
void http_response_set_content_type(BSP_HTTP_RESPONSE *resp, const char *type, ssize_t len);
void http_response_set_content_length(BSP_HTTP_RESPONSE *resp, size_t len);
void http_response_set_content(BSP_HTTP_RESPONSE *resp, const char *content, ssize_t len);
void http_response_append_content(BSP_HTTP_RESPONSE *resp, const char *content, ssize_t len);
void http_response_set_transfer_encoding(BSP_HTTP_RESPONSE *resp, const char *encoding, ssize_t len);
void http_response_set_upgrade(BSP_HTTP_RESPONSE *resp, const char *upgrade, ssize_t len);
void http_response_set_connection(BSP_HTTP_RESPONSE *resp, const char *connection, ssize_t len);
void http_response_set_sec_websocket_accept(BSP_HTTP_RESPONSE *resp, const char *accept, ssize_t len);
void http_response_set_sec_websocket_protocol(BSP_HTTP_RESPONSE *resp, const char *protocol, ssize_t len);
void http_response_set_sec_websocket_origin(BSP_HTTP_RESPONSE *resp, const char *origin, ssize_t len);

// Generate a response stream
BSP_STRING * generate_http_response(BSP_HTTP_RESPONSE *resp);

// Parse a response to object
size_t parse_http_response(const char *data, ssize_t len, BSP_HTTP_RESPONSE *resp);

// Give response to HTML5 WebSocket handshake
int websocket_handshake(BSP_HTTP_REQUEST *req, BSP_HTTP_RESPONSE *resp);

// Parse WebSocket data(RFC6455)
size_t parse_websocket_data(const char *data, ssize_t len, int *opcode, BSP_STRING *data_str);

// Generate WebSocket data
BSP_STRING * generate_websocket_data(BSP_STRING *data, int opcode, int mask);

#endif  /* _LIB_BSP_CORE_HTTP_H */
