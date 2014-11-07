/*
 * socket.c
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
 * Network server / connector/ client operator
 * TCP, UDP and UNIX pipe were supported in this branch
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/25/2013
 * @changelog 
 *      [05/30/2012] - Creation
 *      [06/04/2012] - Free client method
 *      [06/07/2012] - TCP client's try_read added
 *      [03/28/2013] - Uniform API
 *      [05/10/2013] - Initialization
 *      [06/24/2013] - Socket driven processor
 *      [12/10/2013] - Try read logic bug fixed
 */

#include "bsp.h"

// Initialization
int socket_init()
{
    return BSP_RTN_SUCCESS;
}

// Sets sockets' snd buffer size to the maximum value
static void maximize_udpbuf(const int fd)
{
    socklen_t intsize = sizeof(int);
    int min, max, avg;
    int old_size;
    
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : Getsockopt to FD %d error", fd);
        return;
    }

    min = avg = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max)
    {
        avg = ((unsigned int) (min + max)) / 2;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &avg, intsize) == 9)
        {
            min = avg + 1;
        }
        else
        {
            max = avg - 1;
        }
    }

    trace_msg(TRACE_LEVEL_DEBUG, "Socket : Set FD %d send buffer to %d", fd, avg);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &old_size, &intsize) != 0)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : Getsockopt to FD %d error", fd);
        return;
    }

    min = avg = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max)
    {
        avg = ((unsigned int) (min + max)) / 2;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &avg, intsize) == 9)
        {
            min = avg + 1;
        }
        else
        {
            max = avg - 1;
        }
    }

    trace_msg(TRACE_LEVEL_DEBUG, "Socket : Set FD %d recv buffer to %d", fd, avg);

    return;
}

static inline void _clear_socket(struct bsp_socket_t *sck)
{
    if (!sck)
    {
        return;
    }

    sck->read_block = NULL;
    sck->read_buffer_data_size = 0;
    sck->read_buffer_offset = 0;
    sck->read_buffer = NULL;
    sck->read_buffer_size = 0;
    sck->iov_list_sent = 0;
    size_t n;
    struct iovec *i;
    if (sck->iov_list && sck->iov_list_size)
    {
        // Clear all leaked iov
        for (n = 0; n < sck->iov_list_curr; n ++)
        {
            i = &sck->iov_list[n];
            if (i->iov_len && i->iov_base)
            {
                bsp_free(i->iov_base);
            }
        }
    }
    sck->iov_list_curr = 0;

    return;
}

static inline void _close_socket(struct bsp_socket_t *sck)
{
    if (!sck || !(sck->state & STATE_CLOSE))
    {
        return;
    }

    remove_from_thread(sck->fd);
    trace_msg(TRACE_LEVEL_NOTICE, "Socket : Try to close socket %d", sck->fd);
    if (sck->read_buffer)
    {
        bsp_free(sck->read_buffer);
    }

    _clear_socket(sck);
    if (sck->iov_list)
    {
        bsp_free(sck->iov_list);
    }
    sck->iov_list_size = 0;
    shutdown(sck->fd, SHUT_RDWR);
    unreg_fd(sck->fd);

    return;
}

static inline void _init_socket(struct bsp_socket_t *sck, int fd, struct sockaddr_storage *saddr, struct addrinfo *addr)
{
    if (!sck)
    {
        return;
    }

    _clear_socket(sck);
    
    if (saddr && addr)
    {
        memcpy(&sck->saddr, saddr, sizeof(struct sockaddr_storage));
        memcpy(&sck->addr, addr, sizeof(struct addrinfo));
        sck->addr.ai_addr = (struct sockaddr *) &sck->saddr;
    }

    sck->addr.ai_next = NULL;
    sck->fd = fd;
    sck->ev.data.fd = fd;
    sck->ev.events = EPOLLIN | EPOLLET;
#ifdef EPOLLRDHUP
    // Linux kernel >= 2.6.17
    // Add EPOLLRDHUP to detect peer close on edge triggered
    sck->ev.events |= EPOLLRDHUP;
#endif
    sck->conn_time = time(NULL);
    sck->state = STATE_IDLE;
    bsp_spin_init(&sck->send_lock);

    return;
}

static char * _append_read_buffer(struct bsp_socket_t *sck, const char *data, size_t len)
{
    if (!sck || !data)
    {
        return NULL;
    }

    char *ret = NULL;
    if (sck->read_buffer_data_size + len > sck->read_buffer_size)
    {
        // Enlarge read buffer
        size_t newsize = 2 << (int) log2(sck->read_buffer_data_size + len);
        char *newbuff = bsp_realloc(sck->read_buffer, newsize);
        if (!newbuff)
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Enlarge socket read buffer error");
            return NULL;
        }

        trace_msg(TRACE_LEVEL_DEBUG, "Socket : Socket %d's read buffer enlarge to %d", sck->fd, (int) newsize);
        sck->read_buffer = newbuff;
        sck->read_buffer_size = newsize;
    }

    ret = sck->read_buffer + sck->read_buffer_data_size;
    memcpy(ret, data, len);
    sck->read_buffer_data_size += len;
    trace_msg(TRACE_LEVEL_DEBUG, "Socket : Appended %d bytes to socket %d's read buffer", (int) len, sck->fd);

    return ret;
}

