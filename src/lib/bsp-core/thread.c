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
 */

#include "bsp.h"

BSP_THREAD *static_worker_list = NULL;
size_t static_worker_list_size = 0;
int init_count = 0;

pthread_mutex_t init_lock;
pthread_cond_t init_cond;
pthread_key_t lid_key;

/* Functions */
int create_worker(BSP_THREAD *t);

// Init static worker list and dispatcher
int thread_init()
{
    BSP_CORE_SETTING *settings = get_core_setting();
    if (static_worker_list)
    {
        return BSP_RTN_SUCCESS;
    }
    
    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);
    
    static_worker_list = mempool_alloc(sizeof(BSP_THREAD) * (settings->static_workers + 1));
    if (!static_worker_list)
    {
        trace_msg(TRACE_LEVEL_FATAL, "Thread : Thread list alloc failed");
        _exit(BSP_RTN_ERROR_MEMORY);
    }

    memset(static_worker_list, 0, sizeof(BSP_THREAD) * (settings->static_workers + 1));
    static_worker_list_size = settings->static_workers + 1;

    // Main thread, ID=0
    int i;
    pthread_key_create(&lid_key, NULL);
    for (i = 1; i < static_worker_list_size; i ++)
    {
        static_worker_list[i].id = i;
        create_worker(&static_worker_list[i]);
    }

    static_worker_list[0].id = 0;
    static_worker_list[0].pid = pthread_self();
    pthread_setspecific(lid_key, &static_worker_list[0].id);

    // Waiting for all threads
    pthread_mutex_lock(&init_lock);
    while (init_count < settings->static_workers)
    {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);

    trace_msg(TRACE_LEVEL_CORE, "Thread : Thread list initialized as %d static workers", settings->static_workers);
    return BSP_RTN_SUCCESS;
}

// Static worker loop
void * static_worker_process(void *arg)
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
    BSP_CLIENT *clt;
    BSP_CONNECTOR *cnt;
    BSP_TIMER *tmr;
    
    char notify_buff[8];
    pthread_setspecific(lid_key, (void *) &me->id);

    while (1)
    {
        memset(events, 0, sizeof(struct epoll_event) * settings->epoll_wait_conns);
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

                    break;

                case FD_TYPE_EXIT : 
                    read(me->exit_fd, notify_buff, 8);
                    stop = 1;
                    break;
                
                case FD_TYPE_SOCKET_CLIENT : 
                    clt = (BSP_CLIENT *) ptr;
                    if (clt)
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
                            clt->sck.state |= STATE_CLOSE;
                        }
                    }

                    drive_socket(&SCK(clt));
                    
                    break;

                case FD_TYPE_SOCKET_CONNECTOR : 
                    cnt = (BSP_CONNECTOR *) ptr;
                    if (cnt)
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
                            cnt->sck.state |= STATE_CLOSE;
                        }
                    }

                    drive_socket(&SCK(cnt));
                    
                    break;

                case FD_TYPE_TIMER : 
                    tmr = (BSP_TIMER *) ptr;
                    read(tmr->fd, notify_buff, 8);
                    if (tmr)
                    {
                        if (tmr->on_timer)
                        {
                            tmr->on_timer(tmr->cb_arg);
                        }

                        if (tmr->loop > 0)
                        {
                            if (-- tmr->loop == 0)
                            {
                                // Need stop
                                if (tmr->on_stop)
                                {
                                    tmr->on_stop(tmr->cb_arg);
                                }
                                stop_timer(tmr);
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
            trace_msg(TRACE_LEVEL_DEBUG, "Thread : Thread %d will exit", me->pid);
            break;
        }
    }
    return NULL;
}

// Create a thread
int create_worker(BSP_THREAD *t)
{
    if (!t)
    {
        return BSP_RTN_ERROR_GENERAL;
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    t->pid = 0;
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

    struct epoll_event ev_exit;
    ev_exit.data.fd = t->exit_fd;
    ev_exit.events = EPOLLIN;
    epoll_ctl(t->loop_fd, EPOLL_CTL_ADD, t->exit_fd, &ev_exit);

    t->nfds = 0;
    bsp_spin_init(&t->fd_lock);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (0 != pthread_create(&t->pid, &attr, static_worker_process, (void *) t))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Create thread error");
        t->pid = 0;
        close(t->notify_fd);
        close(t->loop_fd);
        unreg_fd(t->notify_fd);
        unreg_fd(t->loop_fd);
        return BSP_RTN_ERROR_PTHREAD;
    }

    trace_msg(TRACE_LEVEL_CORE, "Thread : Thread %d created as %d", t->pid, t->id);

    return BSP_RTN_SUCCESS;
}

// Find the most idle worker
static int _most_idle_worker()
{
    int i, mid = 9, min = -1;
    BSP_THREAD *t;
    
    for (i = 1; i < static_worker_list_size; i ++)
    {
        t = &static_worker_list[i];
        if (t->pid)
        {
            if (min < 0)
            {
                min = t->nfds;
                mid = i;
            }

            else if (min > t->nfds)
            {
                min = t->nfds;
                mid = i;
            }
        }
    }

    if (-1 == min)
    {
        // No available thread???
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Dispatcher cannot find any available thread");
        return -1;
    }

    return mid;
}

