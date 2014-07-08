/*
 * server.c
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
 * Network server runtime
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/24/2012
 * @changelog 
 *      [06/01/2012] - Creation
 *      [07/24/2012] - add_connector added      
 */

#include "bsp.h"

// One process can only have one loop
struct bsp_main_loop_t loop = {-1, -1, -1, -1, NULL, NULL, NULL};
size_t proc_data(BSP_CLIENT *clt, const char *data, ssize_t len);

// Accept a TCP client
BSP_CLIENT * server_accept(BSP_SERVER *srv, struct sockaddr_storage *addr)
{
    BSP_CLIENT *clt = new_client(srv, addr);

    if (clt)
    {
        // Default data type, we can modify them in on_accept event
        clt->client_type = srv->def_client_type;
        clt->data_type = srv->def_data_type;

        dispatch_to_thread(SFD(clt), STATIC_WORKER);
    }
    
    return clt;
}

// Add server to main loop
int add_server(BSP_SERVER *srv)
{
    if (srv)
    {
        // Default dataflow
        if (!srv->on_data)
        {
            srv->on_data = proc_data;
        }
        
        // Add into epoll
        dispatch_to_thread(SFD(srv), MAIN_THREAD);
        
        return BSP_RTN_SUCCESS;
    }

    return BSP_RTN_ERROR_GENERAL;
}
/*
// Delete server from main loop
int del_server(int srv_fd)
{
    if (loop.loop_fd < 0)
    {
        return -1;
    }

    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    BSP_SERVER *srv;
    int fd_type = FD_TYPE_SOCKET_SERVER;

    srv = (BSP_SERVER *) get_fd(srv_fd, &fd_type);
    if (srv)
    {
        epoll_ctl(loop.loop_fd, EPOLL_CTL_DEL, srv_fd, NULL);
        trace_msg(TRACE_LEVEL_DEBUG, "Server : Server %d remove from main loop", srv_fd);

        if (loop.notify_fd > 0)
        {
            // Send a signal
            write(loop.notify_fd, buff, 8);
        }
        return srv_fd;
    }

    return -1;
}
*/
// Server output
static size_t _real_output_client(BSP_CLIENT *clt, const char *data, ssize_t len)
{
    if (!clt)
    {
        return -1;
    }
    
    size_t slen = 0;
    
    if (clt->client_type == CLIENT_TYPE_DATA)
    {
        slen = append_data_socket(&SCK(clt), (const char *) data, (ssize_t) len);
    }
    else if (clt->client_type == CLIENT_TYPE_WEBSOCKET_DATA)
    {
        // WebSocket data
        BSP_STRING *ws_data = new_string(NULL, 0);
        if (ws_data)
        {
            generate_websocket_data((const char *) data, (ssize_t) len, WS_OPCODE_BINARY, 0, ws_data);
            slen = append_data_socket(&SCK(clt), (const char *) STR_STR(ws_data), STR_LEN(ws_data));
            del_string(ws_data);
        }
    }
    else
    {
        // Send nothing
    }
    
    flush_socket(&SCK(clt));
    
    return slen;
}

/* Output functions */
// Raw data, just put a header
size_t output_client_raw(BSP_CLIENT *clt, const char *data, ssize_t len)
{
    if (!clt)
    {
        return 0;
    }
    
    if (len < 0)
    {
        len = strlen(data);
    }
    
    BSP_STRING *stream = new_string(data, len);
    if (!stream)
    {
        return 0;
    }
    
    char placeholder[1] = {((PACKET_TYPE_RAW & 0b111) << 5) | ((clt->packet_length_type & 0b1) << 4) | ((clt->packet_serialize_type & 0b11) << 2) | (clt->packet_compress_type & 0b11)};
    char num_str[8];
    int ret = BSP_RTN_SUCCESS;
    size_t sent = 0;
    
    // If compress
    switch (clt->packet_compress_type)
    {
        case COMPRESS_TYPE_DEFLATE : 
            ret = string_compress_deflate(stream);
            break;
#ifdef ENABLE_LZ4
        case COMPRESS_TYPE_LZ4 : 
            ret = string_compress_lz4(stream);
            break;
#endif
#ifdef ENABLE_SNAPPY
        case COMPRESS_TYPE_SNAPPY : 
            ret = string_compress_snappy(stream);
            break;
#endif
        default : 
            break;
    }
    
    if (BSP_RTN_SUCCESS != ret)
    {
        // Compress failed
        trace_msg(TRACE_LEVEL_ERROR, "Server : Data compress failed");
    }
    
    BSP_STRING *str = new_string(NULL, 0);
    if (!str)
    {
        del_string(stream);
        return 0;
    }
    
    string_append(str, placeholder, 1);
    if (LENGTH_TYPE_32B == clt->packet_length_type)
    {
        set_int32((int32_t) STR_LEN(stream), num_str);
        string_append(str, (const char *) num_str, 4);
    }
    else
    {
        set_int64((int64_t) STR_LEN(stream), num_str);
        string_append(str, (const char *) num_str, 8);
    }
    
    string_append(str, STR_STR(stream), STR_LEN(stream));
    sent = _real_output_client(clt, STR_STR(str), STR_LEN(str));
    del_string(stream);
    del_string(str);

    return sent;
}