static inline ssize_t _try_read_socket(struct bsp_socket_t *sck)
{
    if (!sck || !sck->read_block)
    {
        return 0;
    }

    ssize_t len, tlen = 0;
    while (1)
    {
        len = read(sck->fd, sck->read_block, READ_ONCE);
        if (len < 0)
        {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // Go on
                continue;
            }
            else
            {
                // Read error
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Read socket %d error", sck->fd);
                sck->state |= STATE_PRECLOSE;
                break;
            }
        }
        else if (len == 0)
        {
            if (IS_UDP(sck))
            {
                // UDP accept null packet
            }
            else
            {
                // TCP FIN
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : Socket %d FIN", sck->fd);
                sck->state |= STATE_PRECLOSE;
                break;
            }
        }
        else
        {
            // Normal data
            trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Read %d bytes from socket %d", (int) len, sck->fd);
            _append_read_buffer(sck, (const char *) sck->read_block, len);
            tlen += len;

            if (len < READ_ONCE)
            {
                // All data gone
                break;
            }
        }
    }

    return tlen;
}

static inline ssize_t _try_send_socket(struct bsp_socket_t *sck)
{
    if (!sck || !(sck->state & STATE_WRITE))
    {
        return 0;
    }

    size_t n;
    // Generate MsgHdr
    struct msghdr m;
    struct iovec *i = NULL;
    size_t msg_size = 0;
    bsp_spin_lock(&sck->send_lock);
    memset(&m, 0, sizeof(struct msghdr));
    m.msg_iov = sck->iov_list + sck->iov_list_sent;

    for (n = sck->iov_list_sent; n < sck->iov_list_curr; n ++)
    {
        i = &sck->iov_list[n];
        if (IS_UDP(sck) && (msg_size + i->iov_len) > UDP_PACKET_MAX_LEN)
        {
            break;
        }

        if (m.msg_iovlen >= IOV_MAX - 1)
        {
            break;
        }

        msg_size += i->iov_len;
        m.msg_iovlen ++;
    }

    ssize_t len = sendmsg(sck->fd, &m, 0);
    if (len < 0)
    {
        // Send error
        bsp_spin_unlock(&sck->send_lock);
        trace_msg(TRACE_LEVEL_DEBUG, "Socket : Send data from socket %d error", sck->fd);
        return len;
    }

    // Count IOV
    ssize_t leftover = len;
    void *new;
    for (n = 0; n < m.msg_iovlen; n ++)
    {
        i = &m.msg_iov[n];
        if (leftover >= i->iov_len)
        {
            // IOV complete
            leftover -= i->iov_len;
            if (i->iov_base)
            {
                bsp_free(i->iov_base);
                memset(i, 0, sizeof(struct iovec));
            }
        }
        else
        {
            // Mark uncomplete
            new = bsp_malloc(i->iov_len - leftover);
            if (!new)
            {
                // Clear current iov
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Create new send buffer error");
                bsp_free(i->iov_base);
                memset(i, 0, sizeof(struct iovec));
                n ++;
            }
            else
            {
                memcpy(new, i->iov_base + leftover, (i->iov_len - leftover));
                i->iov_base = new;
                i->iov_len -= leftover;
            }
            
            break;
        }
    }
    sck->iov_list_sent += n;
    trace_msg(TRACE_LEVEL_DEBUG, "Socket : Sent %d bytes to client %d", (int) len, sck->fd);
    if (sck->iov_list_sent >= sck->iov_list_curr)
    {
        // All data sent, clear data
        sck->iov_list_sent = 0;
        sck->iov_list_curr = 0;
        trace_msg(TRACE_LEVEL_DEBUG, "Socket : All data in socket %d sent off", sck->fd);

        // Update epoll event
        sck->state &= ~STATE_WRITE;
        
        // Test closing mark
        if (sck->state & STATE_PRECLOSE)
        {
            sck->state |= STATE_CLOSE;
        }
        else
        {
            sck->ev.events &= ~EPOLLOUT;
        }
    }
    bsp_spin_unlock(&sck->send_lock);
    modify_fd_events(sck->fd, &sck->ev);

    return len;
}

