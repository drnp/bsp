/*
 * thread.c
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
 * Static worker pool and dynamic task threads handler
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/15/2012
 * @changelog 
 *      [06/04/2012] - Creation
 *      [06/15/2012] - modify_fd_events method added
 *      [06/03/2014] - Normalize main thread
 */

#include "bsp.h"

BSP_THREAD main_thread;
BSP_THREAD *static_worker_pool = NULL;
size_t static_worker_total = 0;
int init_count = 0;

pthread_mutex_t init_lock;
pthread_cond_t init_cond;
pthread_key_t lid_key;
int curr_static_worker;

/* Functions */
int create_worker(BSP_THREAD *t);

// Init static worker list and dispatcher
int thread_init()
{
    BSP_CORE_SETTING *settings = get_core_setting();
    if (static_worker_pool)
    {
        return BSP_RTN_SUCCESS;
    }

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);
    static_worker_total = settings->static_workers;
    static_worker_pool = bsp_calloc(static_worker_total, sizeof(BSP_THREAD));
    if (!static_worker_pool)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "Thread : Thread list alloc failed");
    }

    int i;
    BSP_THREAD *worker;
    pthread_key_create(&lid_key, NULL);
    curr_static_worker = 0;
    for (i = 0; i < static_worker_total; i ++)
    {
        worker = &static_worker_pool[i];
        worker->id = i;
        create_worker(worker);
    }

    // Waiting for all workers
    pthread_mutex_lock(&init_lock);
    while (init_count < static_worker_total)
    {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);
    trace_msg(TRACE_LEVEL_CORE, "Thread : Thread list initialized as %d static workers", static_worker_total);

    // Main thread
    main_thread.id = -1;
    create_worker(&main_thread);
    pthread_setspecific(lid_key, (void *) &main_thread.id);

    return BSP_RTN_SUCCESS;
}

// Create a thread
int create_worker(BSP_THREAD *t)
{
    if (!t)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    // Create runner
    t->script_runner.state = script_new_state();
    if (!t->script_runner.state)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Thread create runner error");
        return BSP_RTN_ERROR_SCRIPT;
    }
    bsp_spin_init(&t->script_runner.lock);
    //script_new_stack(&t->script_runner);
    BSP_CORE_SETTING *settings = get_core_setting();
    t->loop_fd = epoll_create(settings->epoll_wait_conns);
    if (-1 == t->loop_fd)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Thread create epollfd error");
        return BSP_RTN_ERROR_EPOLL;
    }

    t->notify_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (-1 == t->notify_fd)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Thread create eventfd error");
        close(t->loop_fd);
        return BSP_RTN_ERROR_EVENTFD;
    }

    t->exit_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (-1 == t->exit_fd)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Thread create exitfd error");
        close(t->loop_fd);
        close(t->notify_fd);
        return BSP_RTN_ERROR_EVENTFD;
    }

    reg_fd(t->loop_fd, FD_TYPE_EPOLL, NULL);
    reg_fd(t->notify_fd, FD_TYPE_EVENT, NULL);
    reg_fd(t->exit_fd, FD_TYPE_EXIT, NULL);

    // Add notify to epoll
    struct epoll_event ev_notify;
    ev_notify.data.fd = t->notify_fd;
    ev_notify.events = EPOLLIN;
    epoll_ctl(t->loop_fd, EPOLL_CTL_ADD, t->notify_fd, &ev_notify);

    // Add exit to epoll
    struct epoll_event ev_exit;
    ev_exit.data.fd = t->exit_fd;
    ev_exit.events = EPOLLIN;
    epoll_ctl(t->loop_fd, EPOLL_CTL_ADD, t->exit_fd, &ev_exit);

    t->nfds = 0;
    //bsp_spin_init(&t->fd_lock);

    if (t->id >= 0)
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (0 != pthread_create(&t->pid, &attr, thread_process, (void *) t))
        {
            trigger_exit(BSP_RTN_ERROR_PTHREAD, "Thread : Create thread error");
        }
        trace_msg(TRACE_LEVEL_CORE, "Thread : Thread %lld created as %d", (unsigned long long) t->pid, t->id);
    }
    else
    {
        t->pid = MAIN_THREAD_PID;
        trace_msg(TRACE_LEVEL_CORE, "Thread : Main thread initialized");
    }

    return BSP_RTN_SUCCESS;
}

