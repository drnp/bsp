/*
 * os.c
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
 * OS-related functions
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/23/2012
 * @changelog 
 *      [06/06/2012] - Creation
 *      [10/23/2012] - Stop signal capture added
 */

#include "bsp.h"

#include <signal.h>

// Capture signals
// All quit signals will redirect to function exit_handler
// SIGPIPE will be ignored
static void exit_handler(const int sig)
{
    // DO ALL THINGS YOU WANT...
    BSP_CORE_SETTING *settings = get_core_setting();
    if (settings->on_proc_exit)
    {
        settings->on_proc_exit(sig);
    }
    
    fprintf(stderr, "\n\n");
    trace_msg(TRACE_LEVEL_CORE, "Core   : SIGNAL %d handled, process terminated", sig);
    fprintf(stderr, "\n\n");
    
    exit(BSP_RTN_SUCCESS);
}

static void tstp_handler(const int sig)
{
    // Stop action
    BSP_CORE_SETTING *settings = get_core_setting();
    if (settings->on_proc_tstp)
    {
        settings->on_proc_tstp(sig);
    }
    
    fprintf(stderr, "\n\n");
    trace_msg(TRACE_LEVEL_CORE, "Core   : SIGNAL %d handled, TSTP action requested", sig);
    fprintf(stderr, "\n\n");

    // Do nothing
    return;
}

static void usr1_handler(const int sig)
{
    // User defineded 1
    BSP_CORE_SETTING *settings = get_core_setting();
    if (settings->on_proc_usr1)
    {
        settings->on_proc_usr1(sig);
    }

    fprintf(stderr, "\n\n");
    trace_msg(TRACE_LEVEL_CORE, "Core   : SIGNAL %d handled, USR1 action requested", sig);
    fprintf(stderr, "\n\n");
}

static void usr2_handler(const int sig)
{
    // User defineded 1
    BSP_CORE_SETTING *settings = get_core_setting();
    if (settings->on_proc_usr2)
    {
        settings->on_proc_usr2(sig);
    }

    fprintf(stderr, "\n\n");
    trace_msg(TRACE_LEVEL_CORE, "Core   : SIGNAL %d handled, USR2 action requested", sig);
    fprintf(stderr, "\n\n");
}

// Set common signals with default behavior, we can overwrite them later
void signal_init()
{
    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);
    signal(SIGQUIT, exit_handler);
    signal(SIGKILL, exit_handler);
    signal(SIGTSTP, tstp_handler);
    signal(SIGUSR1, usr1_handler);
    signal(SIGUSR2, usr2_handler);
    signal(SIGPIPE, SIG_IGN);

    trace_msg(TRACE_LEVEL_DEBUG, "Core   : Signals set with default behavior");

    return;
}

// Regist signal behavior
void set_signal(const int sig, void (cb)(int))
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_sigaction = NULL;
    sa.sa_handler = (cb) ? cb : SIG_IGN;

    if (0 != sigaction(sig, &sa, NULL))
    {
        trace_msg(TRACE_LEVEL_ERROR, "Core   : Set signal %d action failed", sig);
    }

    return;
}

// Make application as daemon process
int proc_daemonize()
{
    int fd;

    // Fork a new process
    switch(fork())
    {
        case -1 : 
            trace_msg(TRACE_LEVEL_ERROR, "Cannot fork child process");
            return -1;
        case 0 :
            break;
        default :
            exit(1);
    }
    
    if (setsid() == -1)
    {
        return -1;
    }

    // Redirect standard IO to null device
    fd = open("/dev/null", O_RDWR, 0);
    if (fd)
    {
        (void) dup2(fd, STDIN_FILENO);
        (void) dup2(fd, STDOUT_FILENO);
        (void) dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
        {
            (void) close(fd);
        }
    }

    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Cannot open null device");
    }
    
    return 0;
}

// Try to enable large page
int enable_large_pages()
{
#if defined(HAVE_GETPAGESIZES) && defined(HAVE_MEMCNTL)
    int ret = -1;
    size_t sizes[32];
    int avail = getpagesizes(sizes, 32);
    if (avail != -1)
    {
        size_t max = sizes[0];
        struct memcntl_mha arg = {0};
        int ii;

        for (ii = 1; ii < avail; ++ii)
        {
            if (max < sizes[ii])
            {
                max = sizes[ii];
            }
        }

        arg.mha_flags = 0;
        arg.mha_pagesize = max;
        arg.mha_cmd = MHA_MAPSIZE_BSSBRK;

        if (memcntl(0, 0, MC_HAT_ADVISE, (caddr_t) &arg, 0, 0) == -1)
        {
            // memcntl failed
            trace_msg(TRACE_LEVEL_FATAL, "Core   : Memcntl failed");
            _exit(BSP_RTN_ERROR_MEMORY);
        }

        else
        {
            ret = 0;
        }

        trace_msg(TRACE_LEVEL_CORE, "Core   : Memory page size set as %d", arg.mha_pagesize);
    }

    else
    {
        // Get pagesizes failed
        trace_msg(TRACE_LEVEL_ERROR, "Core   : Get memory page size error");
    }

    return ret;
#else
    trace_msg(TRACE_LEVEL_CORE, "Core   : HugeTLB not supported on this system");
    return 0;
#endif
}