static inline struct iovec * _new_iovec(struct bsp_socket_t *sck)
{
    if (!sck)
    {
        return NULL;
    }

    while (sck->iov_list_curr >= sck->iov_list_size)
    {
        // Enlarge list
        size_t newlist_size = (0 == sck->iov_list_size) ? IOV_LIST_INITIAL : sck->iov_list_size * 2;
        struct iovec *newlist = bsp_realloc(sck->iov_list, sizeof(struct iovec) * newlist_size);
        if (!newlist)
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Enlarge iovec list error");
            return NULL;
        }

        sck->iov_list = newlist;
        sck->iov_list_size = newlist_size;
        trace_msg(TRACE_LEVEL_DEBUG, "Socket : Enlarge socket %d's iov list to length %d", sck->fd, (int) sck->iov_list_size);
    }

    struct iovec *i = &sck->iov_list[sck->iov_list_curr ++];
    memset(i, 0, sizeof(struct iovec));

    return i;
}

/*
 * Main event processor of socket
 */
int drive_socket(struct bsp_socket_t *sck)
{
    if (!sck)
    {
        return 0;
    }

    int fd_type = FD_TYPE_ANY;
    void *ptr = get_fd(sck->fd, &fd_type);

    if (!ptr)
    {
        return 0;
    }

    BSP_SERVER *srv = NULL;
    BSP_CLIENT *clt = NULL;
    BSP_CONNECTOR *cnt = NULL;
    BSP_CORE_SETTING *settings = get_core_setting();
    BSP_CALLBACK cb;

    if (FD_TYPE_SOCKET_CLIENT == fd_type)
    {
        clt = (BSP_CLIENT *) ptr;
        srv = get_client_connected_server(clt);
    }
    else if (FD_TYPE_SOCKET_CONNECTOR == fd_type)
    {
        cnt = (BSP_CONNECTOR *) ptr;
    }
    else
    {
        // Wrong fd type
        return 0;
    }

    ssize_t len, cblen;
    if (sck->state & STATE_ERROR)
    {
        if (srv)
        {
            trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Server %d ON_ERROR triggered", SFD(srv));
            if (settings->on_srv_events)
            {
                cb.server = srv;
                cb.client = clt;
                cb.event = SERVER_CALLBACK_ON_ERROR;
                settings->on_srv_events(&cb);
            }
        }
    }

    if (sck->state & STATE_CLOSE)
    {
        if (srv)
        {
            srv->nclients --;
            status_op_socket(SFD(srv), STATUS_OP_SOCKET_SERVER_DISCONNECT, 0);
            trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Server %d ON_CLOSE triggered", SFD(srv));
            if (settings->on_srv_events)
            {
                cb.server = srv;
                cb.client = clt;
                cb.event = SERVER_CALLBACK_ON_CLOSE;
                settings->on_srv_events(&cb);
            }
        }
        else if (cnt && cnt->on_close)
        {
            status_op_socket(0, STATUS_OP_SOCKET_CONNECTOR_DISCONNECT, 0);
            trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Connector %d ON_CLOSE triggered", SFD(cnt));
            cnt->on_close(cnt);
        }

        // Close socket immedialy
        trace_msg(TRACE_LEVEL_DEBUG, "Socket : Closing socket %d", sck->fd);
        _close_socket(sck);
        bsp_free(ptr);

        return 0;
    }

    // Try read
    if (sck->state & STATE_READ)
    {
        len = _try_read_socket(sck);
        if (len > 0)
        {
            if (FD_TYPE_SOCKET_CLIENT == fd_type)
            {
                // Client
                if (srv)
                {
                    if (srv->debug_hex_input && !settings->is_daemonize)
                    {
                        debug_str("Client %d read data ...\n", sck->fd);
                        debug_hex(SOCKET_RDATA(clt), SOCKET_RLEN(clt));
                    }
                    status_op_socket(SFD(srv), STATUS_OP_SOCKET_SERVER_READ, len);
                    if (srv->on_data)
                    {
                        trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Server %d ON_DATA triggered", SFD(srv));
                        do
                        {
                            cblen = srv->on_data(clt, SOCKET_RDATA(clt), SOCKET_RLEN(clt));
                            if (cblen <= 0)
                            {
                                break;
                            }
                            trace_msg(TRACE_LEVEL_VERBOSE, "Socket : %d bytes in client %d proceeded", cblen, sck->fd);
                            SOCKET_RPASS(clt, cblen)
                        } while (SOCKET_RSIZE(clt));
                    }
                    else
                    {
                        SOCKET_RPASSALL(clt);
                    }
                }
                else
                {
                    SOCKET_RPASSALL(clt)
                }
            }
            else
            {
                // Connector
                if (settings->debug_hex_connector_input && !settings->is_daemonize)
                {
                    debug_str("Connector %d read data ...\n", sck->fd);
                    debug_hex(SOCKET_RDATA(cnt), SOCKET_RLEN(cnt));
                }
                status_op_socket(0, STATUS_OP_SOCKET_CONNECTOR_READ, len);
                if (cnt->on_data)
                {
                    trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Connector %d ON_DATA triggered", SFD(cnt));
                    do
                    {
                        cblen = cnt->on_data(cnt, SOCKET_RDATA(cnt), SOCKET_RLEN(cnt));
                        if (cblen <= 0)
                        {
                            break;
                        }
                        trace_msg(TRACE_LEVEL_VERBOSE, "Socket : %d bytes in connector %d proceeded", cblen, sck->fd);
                        SOCKET_RPASS(cnt, cblen);
                    } while (SOCKET_RSIZE(cnt));
                }
                else
                {
                    SOCKET_RPASSALL(cnt)
                }
            }
        }
        sck->state &= ~STATE_READ;
    }

    // Try send
    if (sck->state & STATE_WRITE)
    {
        len = _try_send_socket(sck);
        if (0 > len)
        {
            sck->state |= STATE_PRECLOSE;
        }
        else
        {
            if (FD_TYPE_SOCKET_CLIENT == fd_type)
            {
                if (srv)
                {
                    status_op_socket(SFD(srv), STATUS_OP_SOCKET_SERVER_SENT, len);
                }
            }
            else if (FD_TYPE_SOCKET_CONNECTOR == fd_type)
            {
                status_op_socket(0, STATUS_OP_SOCKET_CONNECTOR_SENT, len);
            }
        }

        sck->state &= ~STATE_WRITE;
    }

    // Deal with closing
    if (sck->state & STATE_PRECLOSE)
    {
        // Want close
        bsp_spin_lock(&sck->send_lock);
        if (sck->iov_list_sent >= sck->iov_list_curr)
        {
            // Nothing to send
            trace_msg(TRACE_LEVEL_DEBUG, "Socket : Try close socket %d", sck->fd);
            sck->state |= STATE_CLOSE;
        }
        bsp_spin_unlock(&sck->send_lock);
        flush_socket(sck);
    }

    return 0;
}

