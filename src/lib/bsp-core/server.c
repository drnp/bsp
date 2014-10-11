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
 *      [07/10/2014] - 
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
            ws_data = generate_websocket_data((const char *) data, (ssize_t) len, WS_OPCODE_BINARY, 0);
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

    char placeholder[1] = {((PACKET_TYPE_RAW & 0b111) << 5) | ((clt->packet_serialize_type & 0b111) << 2) | (clt->packet_compress_type & 0b11)};
    char num_str[9];
    int ret = BSP_RTN_SUCCESS;
    size_t sent = 0;
    BSP_STRING *stream = (COMPRESS_TYPE_NONE == clt->packet_compress_type) ? new_string_const(data, len) : new_string(data, len);
    if (!stream)
    {
        return 0;
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
            break;
    }

    if (BSP_RTN_SUCCESS != ret)
    {
        // Compress failed
        trace_msg(TRACE_LEVEL_ERROR, "Server : Data compress failed");
    }

    BSP_STRING *str = new_string(placeholder, 1);
    if (!str)
    {
        del_string(stream);
        return 0;
    }

    int num_len = set_vint((int64_t) STR_LEN(stream), num_str);
    string_append(str, (const char *) num_str, (ssize_t) num_len);
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

    char placeholder[1] = {((PACKET_TYPE_OBJ & 0b111) << 5) | ((clt->packet_serialize_type & 0b111) << 2) | (clt->packet_compress_type & 0b11)};
    char num_str[9];
    int ret = BSP_RTN_SUCCESS;
    size_t sent = 0;
    BSP_STRING *stream = NULL;

    // Pack data
    switch (clt->packet_serialize_type)
    {
        case SERIALIZE_TYPE_NATIVE : 
            stream = object_serialize(obj);
            break;
        case SERIALIZE_TYPE_JSON : 
            stream = json_nd_encode(obj);
            break;
        case SERIALIZE_TYPE_MSGPACK : 
            //stream = msgpack_nd_encode(obj);
            break;
        case SERIALIZE_TYPE_AMF : 
            //stream = amf_nd_encode(obj);
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

    BSP_STRING *str = new_string(placeholder, 1);
    if (!str)
    {
        del_string(stream);
        return 0;
    }

    int num_len = set_vint((int64_t) STR_LEN(stream), num_str);
    string_append(str, (const char *) num_str, (ssize_t) num_len);
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

    char placeholder[1] = {((PACKET_TYPE_CMD & 0b111) << 5) | ((clt->packet_serialize_type & 0b111) << 2) | (clt->packet_compress_type & 0b11)};
    char num_str[9];
    set_int32((int32_t) cmd, num_str);
    int ret = BSP_RTN_SUCCESS;
    size_t sent = 0;
    BSP_STRING *stream = new_string((const char *) num_str, 4);
    if (!stream)
    {
        return 0;
    }

    // Pack data
    BSP_STRING *data = NULL;
    switch (clt->packet_serialize_type)
    {
        case SERIALIZE_TYPE_NATIVE : 
            data = object_serialize(obj);
            break;
        case SERIALIZE_TYPE_JSON : 
            data = json_nd_encode(obj);
            break;
        case SERIALIZE_TYPE_MSGPACK : 
            //data = msgpack_nd_encode(obj);
            break;
        case SERIALIZE_TYPE_AMF : 
            //data = amf_nd_encode(obj);
            break;
        default : 
            break;
    }

    if (data)
    {
        string_append(stream, STR_STR(data), STR_LEN(data));
        del_string(data);
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

    BSP_STRING *str = new_string(placeholder, 1);
    if (!str)
    {
        del_string(stream);
        return 0;
    }

    int num_len = set_vint((int64_t) STR_LEN(stream), num_str);
    string_append(str, (const char *) num_str, (ssize_t) num_len);
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
    char hdr;
    int p_type, s_type, c_type;

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
                // | * * * | * * * | * * |
                // First 3 bits         : packet type (RAW / OBJ / CMD)
                // Following 3 bits     : Object serializa type (Native - BSP.Packet / Json / MsgPack / AMF / ...)
                // last 2 bits          : Compression type (None / Zlib deflate / miniLZO / Google snappy)
                p_type = (hdr >> 5) & 0b111;
                s_type = (hdr >> 2) & 0b111;
                c_type = (hdr) & 0b11;

                if (PACKET_TYPE_RAW == p_type || 
                    PACKET_TYPE_OBJ == p_type || 
                    PACKET_TYPE_CMD == p_type)
                {
                    // Data packet
                    int safe = (int) remaining;
                    plen = get_vint(stream + 1, &safe);
                    if (safe < 0)
                    {
                        // Imperfect packet
                        break;
                    }
                    remaining -= safe;
                    stream += safe + 1;

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

                    str = (COMPRESS_TYPE_NONE == c_type) ? new_string_const(stream, plen) : new_string(stream, plen);
                    if (!str)
                    {
                        break;
                    }
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
                        switch (s_type)
                        {
                            case SERIALIZE_TYPE_NATIVE : 
                                // BSP.Packet
                                obj = object_unserialize(str);
                                break;
                            case SERIALIZE_TYPE_JSON : 
                                // JSON
                                obj = json_nd_decode(str);
                                break;
                            case SERIALIZE_TYPE_MSGPACK : 
                                // MsgPack
                                //obj = msgpack_nd_decode(str);
                                break;
                            case SERIALIZE_TYPE_AMF : 
                                // Adobe AMF
                                //obj = amf_nd_decode(str);
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
                        // Command (CmdID + params)
                        if (STR_LEN(str) >= 4)
                        {
                            int cmd = (int) get_int32(STR_STR(str));
                            BSP_STRING *body = new_string_const(STR_STR(str) + 4, STR_LEN(str) - 4);
                            if (body)
                            {
                                switch (s_type)
                                {
                                    case SERIALIZE_TYPE_NATIVE : 
                                        // BSP.Packet
                                        obj = object_unserialize(body);
                                        break;
                                    case SERIALIZE_TYPE_JSON : 
                                        // JSON
                                        obj = json_nd_decode(body);
                                        break;
                                    case SERIALIZE_TYPE_MSGPACK : 
                                        // MsgPack
                                        //obj = msgpack_nd_decode(body);
                                        break;
                                    case SERIALIZE_TYPE_AMF : 
                                        // Adobe AMF
                                        //obj = amf_nd_decode(body);
                                        break;
                                    default : 
                                        // Do nothing
                                        break;
                                }
                                del_string(body);
                            }

                            if (srv->on_events)
                            {
                                srv->on_events(clt, SERVER_CALLBACK_ON_DATA_CMD, cmd, obj, 0);
                            }

                            del_object(obj);
                        }
                        else
                        {
                            trace_msg(TRACE_LEVEL_DEBUG, "Server : Ignore a sick packet");
                        }
                    }
                    del_string(str);
<<<<<<< HEAD
                    ret += 1 + (l_type = LENGTH_TYPE_32B ? 4 : 8) + plen;
=======
                    ret += 1 + safe + plen;
>>>>>>> dev
                }
                else
                {
                    // Special
                    if (PACKET_TYPE_REP == p_type)
                    {
                        // Report serialize and compression
                        clt->packet_serialize_type = s_type;
                        clt->packet_compress_type = c_type;
                        trace_msg(TRACE_LEVEL_VERBOSE, "Server : Client %d report as serialize type : %d, compress type : %d", SFD(clt), s_type, c_type);
                        // Send back a response
                        _real_output_client(clt, stream, 1);
                        ret ++;
                    }
                    else if (PACKET_TYPE_HEARTBEAT == p_type)
                    {
                        // Heartbeat
                        // Send back a response
                        trace_msg(TRACE_LEVEL_VERBOSE, "Server : Client %d send a heartbeat request", SFD(clt));
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
                    trace_msg(TRACE_LEVEL_NOTICE, "Server : A websocket client %d handshaked", SFD(clt));
                    // Send handshake back
                    resp_str = generate_http_response(resp);
                    if (resp_str)
                    {
                        // Change client type
                        clt->client_type = CLIENT_TYPE_WEBSOCKET_DATA;
                        append_data_socket(&SCK(clt), STR_STR(resp_str), STR_LEN(resp_str));
                        flush_socket(&SCK(clt));
                        trace_msg(TRACE_LEVEL_NOTICE, "Server : Websocket handshake from client %d responsed", SFD(clt));
                        del_string(resp_str);
                    }
                    else
                    {
                        // Rolling all data
                        header_len = len;
                    }
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
                        resp_str = generate_websocket_data((const char *) STR_STR(data_str), STR_LEN(data_str), WS_OPCODE_PONG, 0);
                        append_data_socket(&SCK(clt), (const char *) STR_STR(resp_str), STR_LEN(resp_str));
                        del_string(resp_str);
                        flush_socket(&SCK(clt));
                        // Refresh heartbeat
                        trace_msg(TRACE_LEVEL_VERBOSE, "Server : Websocket client send ping");
                        clt->last_hb_time = time(NULL);
                        break;
                    case WS_OPCODE_PONG : 
                        // WTF ~~~ Why you send me a PONG ?
                        break;
                    case WS_OPCODE_CLOSE : 
                        // Send a CLOSE back
                        resp_str = generate_websocket_data((const char *) STR_STR(data_str), STR_LEN(data_str), WS_OPCODE_CLOSE, 0);
                        append_data_socket(&SCK(clt), (const char *) STR_STR(resp_str), STR_LEN(resp_str));
                        del_string(resp_str);
                        flush_socket(&SCK(clt));
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