// Send an object
size_t output_client_obj(BSP_CLIENT *clt, BSP_OBJECT *obj)
{
    if (!clt || !obj)
    {
        return 0;
    }
    
    BSP_STRING *stream = new_string(NULL, 0);
    if (!stream)
    {
        return 0;
    }
    
    char placeholder[1] = {((PACKET_TYPE_OBJ & 0b111) << 5) | ((clt->packet_length_type & 0b1) << 4) | ((clt->packet_serialize_type & 0b11) << 2) | (clt->packet_compress_type & 0b11)};
    char num_str[8];
    int ret = BSP_RTN_SUCCESS;
    size_t sent = 0;
    
    // Pack data
    switch (clt->packet_serialize_type)
    {
        case SERIALIZE_TYPE_NATIVE : 
            object_serialize(obj, stream, clt->packet_length_type);
            break;
        case SERIALIZE_TYPE_JSON : 
            json_nd_encode(obj, stream);
            break;
        case SERIALIZE_TYPE_MSGPACK : 
            //msgpack_nd_encode(obj, stream);
            break;
        case SERIALIZE_TYPE_AMF : 
            //amf_nd_encode(obj, stream);
            break;
        default : 
            break;
    }
    
    // If compress
    switch (clt->packet_compress_type)
    {
        case COMPRESS_TYPE_DEFLATE : 
            ret = string_compress_deflate(stream);
            break;
#ifdef ENABLE_LZ4
        case COMPRESS_TYPE_LZ4 : 
            ret = string_compress_lz4(stream);
            break;
#endif
#ifdef ENABLE_SNAPPY
        case COMPRESS_TYPE_SNAPPY : 
            ret = string_compress_snappy(stream);
            break;
#endif
        case COMPRESS_TYPE_NONE : 
        default : 
            // No compress, do nothing
            break;
    }
    
    if (BSP_RTN_SUCCESS != ret)
    {
        // Compress failed
        trace_msg(TRACE_LEVEL_ERROR, "Server : Data compress failed");
    }
    
    BSP_STRING *str = new_string(NULL, 0);
    if (!str)
    {
        del_string(stream);
        return 0;
    }
    
    string_append(str, placeholder, 1);
    if (LENGTH_TYPE_32B == clt->packet_length_type)
    {
        set_int32((int32_t) STR_LEN(stream), num_str);
        string_append(str, (const char *) num_str, 4);
    }
    else
    {
        set_int64((int64_t) STR_LEN(stream), num_str);
        string_append(str, (const char *) num_str, 8);
    }
    
    string_append(str, STR_STR(stream), STR_LEN(stream));
    sent = _real_output_client(clt, STR_STR(str), STR_LEN(str));
    del_string(stream);
    del_string(str);
    
    return sent;
}