// Append data to socket's send buffer
size_t append_data_socket(struct bsp_socket_t *sck, BSP_STRING *data)
{
    if (!sck || !data)
    {
        return 0;
    }

    size_t leftover = STR_LEN(data);
    size_t append;
    void *buf;
    struct iovec *iov = NULL;
    BSP_CORE_SETTING *settings = get_core_setting();
    if (settings->debug_hex_output && !settings->is_daemonize)
    {
        debug_str("Appendding data to socket %d ...\n", sck->fd);
        debug_hex(STR_STR(data), STR_LEN(data));
    }

    bsp_spin_lock(&sck->send_lock);
    if (IS_UDP(sck))
    {
        // explode into MTU
        while (leftover > 0)
        {
            append = (leftover > UDP_PACKET_MAX_LEN) ? UDP_PACKET_MAX_LEN : leftover;
            buf = bsp_malloc(append);
            if (!buf)
            {
                bsp_spin_unlock(&sck->send_lock);
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Alloc send buffer error");
                return STR_LEN(data) - leftover;
            }
            memcpy(buf, STR_STR(data) + (STR_LEN(data) - leftover), append);
            iov = _new_iovec(sck);
            if (!iov)
            {
                bsp_free(buf);
                bsp_spin_unlock(&sck->send_lock);
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Create new IOV error");
                return STR_LEN(data) - leftover;
            }
            leftover -= append;
            iov->iov_base = buf;
            iov->iov_len = append;
        }
    }
    else
    {
        buf = bsp_malloc(STR_LEN(data));
        if (!buf)
        {
            bsp_spin_unlock(&sck->send_lock);
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Alloc send buffer error");
            return 0;
        }
        memcpy(buf, STR_STR(data), STR_LEN(data));
        iov = _new_iovec(sck);
        if (!iov)
        {
            bsp_free(buf);
            bsp_spin_unlock(&sck->send_lock);
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Create new IOV error");
            return 0;
        }
        iov->iov_base = buf;
        iov->iov_len = STR_LEN(data);
    }

    bsp_spin_unlock(&sck->send_lock);
    trace_msg(TRACE_LEVEL_DEBUG, "Socket : Append %d byte to socket %d's send buffer", (int) STR_LEN(data), sck->fd);

    return STR_LEN(data);
}

// If any data in send buffer(IOV), try send all
int flush_socket(struct bsp_socket_t *sck)
{
    // Ready to send
    if (!sck)
    {
        return -1;
    }

    // Update epoll event
    sck->ev.events |= EPOLLOUT;
    modify_fd_events(sck->fd, &sck->ev);
    
    return 0;
}