// Static thread loop
void * thread_process(void *arg)
{
    BSP_THREAD *me = (BSP_THREAD *) arg;
    if (!me)
    {
        return NULL;
    }

    pthread_mutex_lock(&init_lock);
    init_count ++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

    BSP_CORE_SETTING *settings = get_core_setting();
    struct epoll_event events[settings->epoll_wait_conns];
    int nfds, i, what, fd_type;
    int stop = 0;
    void *ptr;
    BSP_SERVER *srv = NULL;
    BSP_CLIENT *clt = NULL;
    BSP_CONNECTOR *cnt = NULL;
    BSP_TIMER *tmr = NULL;
    BSP_CALLBACK cb;
    struct sockaddr_storage udp_from;
    socklen_t udp_addrlen = sizeof(udp_from);
    static char udp_buff[16384];
    ssize_t udp_dlen = 0;
    int udp_proto;
    //char udp_resp[4];
    char notify_buff[8];
    uint64_t timer;
    pthread_setspecific(lid_key, (void *) &me->id);

    while (1)
    {
        //memset(events, 0, sizeof(struct epoll_event) * settings->epoll_wait_conns);
        nfds = epoll_wait(me->loop_fd, events, settings->epoll_wait_conns, -1);
        for (i = 0; i < nfds; i ++)
        {
            what = events[i].events;
            fd_type = FD_TYPE_ANY;
            ptr = get_fd(events[i].data.fd, &fd_type);
            switch (fd_type)
            {
                case FD_TYPE_EVENT : 
                    // A new fd dispatch here
                    if (8 != read(me->notify_fd, notify_buff, 8))
                    {
                        // Eventfd error
                        trace_msg(TRACE_LEVEL_ERROR, "Thread : Thread %d read notify error", me->pid);
                    }
                    if (notify_buff[7] == 127)
                    {
                        // GC event
                        trace_msg(TRACE_LEVEL_NOTICE, "Thread : Thread %d script GC triggered", me->id);
                        lua_gc(me->script_runner.state, LUA_GCCOLLECT, 0);
                    }
                    break;
                case FD_TYPE_EXIT : 
                    read(me->exit_fd, notify_buff, 8);
                    stop = 1;
                    break;
                case FD_TYPE_SOCKET_SERVER : 
                    srv = (BSP_SERVER *) ptr;
                    if (srv && SFD(srv) == events[i].data.fd)
                    {
                        if (what & EPOLLIN)
                        {
                            // New connection
                            if (srv->max_clients > 0 && srv->max_clients <= srv->nclients)
                            {
                                // Server full
                                trace_msg(TRACE_LEVEL_ERROR, "Thread : Server %d full", SFD(srv));
                                break;
                            }
                            if (SOCK_STREAM == srv->sck.addr.ai_socktype)
                            {
                                // Accept new client for TCP or Local servers
                                clt = server_accept(srv, NULL);
                                if (!clt)
                                {
                                    break;
                                }
                                clt->packet_serialize_type = (CLIENT_TYPE_WEBSOCKET_HANDSHAKE == clt->client_type) ? SERIALIZE_TYPE_JSON : SERIALIZE_TYPE_NATIVE;
                                clt->packet_compress_type = COMPRESS_TYPE_NONE;
                                clt->last_hb_time = time(NULL);
                                if (settings->on_srv_events)
                                {
                                    trace_msg(TRACE_LEVEL_VERBOSE, "Thread : Server %d ON_ACCEPT event triggered", srv->sck.fd);
                                    cb.server = srv;
                                    cb.client = clt;
                                    cb.event = SERVER_CALLBACK_ON_ACCEPT;
                                    settings->on_srv_events(&cb);
                                }
                            }
                            else
                            {
                                // UDP, read data from socket
                                udp_dlen = recvfrom(srv->sck.fd, udp_buff, 16384, 0, (struct sockaddr *) &udp_from, &udp_addrlen);
                                if (udp_dlen >= 4)
                                {
                                    udp_proto = get_int32(udp_buff);
                                    if (udp_proto == settings->udp_proto_main || udp_proto == settings->udp_proto_status)
                                    {
                                        clt = server_accept(srv, &udp_from);
                                        if (!clt)
                                        {
                                            break;
                                        }
                                        // Send response
                                        //set_int32(udp_proto, udp_resp);
                                        //append_data_socket(&clt->sck, udp_resp, 4);
                                        //flush_socket(&clt->sck);
                                        if (settings->on_srv_events)
                                        {
                                            trace_msg(TRACE_LEVEL_VERBOSE, "Thread : Server %d ON_ACCEPT event triggered", srv->sck.fd);
                                            cb.server = srv;
                                            cb.client = clt;
                                            cb.event = SERVER_CALLBACK_ON_CONNECT;
                                            settings->on_srv_events(&cb);
                                        }
                                    }
                                    else
                                    {
                                        // Invalid UDP proto);
                                    }
                                }
                                
                                if (udp_dlen < 0)
                                {
                                    // Error
                                }
                            }

                            srv->nclients ++;
                        }
                        else if (what & EPOLLOUT)
                        {
                            // We cannot write data to a server
                        }
                        else
                        {
                            // oooW~~
                        }
                    }
                    break;
                case FD_TYPE_SOCKET_CLIENT : 
                    clt = (BSP_CLIENT *) ptr;
                    if (clt && SFD(clt) == events[i].data.fd)
                    {
                        if (what & EPOLLIN)
                        {
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Client %d try read", SFD(clt));
                            clt->sck.state |= STATE_READ;
                        }
                        if (what & EPOLLOUT)
                        {
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Client %d try send", SFD(clt));
                            clt->sck.state |= STATE_WRITE;
                        }
                        if (what & EPOLLHUP)
                        {
                            // Connection hang up
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Client %d socket hang up", SFD(clt));
                            clt->sck.state |= STATE_CLOSE;
                        }
#ifdef EPOLLRDHUP
                        if (what & EPOLLRDHUP)
                        {
                            // Peer close
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Client %d connection peer closed", SFD(clt));
                            clt->sck.state |= STATE_PRECLOSE;
                        }
#endif
                        if (what & EPOLLERR)
                        {
                            trace_msg(TRACE_LEVEL_ERROR, "Thread : Connection error on socket %d", SFD(clt));
                            clt->sck.state |= STATE_PRECLOSE;
                            clt->sck.state |= STATE_ERROR;
                        }
                        
                        drive_socket(&SCK(clt));
                    }
                    break;
                case FD_TYPE_SOCKET_CONNECTOR : 
                    cnt = (BSP_CONNECTOR *) ptr;
                    if (cnt && SFD(cnt) == events[i].data.fd)
                    {
                        if (what & EPOLLIN)
                        {
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Connector %d try read", SFD(cnt));
                            cnt->sck.state |= STATE_READ;
                        }
                        if (what & EPOLLOUT)
                        {
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Connector %d try send", SFD(cnt));
                            cnt->sck.state |= STATE_WRITE;
                        }
                        if (what & EPOLLHUP)
                        {
                            // Connection hang up
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Connector %d socket hang up", SFD(cnt));
                            cnt->sck.state |= STATE_CLOSE;
                        }
#ifdef EPOLLRDHUP
                        if (what & EPOLLRDHUP)
                        {
                            // Peer close
                            trace_msg(TRACE_LEVEL_NOTICE, "Thread : Connector %d connection peer closed", SFD(cnt));
                            cnt->sck.state |= STATE_PRECLOSE;
                        }
#endif
                        if (what & EPOLLERR)
                        {
                            trace_msg(TRACE_LEVEL_ERROR, "Thread : Connection error on socket %d", SFD(cnt));
                            cnt->sck.state |= STATE_PRECLOSE;
                            cnt->sck.state |= STATE_ERROR;
                        }

                        drive_socket(&SCK(cnt));
                    }
                    break;
                case FD_TYPE_TIMER : 
                    tmr = (BSP_TIMER *) ptr;
                    if (tmr)
                    {
                        read(tmr->fd, notify_buff, 8);
                        memcpy(&timer, notify_buff, 8);
                        tmr->timer += timer;
                        if (tmr->on_timer)
                        {
                            tmr->on_timer(tmr);
                        }
                        if (tmr->loop > 0)
                        {
                            if (-- tmr->loop == 0)
                            {
                                // Need stop
                                if (tmr->on_stop)
                                {
                                    tmr->on_stop(tmr);
                                }
                                free_timer(tmr);
                            }
                        }
                        status_op_timer(STATUS_OP_TIMER_TRIGGER);
                    }
                    break;
                default : 
                    break;
            }
        }
        if (stop)
        {
            trace_msg(TRACE_LEVEL_DEBUG, "Thread : Thread %d exited", me->pid);
            break;
        }
    }
    return NULL;
}