// Send command (Command ID + Params object)
size_t output_client_cmd(BSP_CLIENT *clt, int cmd, BSP_OBJECT *obj)
{
    if (!clt || !obj)
    {
        return 0;
    }
    
    BSP_STRING *stream = new_string(NULL, 0);
    if (!stream)
    {
        return 0;
    }
    
    char placeholder[1] = {((PACKET_TYPE_CMD & 0b111) << 5) | ((clt->packet_length_type & 0b1) << 4) | ((clt->packet_serialize_type & 0b11) << 2) | (clt->packet_compress_type & 0b11)};
    char num_str[8];
    set_int32((int32_t) cmd, num_str);
    string_append(stream, (const char *) num_str, 4);
    int ret = BSP_RTN_SUCCESS;
    size_t sent = 0;
    
    // Pack data
    switch (clt->packet_serialize_type)
    {
        case SERIALIZE_TYPE_NATIVE : 
            object_serialize(obj, stream, clt->packet_length_type);
            break;
        case SERIALIZE_TYPE_JSON : 
            json_nd_encode(obj, stream);
            break;
        case SERIALIZE_TYPE_MSGPACK : 
            //msgpack_nd_encode(obj, stream);
            break;
        case SERIALIZE_TYPE_AMF : 
            //amf_nd_encode(obj, stream);
            break;
        default : 
            break;
    }
    
    // If compress
    switch (clt->packet_compress_type)
    {
        case COMPRESS_TYPE_DEFLATE : 
            ret = string_compress_deflate(stream);
            break;
#ifdef ENABLE_LZ4
        case COMPRESS_TYPE_LZ4 : 
            ret = string_compress_lz4(stream);
            break;
#endif
#ifdef ENABLE_SNAPPY
        case COMPRESS_TYPE_SNAPPY : 
            ret = string_compress_snappy(stream);
            break;
#endif
        case COMPRESS_TYPE_NONE : 
        default : 
            // No compress, do nothing
            break;
    }
    
    if (BSP_RTN_SUCCESS != ret)
    {
        // Compress failed
        trace_msg(TRACE_LEVEL_ERROR, "Server : Data compress failed");
    }
    
    BSP_STRING *str = new_string(NULL, 0);
    if (!str)
    {
        del_string(stream);
        return 0;
    }
    
    string_append(str, placeholder, 1);
    if (LENGTH_TYPE_32B == clt->packet_length_type)
    {
        set_int32((int32_t) STR_LEN(stream), num_str);
        string_append(str, (const char *) num_str, 4);
    }
    else
    {
        set_int64((int64_t) STR_LEN(stream), num_str);
        string_append(str, (const char *) num_str, 8);
    }
    
    string_append(str, STR_STR(stream), STR_LEN(stream));
    sent = _real_output_client(clt, STR_STR(str), STR_LEN(str));
    del_string(stream);
    del_string(str);

    return sent;
}