// UNIX local domain server
static BSP_SERVER * _new_unix_server(const char *path, int access_mask)
{
    BSP_SERVER *srv = NULL;
    int flags = 1;
    int old_umask;
    struct stat tstat;
    struct sockaddr_un addr;
    struct linger ling = {0, 0};
    if (!path)
    {
        return NULL;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd || BSP_RTN_SUCCESS != set_fd_nonblock(fd))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : Create UNIX local socket error");
        return NULL;
    }

    // Clean up previous socket file
    if (0 == lstat(path, &tstat))
    {
        if (S_ISSOCK(tstat.st_mode))
        {
            unlink(path);
        }
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &flags, sizeof(flags));
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &flags, sizeof(flags));
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &ling, sizeof(ling));
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    old_umask = umask(~(access_mask & 0777));
    if (-1 == bind(fd, (struct sockaddr *) &addr, sizeof(addr)))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : Bind error on unix local domain %s", path);
        close(fd);
        umask(old_umask);
        return NULL;
    }
    umask(old_umask);
    if (-1 == listen(fd, settings->tcp_listen_backlog))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : UNIX local server listen error");
        close(fd);
        return NULL;
    }

    srv = bsp_calloc(1, sizeof(BSP_SERVER));
    if (!srv)
    {
        trace_msg(TRACE_LEVEL_FATAL, "Socket : Server alloc failed");
        _exit(BSP_RTN_ERROR_MEMORY);
    }

    memset(&srv->sck, 0, sizeof(struct bsp_socket_t));
    _init_socket(&srv->sck, fd, NULL, NULL);
    srv->nclients = 0;
    srv->on_data = NULL;
    srv->def_client_type = 0;
    srv->def_data_type = 0;
    reg_fd(fd, FD_TYPE_SOCKET_SERVER, (void *) srv);
    status_op_socket(0, STATUS_OP_SOCKET_SERVER_ADD, fd);
    trace_msg(TRACE_LEVEL_DEBUG, "Socket : UNIX local server created on path %s", path);

    return srv;
}

// Create new socket servers based on given address:port and other conditions.
int new_server(
               const char *addr, 
               int port, 
               int inet_type, 
               int sock_type, 
               int *fds, 
               int *nfds
               )
{
    int fd;
    int total = 0;
    int ret;
    int flag = 1;
    char port_str[9] = {0, 0, 0, 0, 0, 0, 0, 0};
    char ipaddr[64];
    struct linger ling = {0, 0};
    struct addrinfo *ai;
    struct addrinfo *next;
    struct addrinfo hints;
    BSP_SERVER *srv = NULL;

    if (*nfds < 1)
    {
        return 0;
    }

    if (INET_TYPE_LOCAL == inet_type)
    {
        // UNIX local domain
        // Address regards as socket path, port as access mask
        srv = _new_unix_server(addr, port);
        if (srv)
        {
            fds[0] = SFD(srv);
            total = 1;
        }

        return total;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;

    switch (inet_type)
    {
        case INET_TYPE_IPV4 : 
            hints.ai_family = AF_INET;
            break;
        case INET_TYPE_IPV6 : 
            hints.ai_family = AF_INET6;
            break;
        case INET_TYPE_ANY : 
        default : 
            // We do not support AF_LOCAL here
            hints.ai_family = AF_UNSPEC;
            break;
    }

    switch (sock_type)
    {
        case SOCK_TYPE_ANY : 
            // TCP & UDP
            hints.ai_socktype = 0;
            break;
        case SOCK_TYPE_TCP : 
            hints.ai_socktype = SOCK_STREAM;
            break;
        case SOCK_TYPE_UDP : 
            hints.ai_socktype = SOCK_DGRAM;
            break;
        default : 
            break;
    }

    snprintf(port_str, 7, "%d", port);
    ret = getaddrinfo(addr, port_str, &hints, &ai);
    if (ret != 0)
    {
        if (ret != EAI_SYSTEM)
        {
            // gai_strerror(error);
            trace_msg(TRACE_LEVEL_ERROR, "Socket : GetAddrInfo error : %s", gai_strerror(ret));
        }
        else
        {
            // getaddrinfo error
            trace_msg(TRACE_LEVEL_ERROR, "Socket : GetAddrInfo unknown error");
        }
        
        return total;
    }

    for (next = ai; next; next = next->ai_next)
    {
        if (AF_LOCAL == next->ai_family)
        {
            // We do not support AF_LOCAL at this time.
            continue;
        }

        fd = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
        if (-1 == fd || BSP_RTN_SUCCESS != set_fd_nonblock(fd))
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Create socket error");
            continue;
        }

        if (AF_INET == next->ai_family || AF_INET6 == next->ai_family)
        {
            // Network socket
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &flag, sizeof(flag));
            if (AF_INET == next->ai_family)
            {
                // IPv4
                struct sockaddr_in *srv_sin = (struct sockaddr_in *) next->ai_addr;
                inet_ntop(AF_INET, &srv_sin->sin_addr.s_addr, ipaddr, 63);
            }
            else
            {
                // IPv6
                struct sockaddr_in6 *srv_sin6 = (struct sockaddr_in6 *) next->ai_addr;
                inet_ntop(AF_INET6, &srv_sin6->sin6_addr.s6_addr, ipaddr, 63);
            }

            if (SOCK_STREAM == next->ai_socktype)
            {
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : Try to create a TCP server on %s:%d", ipaddr, port);
                // TCP
                if (0 != setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &flag, sizeof(flag)) || 
                    0 != setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &ling, sizeof(ling)) || 
                    0 != setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(flag)) || 
                    0 != setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &flag, sizeof(flag)))
                {
                    // Setsockopt error
                    trace_msg(TRACE_LEVEL_ERROR, "Socket : TCP SetSockOpt error");
                    close(fd);
                    continue;
                }
            }
            else if (SOCK_DGRAM == next->ai_socktype)
            {
                // UDP
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : Try to create an UDP server on %s:%d", ipaddr, port);
                maximize_udpbuf(fd);
            }
            else
            {
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Try to create a server with unsupported sock type");
                close(fd);
                continue;
            }

            // Bind
            // Don't worry about the length of sockaddr, It's sockaddr_storage in fact.
            if (-1 == bind(fd, next->ai_addr, next->ai_addrlen))
            {
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Bind error on %d@%d", port, sock_type);
                close(fd);
                continue;
            }

            if (SOCK_STREAM == next->ai_socktype && -1 == listen(fd, settings->tcp_listen_backlog))
            {
                // Listen for TCP server
                trace_msg(TRACE_LEVEL_ERROR, "Socket : TCP server listen error");
                close(fd);
                continue;
            }

            trace_msg(TRACE_LEVEL_CORE, "Socket : Network server created on %d@%d", port, sock_type);
        }
        else
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Try to create a server with unsupported type");
            close(fd);
            continue;
        }

        srv = bsp_calloc(1, sizeof(BSP_SERVER));
        if (!srv)
        {
            trigger_exit(BSP_RTN_ERROR_MEMORY, "Socket : Server alloc failed");
        }

        _init_socket(&srv->sck, fd, (struct sockaddr_storage *) next->ai_addr, next);

        srv->name = NULL;
        srv->nclients = 0;
        srv->on_data = NULL;
        srv->def_client_type = 0;
        srv->def_data_type = 0;
        reg_fd(fd, FD_TYPE_SOCKET_SERVER, (void *) srv);
        status_op_socket(0, STATUS_OP_SOCKET_SERVER_ADD, fd);

        total ++;
        if (total >= *nfds)
        {
            trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Too many servers one time");
            break;
        }

        fds[total - 1] = fd;
    }

    *nfds = total;
    freeaddrinfo(ai);

    return total;
}

