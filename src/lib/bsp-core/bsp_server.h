/*
 * bsp_server.h
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
 * @update 04/09/2013
 * @changelog 
 *      [06/01/2012] - Creation
 *      [04/09/2013] - New output functions
 */

#ifndef _LIB_BSP_CORE_SERVER_H

#define _LIB_BSP_CORE_SERVER_H
/* Headers */

/* Definations */
#define CLIENT_TYPE_DATA                        0x0
#define CLIENT_TYPE_WEBSOCKET_HANDSHAKE         0x1
#define CLIENT_TYPE_WEBSOCKET_DATA              0x2

#define DATA_TYPE_STREAM                        0x0
#define DATA_TYPE_PACKET                        0x1

#define PACKET_TYPE_REP                         0x0
#define PACKET_TYPE_RAW                         0x1
#define PACKET_TYPE_OBJ                         0x2
#define PACKET_TYPE_CMD                         0x3
#define PACKET_TYPE_HEARTBEAT                   0x7

#define LENGTH_TYPE_32B                         0x0
#define LENGTH_TYPE_64B                         0x1

#define SERIALIZE_TYPE_NATIVE                   0x0
#define SERIALIZE_TYPE_JSON                     0x1
#define SERIALIZE_TYPE_MSGPACK                  0x2
#define SERIALIZE_TYPE_AMF                      0x3

#define UDP_PACKET_REG                          0x1
#define UDP_PACKET_UNREG                        0x2
#define UDP_PACKET_CTRL                         0x3
#define UDP_PACKET_HB                           0x7F
#define UDP_PACKET_DATA                         0x0

// Callback type
#define SERVER_CALLBACK_ON_ACCEPT               0x80
#define SERVER_CALLBACK_ON_CONNECT              0x81
#define SERVER_CALLBACK_ON_CLOSE                0x82
#define SERVER_CALLBACK_ON_ERROR                0x83
#define SERVER_CALLBACK_ON_HEARTBEAT            0x84
#define SERVER_CALLBACK_ON_DATA_RAW             0x1
#define SERVER_CALLBACK_ON_DATA_OBJ             0x2
#define SERVER_CALLBACK_ON_DATA_CMD             0x3
#define SERVER_CALLBACK_ON_LOOP_START           0xC1
#define SERVER_CALLBACK_ON_LOOP_EXIT            0xC2
#define SERVER_CALLBACK_ON_LOOP_TIMER           0xC3

/* Macros */

/* Structs */
struct bsp_main_loop_t
{
    int                 loop_fd;
    int                 notify_fd;
    int                 exit_fd;
    int                 timer_fd;

    void                (*on_start) (unsigned long long);
    void                (*on_exit) (unsigned long long);
    void                (*on_timer) (unsigned long long);
};

typedef struct bsp_callback_t
{
    BSP_SERVER          *server;
    BSP_CLIENT          *client;
    int                 event;
    int                 cmd;
    BSP_STRING          *stream;
    BSP_OBJECT          *obj;
} BSP_CALLBACK;

/* Functions */
BSP_CLIENT * server_accept(BSP_SERVER *srv, struct sockaddr_storage *addr);

// Set main loop's event callback
// We support 3 types of events now : 
//  SERVER_CALLBACK_ON_LOOP_START - When loop start
//  SERVER_CALLBACK_ON_LOOP_EXIT  - When loop stop / exit
//  SERVER_CALLBACK_ON_LOOP_TIMER - When main timer(1Hz) triggered
//void set_loop_event(int event_type, void (* callback) (unsigned long long));

// Main loop of a server application
//int start_loop(void);
// Stop main loop
//int exit_loop(void);

// Add server to main loop
int add_server(BSP_SERVER *srv);
BSP_SERVER * get_server(const char *name);
size_t output_client_raw(BSP_CLIENT *clt, const char *data, ssize_t len);
size_t output_client_obj(BSP_CLIENT *clt, BSP_OBJECT *obj);
size_t output_client_cmd(BSP_CLIENT *clt, int cmd, BSP_OBJECT *obj);

#endif  /* _LIB_BSP_CORE_SERVER_H */