/* Main data driver */
// Binary stream
static size_t _proc_stream(BSP_CLIENT *clt, const char *data, size_t len)
{
    int fd_type = FD_TYPE_SOCKET_SERVER;
    BSP_SERVER *srv = (BSP_SERVER *) get_fd(clt->srv_fd, &fd_type);
    
    if (!srv || !clt || !data)
    {
        return len;
    }
    
    size_t ret = 0;
    size_t remaining = len;
    const char *stream;
    size_t plen;
    BSP_STRING *str = NULL;
    BSP_OBJECT *obj = NULL;
    int cmd;
    char hdr;
    int p_type, l_type, s_type, c_type;
    
    switch (clt->data_type)
    {
        case DATA_TYPE_STREAM : 
            // Just through out all data
            if (srv->on_events)
            {
                srv->on_events(clt, SERVER_CALLBACK_ON_DATA_RAW, 0, (void *) data, len);
            }
            ret = len;
            break;
        case DATA_TYPE_PACKET : 
            while (remaining > 0)
            {
                stream = data + (len - remaining);
                remaining -= 1;
                hdr = stream[0];
                // One-byte header : 
                // | * * * | * | * * | * * |
                // First 3 bits         : packet type (RAW / OBJ / CMD)
                // Next 1 bits          : length type (32/64)
                // Following 2 bits     : Object serializa type (Native - BSP.Packet / Json / MsgPack / AMF)
                // last 2 bits          : Compression type (None / Zlib deflate / miniLZO / Google snappy)
                p_type = (hdr >> 5) & 0b111;
                l_type = (hdr >> 4) & 0b1;
                s_type = (hdr >> 2) & 0b11;
                c_type = (hdr) & 0b11;
                
                if (PACKET_TYPE_RAW == p_type || 
                    PACKET_TYPE_OBJ == p_type || 
                    PACKET_TYPE_CMD == p_type)
                {
                    // Data packet
                    if (LENGTH_TYPE_32B == l_type)
                    {
                        if (remaining < 4)
                        {
                            // Imperfect packet
                            break;
                        }
                        
                        plen = (size_t) get_int32(stream + 1);
                        remaining -= 4;
                        stream += 5;
                    }
                    else
                    {
                        if (remaining < 8)
                        {
                            // Imperfect packet
                            break;
                        }
                        
                        plen = (size_t) get_int64(stream + 1);
                        remaining -= 8;
                        stream += 9;
                    }
                    
                    if (srv->max_packet_length > 0 && srv->max_packet_length < plen)
                    {
                        // Packet too big
                        ret = len;
                        break;
                    }
                    
                    if (remaining < plen)
                    {
                        // Half data
                        break;
                    }
                    
                    str = new_string(NULL, 0);
                    if (!str)
                    {
                        break;
                    }
                    string_append(str, stream, plen);
                    str->compress_type = c_type;
                    
                    switch (c_type)
                    {
                        case COMPRESS_TYPE_DEFLATE : 
                            string_decompress_deflate(str);
                            break;
#ifdef ENABLE_LZ4
                        case COMPRESS_TYPE_LZ4 : 
                            string_decompress_lz4(str);
                            break;
#endif
#ifdef ENABLE_SNAPPY
                        case COMPRESS_TYPE_SNAPPY : 
                            string_decompress_snappy(str);
                            break;
#endif
                        case COMPRESS_TYPE_NONE : 
                        default : 
                            // Do nothing
                            break;
                    }
                    remaining -= plen;
                    
                    if (PACKET_TYPE_RAW == p_type)
                    {
                        // Lengthed string
                        if (srv->on_events)
                        {
                            srv->on_events(clt, SERVER_CALLBACK_ON_DATA_RAW, 0, STR_STR(str), STR_LEN(str));
                        }
                    }
                    else if (PACKET_TYPE_OBJ == p_type)
                    {
                        // Single object
                        obj = new_object();
                        if (obj)
                        {
                            switch (s_type)
                            {
                                case SERIALIZE_TYPE_NATIVE : 
                                    // BSP.Packet
                                    object_unserialize(STR_STR(str), STR_LEN(str), obj, l_type);
                                    break;
                                case SERIALIZE_TYPE_JSON : 
                                    // JSON
                                    json_nd_decode(STR_STR(str), STR_LEN(str), obj);
                                    break;
                                case SERIALIZE_TYPE_MSGPACK : 
                                    // MsgPack
                                    //msgpack_nd_decode(str->str, str->ori_len, obj);
                                    break;
                                case SERIALIZE_TYPE_AMF : 
                                    // Adobe AMF
                                    //amf_nd_decode(str->str, str->ori_len, obj);
                                    break;
                                default : 
                                    // Do nothing
                                    break;
                            }
                            
                            if (srv->on_events)
                            {
                                srv->on_events(clt, SERVER_CALLBACK_ON_DATA_OBJ, 0, obj, 0);
                            }
                            
                            del_object(obj);
                        }
                        else
                        {
                            trace_msg(TRACE_LEVEL_ERROR, "Server : Alloc output object error");
                        }
                    }
                    else
                    {
                        // Command (CmdID + params)
                        if (STR_LEN(str) >= 4)
                        {
                            cmd = (int) get_int32(STR_STR(str));
                            obj = new_object();
                            if (obj)
                            {
                                switch (s_type)
                                {
                                    case SERIALIZE_TYPE_NATIVE : 
                                        // BSP.Packet
                                        object_unserialize(STR_STR(str) + 4, STR_LEN(str) - 4, obj, l_type);
                                        break;
                                    case SERIALIZE_TYPE_JSON : 
                                        // JSON
                                        json_nd_decode(STR_STR(str) + 4, STR_LEN(str) - 4, obj);
                                        break;
                                    case SERIALIZE_TYPE_MSGPACK : 
                                        // MsgPack
                                        //msgpack_nd_decode(str->str + 4, str->ori_len - 4, obj);
                                        break;
                                    case SERIALIZE_TYPE_AMF : 
                                        // Adobe AMF
                                        //amf_nd_decode(str->str + 4, str->ori_len - 4, obj);
                                        break;
                                    default : 
                                        // Do nothing
                                        break;
                                }
                                
                                if (srv->on_events)
                                {
                                    srv->on_events(clt, SERVER_CALLBACK_ON_DATA_CMD, cmd, obj, 0);
                                }
                                
                                del_object(obj);
                            }
                            else
                            {
                                trace_msg(TRACE_LEVEL_ERROR, "Server : Alloc output object error");
                            }
                        }
                        else
                        {
                            trace_msg(TRACE_LEVEL_DEBUG, "Server : Ignore a sick packet");
                        }
                    }
                    
                    ret += 1 + (l_type = LENGTH_TYPE_32B ? 4 : 8) + plen;
                }
                else
                {
                    // Special
                    if (PACKET_TYPE_REP == p_type)
                    {
                        // Report compression and length mark type
                        clt->packet_length_type = l_type;
                        clt->packet_serialize_type = s_type;
                        clt->packet_compress_type = c_type;
                        
                        // Send back a response
                        _real_output_client(clt, stream, 1);
                        ret ++;
                    }
                    else if (PACKET_TYPE_HEARTBEAT == p_type)
                    {
                        // Heartbeat
                        // Send back a response
                        _real_output_client(clt, stream, 1);
                        ret ++;
                    }
                    else
                    {
                        // Unknown packet type, drop all
                        ret = len;
                        remaining = 0;
                    }
                }
                
                clt->last_hb_time = time(NULL);
            }
            break;
        default : 
            // Unknown type, ignore all data
            ret = len;
    }
    
    return ret;
}