// Create a new client
BSP_CLIENT * new_client(BSP_SERVER *srv, struct sockaddr_storage *clt_addr)
{
    int fd = 0;
    BSP_CLIENT *clt = NULL;
    char ipaddr[64];
    socklen_t addrlen;

    clt = bsp_calloc(1, sizeof(BSP_CLIENT));
    if (!clt)
    {
        return NULL;
    }

    if (clt_addr)
    {
        memcpy(&clt->sck.saddr, clt_addr, sizeof(struct sockaddr_storage));
    }

    clt->srv_fd = -1;
    
    if (IS_UDP((&srv->sck)))
    {
        // An UDP server, make a new socket
        fd = socket(srv->sck.addr.ai_family, srv->sck.addr.ai_socktype, srv->sck.addr.ai_protocol);
        if (fd < 0)
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : UDP fake fd failed");
            bsp_free(clt);
            return NULL;
        }
        else
        {
            // Bind to a random port
            if (AF_INET6 == clt->sck.saddr.ss_family)
            {
                struct sockaddr_in6 vaddr6;
                if (0 != bind(fd, (struct sockaddr *) &vaddr6, sizeof(struct sockaddr_in6)))
                {
                    close(fd);
                    trace_msg(TRACE_LEVEL_ERROR, "Socket : UDP IPv6 virtual port bind error");
                    bsp_free(clt);
                    return NULL;
                }
                
                addrlen = sizeof(struct sockaddr_in6);
                getsockname(fd, (struct sockaddr *) &vaddr6, &addrlen);
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : UDP IPv6 virtual port bind to %d", ntohs(vaddr6.sin6_port));
            }
            else
            {
                struct sockaddr_in vaddr4;
                if (0 != bind(fd, (struct sockaddr *) &vaddr4, sizeof(struct sockaddr_in)))
                {
                    close(fd);
                    trace_msg(TRACE_LEVEL_ERROR, "Socket : UDP IPv4 virtual port bind error");
                    bsp_free(clt);
                    return NULL;
                }
                
                addrlen = sizeof(struct sockaddr_in);
                getsockname(fd, (struct sockaddr *) &vaddr4, &addrlen);
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : UDP IPv4 virtual port bind to %d", ntohs(vaddr4.sin_port));
            }

            // Connect
            if (0 != connect(fd, (struct sockaddr *) &clt->sck.saddr, sizeof(struct sockaddr_storage)))
            {
                close(fd);
                trace_msg(TRACE_LEVEL_ERROR, "Socket : UDP virtual port connect error");
                bsp_free(clt);
                return NULL;
            }

            if (AF_INET6 == clt->sck.saddr.ss_family)
            {
                // IPv6
                struct sockaddr_in6 *clt_sin6 = (struct sockaddr_in6 *) &clt->sck.saddr;
                inet_ntop(AF_INET6, &clt_sin6->sin6_addr.s6_addr, ipaddr, 63);
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : IPv6 UDP client connected from %s : %d", ipaddr, ntohs(clt_sin6->sin6_port));
            }
            else
            {
                // IPv4
                struct sockaddr_in *clt_sin4 = (struct sockaddr_in *) &clt->sck.saddr;
                inet_ntop(AF_INET, &clt_sin4->sin_addr.s_addr, ipaddr, 63);
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : IPv4 UDP client connected from %s : %d", ipaddr, ntohs(clt_sin4->sin_port));
            }
        }
    }
    else
    {
        // TCP server
        trace_msg(TRACE_LEVEL_NOTICE, "Socket : A TCP client try to accept by server");
        socklen_t len = sizeof(clt->sck.addr);
        fd = accept(SFD(srv), (struct sockaddr *) &clt->sck.saddr, &len);
        
        if (-1 == fd)
        {
            // Accept error
            trace_msg(TRACE_LEVEL_ERROR, "Socket : TCP Accept failed");
            bsp_free(clt);
            return NULL;
        }
        else
        {
            set_fd_nonblock(fd);
            if (AF_INET6 == clt->sck.saddr.ss_family)
            {
                // IPv6
                struct sockaddr_in6 *clt_sin6 = (struct sockaddr_in6 *) &clt->sck.saddr;
                inet_ntop(AF_INET6, &clt_sin6->sin6_addr.s6_addr, ipaddr, 63);
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : IPv6 TCP client connected from %s : %d", ipaddr, ntohs(clt_sin6->sin6_port));
            }
            else
            {
                // IPv4
                struct sockaddr_in *clt_sin4 = (struct sockaddr_in *) &clt->sck.saddr;
                inet_ntop(AF_INET, &clt_sin4->sin_addr.s_addr, ipaddr, 63);
                trace_msg(TRACE_LEVEL_DEBUG, "Socket : IPv4 TCP client connected from %s : %d", ipaddr, ntohs(clt_sin4->sin_port));
            }
        }
    }

    if (fd > 0)
    {
        _init_socket(&SCK(clt), fd, NULL, NULL);
        clt->sck.addr.ai_family = srv->sck.addr.ai_family;
        clt->sck.addr.ai_socktype = srv->sck.addr.ai_socktype;
        clt->sck.addr.ai_protocol = srv->sck.addr.ai_protocol;
        clt->sck.addr.ai_addrlen = srv->sck.addr.ai_addrlen;
        clt->srv_fd = SFD(srv);
        clt->client_type = 0;
        clt->data_type = 0;
        clt->additional = NULL;
        bsp_spin_init(&clt->script_stack.lock);

        reg_fd(fd, FD_TYPE_SOCKET_CLIENT, (void *) clt);
        status_op_socket(SFD(srv), STATUS_OP_SOCKET_SERVER_CONNECT, 0);
    }

    return clt;
}