// Find the most idle worker
inline static int _next_static_worker()
{
    int id = (curr_static_worker ++) % static_worker_total;
    if (curr_static_worker >= static_worker_total)
    {
        curr_static_worker = 0;
    }

    return id;
}

// Insert a fd to dispatcher, add it to main thread or worker, send a signal to its loop event
int dispatch_to_thread(const int fd, int tid)
{
    if (!static_worker_pool)
    {
        // First dispatch
        thread_init();
    }

    BSP_THREAD *t;
    BSP_SERVER *srv;
    BSP_CLIENT *clt;
    BSP_CONNECTOR *cnt;
    BSP_TIMER *tmr;

    if ((tid < 0 || tid > static_worker_total) && tid != MAIN_THREAD && tid != UNBOUNDED_THREAD)
    {
        // Next static worker
        tid = _next_static_worker();
    }

    t = get_thread(tid);
    if (!t)
    {
        trigger_exit(BSP_RTN_ERROR_PTHREAD, "Unavailable thread selected");
    }
    trace_msg(TRACE_LEVEL_VERBOSE, "Thread : Thread %d selected by dispatcher", tid);

    // Add fd to epoll
    int fd_type = FD_TYPE_ANY;
    void *ptr = get_fd(fd, &fd_type);
    struct epoll_event *ev = NULL;

    if (ptr)
    {
        switch (fd_type)
        {
            case FD_TYPE_SOCKET_SERVER : 
                srv = (BSP_SERVER *) ptr;
                ev = &srv->sck.ev;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a network server to thread %d", tid);
                break;
            case FD_TYPE_SOCKET_CLIENT : 
                clt = (BSP_CLIENT *) ptr;
                ev = &clt->sck.ev;
                clt->sck.read_block = t->read_block;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a network client to thread %d", tid);
                // New stack
                clt->script_stack.state = t->script_runner.state;
                script_new_stack(&clt->script_stack);
                break;
            case FD_TYPE_SOCKET_CONNECTOR : 
                cnt = (BSP_CONNECTOR *) ptr;
                ev = &cnt->sck.ev;
                cnt->sck.read_block = t->read_block;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a network connector to thread %d", tid);
                // New stack
                cnt->script_stack.state = t->script_runner.state;
                script_new_stack(&cnt->script_stack);
                break;
            case FD_TYPE_TIMER : 
                tmr = (BSP_TIMER *) ptr;
                ev = &tmr->ev;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a timer to thread %d", tid);
                // New stack
                tmr->script_stack.state = t->script_runner.state;
                script_new_stack(&tmr->script_stack);
                break;
            default : 
                break;
        }
    }

    set_fd_thread(fd, tid);

    if (0 == epoll_ctl(t->loop_fd, EPOLL_CTL_ADD, fd, ev))
    {
        trace_msg(TRACE_LEVEL_DEBUG, "Thread : FD %d dispatch to thread %d", fd, tid);
        //set_fd_thread(fd, tid);
        t->nfds ++;
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Epoll operate failed");
        //bsp_spin_unlock(&t->fd_lock);
        return BSP_RTN_ERROR_IO;
    }

    // Send a signal to it
    static char sig_buff[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    if (t->pid)
    {
        trace_msg(TRACE_LEVEL_VERBOSE, "Thread : Poke thread %d", t->id);
        write(t->notify_fd, sig_buff, 8);
    }

    return BSP_RTN_SUCCESS;
}

// Remove a fd from thread
int remove_from_thread(const int fd)
{
    int tid = get_fd_thread(fd);
    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    BSP_THREAD *t;
    BSP_CLIENT *clt;
    BSP_CONNECTOR *cnt;
    BSP_TIMER *tmr;

    if (tid >= 0 && tid < static_worker_total)
    {
        t = &static_worker_pool[tid];
    }
    else if (tid == MAIN_THREAD)
    {
        t = &main_thread;
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : FD %d not in thread", fd);
        return BSP_RTN_ERROR_GENERAL;
    }

    int fd_type = FD_TYPE_ANY;
    void *ptr = get_fd(fd, &fd_type);
    if (ptr)
    {
        switch (fd_type)
        {
            case FD_TYPE_SOCKET_SERVER : 
                // WTF ...
                break;
            case FD_TYPE_SOCKET_CLIENT : 
                clt = (BSP_CLIENT *) ptr;
                // Remove stack
                script_remove_stack(&clt->script_stack);
                break;
            case FD_TYPE_SOCKET_CONNECTOR : 
                cnt = (BSP_CONNECTOR *) ptr;
                // Remove stack
                script_remove_stack(&cnt->script_stack);
                break;
            case FD_TYPE_TIMER : 
                tmr = (BSP_TIMER *) ptr;
                // Remove stack
                script_remove_stack(&tmr->script_stack);
                break;
            default : 
                break;
        }
    }

    if (t && t->pid)
    {
        if (0 == epoll_ctl(t->loop_fd, EPOLL_CTL_DEL, fd, NULL))
        {
            set_fd_thread(fd, UNBOUNDED_THREAD);
            t->nfds --;
            // Send a signal to thread to end epoll_wait
            write(t->notify_fd, buff, 8);
            trace_msg(TRACE_LEVEL_DEBUG, "Thread : Remove FD %d from thread %d", fd, t->id);
        }
        else
        {
            trace_msg(TRACE_LEVEL_ERROR, "Thread : EpollCtl error, remove FD %d failed", fd);
            return BSP_RTN_ERROR_EPOLL;
        }
    }

    return BSP_RTN_SUCCESS;
}

// Modify fd's listening event in epoll
int modify_fd_events(const int fd, struct epoll_event *ev)
{
    int tid = get_fd_thread(fd);
    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    BSP_THREAD *t = get_thread(tid);
    
    if (!t)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : FD %d not in thread", fd);

        return BSP_RTN_ERROR_GENERAL;
    }
    
    if (t && t->pid)
    {
        if (0 == epoll_ctl(t->loop_fd, EPOLL_CTL_MOD, fd, ev))
        {
            // Send a signal to thread to end epoll_wait
            write(t->notify_fd, buff, 8);
            trace_msg(TRACE_LEVEL_DEBUG, "Thread : FD %d's event updated", fd);
        }
        else
        {
            trace_msg(TRACE_LEVEL_ERROR, "Thread : FD %s's event update failed", fd);
            return BSP_RTN_ERROR_EPOLL;
        }
    }

    return BSP_RTN_SUCCESS;
}

