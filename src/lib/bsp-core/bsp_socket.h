/*
 * socket.h
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
 * Socket header of libbsp-core
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/16/2013
 * @changelog 
 *      [05/30/2012] - Creation
 *      [06/07/2012] - TCP client try_read added
 *      [09/28/2012] - Client type added
 *      [09/28/2012] - Connection time added
 *      [10/16/2013] - max_packet_length added
 */

#ifndef _LIB_BSP_CORE_SOCKET_H

#define _LIB_BSP_CORE_SOCKET_H
/* Headers */
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "bsp_spinlock.h"

/* Definations */
#ifndef __need_IOV_MAX
#define __need_IOV_MAX
#endif

#ifndef IOV_MAX
#define IOV_MAX                                 1024
#endif

#define SOCK_TYPE_ANY                           0
#define SOCK_TYPE_TCP                           1
#define SOCK_TYPE_UDP                           2

#define INET_TYPE_ANY                           0
#define INET_TYPE_IPV4                          1
#define INET_TYPE_IPV6                          2
#define INET_TYPE_LOCAL                         3

#define DEFAULT_TCP_LISTEN_BACKLOG              1024
#define ALLOC_CLIENT_BLOCK                      1024
#define MAX_SENDBUF_SIZE                        256 * 1024 * 1024
#define READ_BUFFER_INITIAL                     4096
#define READ_BUFFER_HIGHWAT                     256 * 1024 * 1024
#define READ_ONCE                               262144

#define UDP_PACKET_MAX_LEN                      520
#define IOV_LIST_INITIAL                        32
#define MSG_LIST_INITIAL                        8

#define SOCKET_MODE_NEW                         0x0
#define SOCKET_MODE_USED                        0x1

/* Macros */
#define SCK(o)                                  o->sck
#define SFD(o)                                  o->sck.fd
#define SOCKET_RDATA(o)                         o->sck.read_buffer + o->sck.read_buffer_offset
#define SOCKET_RSIZE(o)                         o->sck.read_buffer_data_size
#define SOCKET_RLEN(o)                          o->sck.read_buffer_data_size - o->sck.read_buffer_offset
#define SOCKET_RPASS(o, l)                      o->sck.read_buffer_offset += l; \
                                                if (o->sck.read_buffer_offset >= o->sck.read_buffer_data_size) { \
                                                    o->sck.read_buffer_offset = 0; \
                                                    o->sck.read_buffer_data_size = 0; \
                                                }
#define SOCKET_RPASSALL(o)                      o->sck.read_buffer_offset = 0; \
                                                o->sck.read_buffer_data_size = 0;
#define IS_UDP(sck)                             sck->addr.ai_socktype == SOCK_DGRAM

#define STATE_IDLE                              0b0
#define STATE_LISTENING                         0b1
#define STATE_CONNECTING                        0b10
#define STATE_READ                              0b100
#define STATE_WRITE                             0b1000
#define STATE_PRECLOSE                          0b1000000
#define STATE_CLOSE                             0b10000000

/* Structs */
struct bsp_socket_t
{
    // Summaries
    int                 fd;
    struct sockaddr_storage
                        saddr;
    struct addrinfo     addr;
    struct epoll_event  ev;

    // State
    int                 state;

    // Buffers
    char                *read_block;
    char                *read_buffer;
    size_t              read_buffer_size;
    size_t              read_buffer_data_size;
    size_t              read_buffer_offset;
    
    // UIO
    struct iovec        *iov_list;
    ssize_t             iov_list_size;
    ssize_t             iov_list_sent;
    ssize_t             iov_list_curr;

    time_t              conn_time;

    // Lock
    BSP_SPINLOCK        send_lock;
};

typedef struct bsp_connector_t
{
    // Summaries
    struct bsp_socket_t sck;
    time_t              last_hb_time;
    
    // Callbacks
    void                (* on_close) (struct bsp_connector_t *cnt);
    size_t              (* on_data) (struct bsp_connector_t *cnt, const char *data, ssize_t len);

    // For other object
    void                *additional;

    // UDP protocol
    int32_t             udp_proto;
} BSP_CONNECTOR;

typedef struct bsp_client_t
{
    // Summaries
    int                 srv_fd;
    struct bsp_socket_t sck;
    time_t              last_hb_time;

    // For other object
    void                *additional;

    // Connection type (Raw / WebSocket)
    int                 client_type;

    // Data type (Stream / Packet)
    int                 data_type;

    // UDP Protocol
    int32_t             udp_proto;

    // Packet
    int                 packet_length_type;
    int                 packet_serialize_type;
    int                 packet_compress_type;
    int                 packet_heartbeat;
} BSP_CLIENT;

typedef struct bsp_server_t
{
    // Summaries
    struct bsp_socket_t sck;
    
    // Callbacks
    size_t              (* on_data) (BSP_CLIENT *clt, const char *data, ssize_t len);
    void                (* on_events) (BSP_CLIENT *clt, int callback, int cmd, void *data, ssize_t len);

    // Status
    size_t              nclients;

    // Client default behavior
    int                 heartbeat_check;
    int                 def_client_type;
    int                 def_data_type;
    size_t              max_packet_length;
    size_t              max_clients;

    // Other entry
    void                *additional;
} BSP_SERVER;

/* Functions */
// Initialization
int socket_init();

// Create new socket servers based on given address:port and other conditions.

// If addr is null, IN6ADDR_ANY_INIT(for IPv6) and INADDR_ANY(IPv4) will be used as listener(AI_PASSIVE).
// If inet_type is INET_TYPE_ANY, both IPv6 and IPv4 will be tried to create.
// If sock_type set as SOCK_TYPE_ANY, both TCP and UDP socket will be created.
// If sock_type set as SOCK_TYPE_LOCAL, addr must be set as a valid sock file path, port and inet_type will be ignored.
// The number of servers created successfully will be returned, and fds holds the server list(file descriptors).
// The maxnium number of servers specialed by nfds.
// Only numerical port will be accepted in this branch.
int new_server(const char *addr, int port, int inet_type, int sock_type, int *fds, int *nfds);

// Create a new client

// Both ai_family and ai_socktype were implementated from server
BSP_CLIENT * new_client(BSP_SERVER *srv, struct sockaddr_storage *clt_addr);

// Get client's parasitifer server
BSP_SERVER * get_client_connected_server(BSP_CLIENT *clt);

// Close a client and free resources
int free_client(BSP_CLIENT *clt);

// Create a new connector
// All the four parameters must be set as a non-zero value!
// INET_TYPE_ANY and SOCK_TYPE_ANY will be treated as INET_TYPE_IPV4 and SOCK_TYPE_TCP.
// If sock_type set as SOCK_TYPE_LOCAL, addr must be set as a valid sock file path, port and inet_type will be ignored.
BSP_CONNECTOR * new_connector(const char *addr, int port, int inet_type, int sock_type);

// Close a connector
int free_connector(BSP_CONNECTOR *cnt);

// Append data to socket
size_t append_data_socket(struct bsp_socket_t *sck, const char *data, ssize_t len);

// Try send data
size_t send_data_socket(struct bsp_socket_t *sck);

// Send packages, All data before me will generated as a splited msg
int flush_socket(struct bsp_socket_t *sck);

// Main processor
int drive_socket(struct bsp_socket_t *sck);

#endif  /* _LIB_BSP_CORE_SOCKET_H */