// Get client's parasitifer server
BSP_SERVER * get_client_connected_server(BSP_CLIENT *clt)
{
    BSP_SERVER *srv;
    
    if (!clt)
    {
        return NULL;
    }

    int fd_type = FD_TYPE_SOCKET_SERVER;
    void *ptr = get_fd(clt->srv_fd, &fd_type);
    srv = (BSP_SERVER *) ptr;
    
    return srv;
}

// Close a client and free resource
int free_client(BSP_CLIENT *clt)
{
    if (!clt)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    SCK(clt).state |= STATE_PRECLOSE;
    trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Set client %d as closing", SFD(clt));

    return BSP_RTN_SUCCESS;
}

/* Connect to UNIX local domain */
static BSP_CONNECTOR * _new_unix_connector(const char *path)
{
    BSP_CONNECTOR * cnt = NULL;
    int flags = 1;
    struct stat tstat;
    struct sockaddr_un addr;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd || BSP_RTN_SUCCESS != set_fd_nonblock(fd))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : Create UNIX local socket error");
        return NULL;
    }

    if (0 != lstat(path, &tstat) || !S_ISSOCK(tstat.st_mode))
    {
        // Not sock file
        return NULL;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &flags, sizeof(flags));
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (0 != connect(fd, (struct sockaddr *) &addr, SUN_LEN(&addr)))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : UNIX local connector connect error : %s", strerror(errno));
        close(fd);
        return NULL;
    }

    cnt = bsp_calloc(1, sizeof(BSP_CONNECTOR));
    if (!cnt)
    {
        trace_msg(TRACE_LEVEL_FATAL, "Socket : Connector alloc failed");
        _exit(BSP_RTN_ERROR_MEMORY);
    }
    _init_socket(&cnt->sck, fd, NULL, NULL);
    bsp_spin_init(&cnt->script_stack.lock);

    reg_fd(fd, FD_TYPE_SOCKET_CONNECTOR, (void *) cnt);
    trace_msg(TRACE_LEVEL_DEBUG, "Socket : UNIX connector %d connected to %s", fd, path);

    return cnt;
}