// Trigger script garbage-collection
int trigger_gc(int tid)
{
    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 127};
    BSP_THREAD *t = get_thread(tid);
    if (t)
    {
        write(t->notify_fd, buff, 8);
    }
    else
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    return BSP_RTN_SUCCESS;
}

// Stop all threads
void stop_workers()
{
    int i;
    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 127};
    BSP_THREAD *t;
    
    for (i = 0; i < static_worker_total; i ++)
    {
        t = get_thread(i);
        if (t)
        {
            write(t->exit_fd, buff, 8);
        }
    }

    return;
}

// Get thread by index
BSP_THREAD * get_thread(int tid)
{
    BSP_THREAD *t;
    if (tid >= 0 && tid < static_worker_total)
    {
        t = &static_worker_pool[tid];
    }
    else if (tid == MAIN_THREAD)
    {
        t = &main_thread;
    }
    else
    {
        t = NULL;
    }

    return t;
}

// Find current thread
BSP_THREAD * curr_thread()
{
    int id = UNBOUNDED_THREAD;
    void *addr = pthread_getspecific(lid_key);
    if (addr)
    {
        memcpy(&id, addr, sizeof(int));
    }

    return get_thread(id);
}

int curr_thread_id()
{
    int id = UNBOUNDED_THREAD;
    void *addr = pthread_getspecific(lid_key);
    if (addr)
    {
        memcpy(&id, addr, sizeof(int));
    }

    return id;
}