// Default callback for general server
size_t proc_data(BSP_CLIENT *clt, const char *data, ssize_t len)
{
    if (!clt || !data)
    {
        return 0;
    }
    
    BSP_HTTP_REQUEST *req = NULL;
    BSP_HTTP_RESPONSE *resp = NULL;
    BSP_STRING *resp_str = NULL;
    BSP_STRING *data_str = NULL;
    size_t header_len = 0, data_len = 0;
    int ret, opcode = 0;
    
    switch (clt->client_type)
    {
        case CLIENT_TYPE_DATA : 
            if (len < 0)
            {
                len = strlen(data);
            }
            
            return _proc_stream(clt, data, len);
            break;
        case CLIENT_TYPE_WEBSOCKET_HANDSHAKE : 
            // Send handshake response
            req = new_http_request();
            resp = new_http_response();
            header_len = parse_http_request(data, len, req);
            
            if (header_len > 0)
            {
                ret = websocket_handshake(req, resp);
                if (BSP_RTN_SUCCESS == ret)
                {
                    trace_msg(TRACE_LEVEL_NOTICE, "A websocket client %d handshaked", SFD(clt));
                    // Send handshake back
                    resp_str = new_string(NULL, 0);
                    ret = generate_http_response(resp, resp_str);
                    if (BSP_RTN_SUCCESS == ret)
                    {
                        // Change client type
                        clt->client_type = CLIENT_TYPE_WEBSOCKET_DATA;
                        append_data_socket(&SCK(clt), STR_STR(resp_str), STR_LEN(resp_str));
                        flush_socket(&SCK(clt));
                        trace_msg(TRACE_LEVEL_NOTICE, "Websocket handshake from client %d responsed", SFD(clt));
                    }
                    else
                    {
                        // Rolling all data
                        header_len = len;
                    }
                    
                    del_string(resp_str);
                }
                else
                {
                    header_len = len;
                }
            }
            
            del_http_request(req);
            del_http_response(resp);
            
            return header_len;
            break;
        case CLIENT_TYPE_WEBSOCKET_DATA : 
            // Peel off the websocket shell
            data_str = new_string(NULL, 0);
            data_len = parse_websocket_data(data, len, &opcode, data_str);
            
            if (data_len > 0)
            {
                switch (opcode)
                {
                    case WS_OPCODE_TEXT : 
                    case WS_OPCODE_BINARY : 
                        // General data
                        _proc_stream(clt, (const char *) STR_STR(data_str), STR_LEN(data_str));
                        break;
                    case WS_OPCODE_PING : 
                        // Send a PONG back
                        resp_str = new_string(NULL, 0);
                        if (resp_str)
                        {
                            generate_websocket_data((const char *) STR_STR(data_str), STR_LEN(data_str), WS_OPCODE_PONG, 0, resp_str);
                            append_data_socket(&SCK(clt), (const char *) STR_STR(resp_str), STR_LEN(resp_str));
                            del_string(resp_str);
                            flush_socket(&SCK(clt));
                        }
                        
                        // Refresh heartbeat
                        trace_msg(TRACE_LEVEL_VERBOSE, "Server : Websocket client send ping");
                        clt->last_hb_time = time(NULL);
                        break;
                    case WS_OPCODE_PONG : 
                        // WTF ~~~ Why you send me a PONG ?
                        break;
                    case WS_OPCODE_CLOSE : 
                        // Send a CLOSE back
                        resp_str = new_string(NULL, 0);
                        if (resp_str)
                        {
                            generate_websocket_data((const char *) STR_STR(data_str), STR_LEN(data_str), WS_OPCODE_CLOSE, 0, resp_str);
                            append_data_socket(&SCK(clt), (const char *) STR_STR(resp_str), STR_LEN(resp_str));
                            del_string(resp_str);
                            flush_socket(&SCK(clt));
                        }
                        
                        // Close connection
                        trace_msg(TRACE_LEVEL_VERBOSE, "Server : Websocket client send close request");
                        free_client(clt);
                        break;
                    default : 
                        break;
                }
            }
            
            del_string(data_str);
                
            return data_len;
            break;
        default : 
            // Unknown client type
            return len;
            break;
    }
    return 0;
}