/* Connector */
BSP_CONNECTOR * new_connector(const char *addr, int port, int inet_type, int sock_type)
{
    int fd;
    int error;
    int flag = 1;
    char port_str[9] = {0, 0, 0, 0, 0, 0, 0, 0};
    struct linger ling = {0, 0};
    struct addrinfo *ai;
    struct addrinfo hints;
    BSP_CONNECTOR *cnt = NULL;

    hints.ai_flags = AI_V4MAPPED;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;

    if (INET_TYPE_LOCAL == inet_type)
    {
        cnt = _new_unix_connector(addr);
        return cnt;
    }

    switch (inet_type)
    {
        case INET_TYPE_IPV6 : 
            hints.ai_family = AF_INET6;
            break;
        default : 
            // Default connector based on IPv4
            hints.ai_family = AF_INET;
            break;
    }

    switch (sock_type)
    {
        case SOCK_TYPE_UDP : 
            hints.ai_socktype = SOCK_DGRAM;
            break;
        default : 
            // Default TCP now.
            // Any else? We do not support SCTP now
            hints.ai_socktype = SOCK_STREAM;
            break;
    }

    snprintf(port_str, 7, "%d", port);
    error = getaddrinfo(addr, port_str, &hints, &ai);
    if (error != 0)
    {
        if (error != EAI_SYSTEM)
        {
            // gai_strerror(error);
            trace_msg(TRACE_LEVEL_ERROR, "Socket : GetAddrInfo error : %s", gai_strerror(error));
        }
        else
        {
            // getaddrinfo error
            trace_msg(TRACE_LEVEL_ERROR, "Socket : GetAddrInfo unknown error");
        }

        return NULL;
    }

    if (ai)
    {
        // I just give you ONE connector at one time
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (-1 == fd)
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Create socket error");
            freeaddrinfo(ai);
            return NULL;
        }

        if (AF_INET == ai->ai_family || AF_INET6 == ai->ai_family)
        {
            // Network socket
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &flag, sizeof(flag));
            if (SOCK_STREAM == ai->ai_socktype)
            {
                trace_msg(TRACE_LEVEL_NOTICE, "Socket : Try to create a TCP connector to %s:%d", addr, port);
                // TCP
                if (0 != setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &flag, sizeof(flag)) || 
                    0 != setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &ling, sizeof(ling)) || 
                    0 != setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(flag)))
                {
                    // Setsockopt error
                    trace_msg(TRACE_LEVEL_ERROR, "Socket : TCP SetSockOpt error");
                    close(fd);
                    freeaddrinfo(ai);
                    return NULL;
                }

                // Connect
                if (0 != connect(fd, ai->ai_addr, ai->ai_addrlen))
                {
                    trace_msg(TRACE_LEVEL_ERROR, "Socket : TCP connector connect error : %s", strerror(errno));
                    close(fd);
                    freeaddrinfo(ai);
                    return NULL;
                }
            }
            else if (SOCK_DGRAM == ai->ai_socktype)
            {
                // UDP
                trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Try to create an UDP connector to %s:%d", addr, port);
                maximize_udpbuf(fd);
            }
            else
            {
                trace_msg(TRACE_LEVEL_ERROR, "Socket : Try to create a server with unsupported sock type");
                close(fd);
                freeaddrinfo(ai);
                return NULL;
            }
        }
        else
        {
            trace_msg(TRACE_LEVEL_ERROR, "Socket : Try to connect to an unsupported server");
            close(fd);
            freeaddrinfo(ai);
            return NULL;
        }

        cnt = bsp_calloc(1, sizeof(BSP_CONNECTOR));
        if (!cnt)
        {
            trace_msg(TRACE_LEVEL_FATAL, "Socket : Connector alloc failed");
            _exit(BSP_RTN_ERROR_MEMORY);
        }
        set_fd_nonblock(fd);
        _init_socket(&cnt->sck, fd, (struct sockaddr_storage *) ai->ai_addr, ai);
        bsp_spin_init(&cnt->script_stack.lock);

        reg_fd(fd, FD_TYPE_SOCKET_CONNECTOR, (void *) cnt);
        status_op_socket(0, STATUS_OP_SOCKET_CONNECTOR_CONNECT, 0);
        trace_msg(TRACE_LEVEL_DEBUG, "Socket : Connector %d connected to %s:%d", fd, addr, port);
        freeaddrinfo(ai);

        // We have no on_connect event here, just do it on your mind
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Socket : No address infomation available");
    }

    return cnt;
}

// Close and free an connector
int free_connector(BSP_CONNECTOR *cnt)
{
    if (!cnt)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    SCK(cnt).state |= STATE_PRECLOSE;
    trace_msg(TRACE_LEVEL_VERBOSE, "Socket : Set connector %d as closing", SFD(cnt));

    return BSP_RTN_SUCCESS;
}