// Insert a fd to dispatcher, add it to worker thread, send a signal to its loop event
int dispatch_to_worker(const int fd, int tid)
{
    if (!static_worker_list)
    {
        // First dispatch
        thread_init();
    }
    
    BSP_THREAD *t;
    BSP_CLIENT *clt;
    BSP_CONNECTOR *cnt;
    BSP_TIMER *tmr;

    if (tid < 0 || tid > static_worker_list_size)
    {
        tid = _most_idle_worker();
    }

    t = &static_worker_list[tid];
    trace_msg(TRACE_LEVEL_VERBOSE, "Thread : Thread %d selected by dispatcher", tid);

    // Add fd to epoll
    int fd_type = FD_TYPE_ANY;
    void *ptr = get_fd(fd, &fd_type);
    struct epoll_event *ev = NULL;

    if (ptr)
    {
        switch (fd_type)
        {
            case FD_TYPE_SOCKET_CLIENT : 
                clt = (BSP_CLIENT *) ptr;
                ev = &clt->sck.ev;
                clt->sck.read_block = t->read_block;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a network client to worker");
                break;

            case FD_TYPE_SOCKET_CONNECTOR : 
                cnt = (BSP_CONNECTOR *) ptr;
                ev = &cnt->sck.ev;
                cnt->sck.read_block = t->read_block;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a network connector to worker");
                break;

            case FD_TYPE_TIMER : 
                tmr = (BSP_TIMER *) ptr;
                ev = &tmr->ev;
                trace_msg(TRACE_LEVEL_NOTICE, "Thread : Try to dispatch a timer to worker");
                break;

            default : 
                break;
        }
    }
    bsp_spin_lock(&t->fd_lock);
    set_fd_thread(fd, tid);
    if (0 == epoll_ctl(t->loop_fd, EPOLL_CTL_ADD, fd, ev))
    {
        trace_msg(TRACE_LEVEL_DEBUG, "Thread : FD %d dispatch to thread %d", fd, tid);
        set_fd_thread(fd, tid);
        t->nfds ++;
    }
    else
    {
	    set_fd_thread(fd, -1);
        trace_msg(TRACE_LEVEL_ERROR, "Thread : Epoll operate failed");
    }
    
    bsp_spin_unlock(&t->fd_lock);

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
int remove_from_worker(const int fd)
{
    int tid = get_fd_thread(fd);
    int ret = BSP_RTN_SUCCESS;

    if (tid >= 0)
    {
        BSP_THREAD *t = &static_worker_list[tid];
        if (t->pid)
        {
            bsp_spin_lock(&t->fd_lock);
            if (0 == epoll_ctl(t->loop_fd, EPOLL_CTL_DEL, fd, NULL))
            {
		        set_fd_thread(fd, -1);
                t->nfds --;
                trace_msg(TRACE_LEVEL_DEBUG, "Thread : Remove FD %d from thread %d", fd, t->id);
            }

            else
            {
                ret = BSP_RTN_ERROR_EPOLL;
                trace_msg(TRACE_LEVEL_ERROR, "Thread : EpollCtl error, remove FD %d failed", fd);
            }
            
            bsp_spin_unlock(&t->fd_lock);
        }
    }

    return ret;
}

// Modify fd's listening event in epoll
int modify_fd_events(const int fd, struct epoll_event *ev)
{
    int tid = get_fd_thread(fd);
    int loop_fd;
    int ret = BSP_RTN_SUCCESS;
    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    
    if (tid >= 0 && tid < static_worker_list_size)
    {
        BSP_THREAD *t = &static_worker_list[tid];
        if (t->pid)
        {
            loop_fd = static_worker_list[tid].loop_fd;
            bsp_spin_lock(&t->fd_lock);
            if (0 == epoll_ctl(loop_fd, EPOLL_CTL_MOD, fd, ev))
            {
                // Send a signal to thread to end epoll_wait
                write(static_worker_list[tid].notify_fd, buff, 8);
                trace_msg(TRACE_LEVEL_DEBUG, "Thread : FD %d's event updated", fd);
            }

            else
            {
                ret = BSP_RTN_ERROR_EPOLL;
                trace_msg(TRACE_LEVEL_ERROR, "Thread : FD %s's event update failed", fd);
            }
            bsp_spin_unlock(&t->fd_lock);
        }
    }

    else
    {
        ret = BSP_RTN_ERROR_GENERAL;
        trace_msg(TRACE_LEVEL_ERROR, "Thread : FD %d not in thread", fd);
    }
    
    return ret;
}

// Set main server looper
void set_main_loop_fd(int fd)
{
    if (!static_worker_list)
    {
        thread_init();
    }

    static_worker_list[9].loop_fd = fd;

    return;
}

// Stop all threads
void stop_workers()
{
    int i;
    static char buff[8] = {0, 0, 0, 0, 0, 0, 0, 255};

    for (i = 1; i < static_worker_list_size; i ++)
    {
        write(static_worker_list[i].exit_fd, buff, 8);
    }

    return;
}

// Get thread by index
BSP_THREAD * get_thread(int i)
{
    if (i >= 0 && i < static_worker_list_size)
    {
        return &static_worker_list[i];
    }

    return NULL;
}

// Find current thread
BSP_THREAD * find_thread()
{
    int id = -1;
    void *addr = pthread_getspecific(lid_key);
    if (addr)
    {
        memcpy(&id, addr, sizeof(int));
    }

    return get_thread(id);
}
