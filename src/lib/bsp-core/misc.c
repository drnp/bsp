/*
 * misc.c
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
 * Miscellaneous functions
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/31/2012
 * @changelog 
 *      [05/31/2012] - Creation
 *      [10/31/2012] - Try to get number of processors
 */

#define _GNU_SOURCE

#include <sys/sysinfo.h>

#include "bsp.h"

BSP_SPINLOCK trace_lock = BSP_SPINLOCK_INITIALIZER;

// Get current running process' location
char * get_dir()
{
    char self_name[_POSIX_PATH_MAX];
    char *ret = NULL;
    char *curr;
    ssize_t nbytes = readlink("/proc/self/exe", self_name, _POSIX_PATH_MAX - 1);

    if (-1 == nbytes)
    {
        ret = "./";
    }
    else
    {
        self_name[nbytes] = 0x0;
        ret = realpath(self_name, NULL);
        curr = strrchr(ret, '/');

        if (curr)
        {
            curr[0] = 0x0;
            curr = strrchr(ret, '/');
            // Prev layer
            if (curr)
            {
                curr[0] = 0x0;
            }
        }
    }

    return ret;
}

// Set current working dir
void set_dir(const char *dir)
{
    if (0 == chdir(dir))
    {
        trace_msg(TRACE_LEVEL_NOTICE, "Misc   : Change current working directory to %s", dir);
    }
    else
    {
        trace_msg(TRACE_LEVEL_ERROR, "Misc   : Change current working directory failed");
    }

    BSP_CORE_SETTING *settings = get_core_setting();
    settings->base_dir = dir;
    if (settings->mod_dir)
    {
        free(settings->mod_dir);
    }
    asprintf(&settings->mod_dir, "%s/lib/%s/", dir, BSP_PACKAGE_NAME);
    if (settings->log_dir)
    {
        free(settings->log_dir);
    }
    asprintf(&settings->log_dir, "%s/log/", dir);
    if (settings->runtime_dir)
    {
        free(settings->runtime_dir);
    }
    asprintf(&settings->runtime_dir, "%s/run/", dir);

    return;
}

// Save PID file (a text file whose content is process ID)
void save_pid()
{
    BSP_CORE_SETTING *settings = get_core_setting();
    char filename[_POSIX_PATH_MAX];
    memset(filename, 0, _POSIX_PATH_MAX);
    snprintf(filename, _POSIX_PATH_MAX - 1, "%s/bsp.%d.pid", settings->runtime_dir, settings->instance_id);
    // Try open
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        trace_msg(TRACE_LEVEL_ERROR, "Misc   : Write PID file failed");
        return;
    }

    pid_t pid = getpid();
    fprintf(fp, "%d", (int) pid);
    fclose(fp);
    trace_msg(TRACE_LEVEL_CORE, "Misc   : Saved process ID %d to file %s", (int) pid, filename);

    return;
}

// Trace information, if handle function registered in global setting, message will be proceed by them.
size_t trace_msg(int level, const char *fmt, ...)
{
    size_t nbytes = 0;
    BSP_CORE_SETTING *settings = get_core_setting();
    char msg[MAX_TRACE_LENGTH];

    if (!settings->trace_recorder && !settings->trace_printer)
    {
        // Nothing to do
        return 0;
    }

    if (0 == (level & settings->trace_level))
    {
        // Level ignored
        return 0;
    }

    bsp_spin_lock(&trace_lock);
    time_t now = time((time_t *) NULL);
    va_list ap;
    va_start(ap, fmt);
    nbytes = vsnprintf(msg, MAX_TRACE_LENGTH - 1, fmt, ap);
    if (nbytes >= 0)
    {
        msg[nbytes] = 0;
    }
    va_end(ap);

    if (settings->trace_recorder)
    {
        settings->trace_recorder(now, level, msg);
    }

    if (settings->trace_printer)
    {
        settings->trace_printer(now, level, msg);
    }

    bsp_spin_unlock(&trace_lock);

    return nbytes;
}

// Return message level in string format, if with_color set non-zero, terminal color information will be included
char * get_trace_level_str(int level, int with_color)
{
    switch (level)
    {
        case TRACE_LEVEL_CORE : 
            return (with_color) ? "\033[1;34m CORE    \033[0m" : " CORE    ";
            break;
        case TRACE_LEVEL_FATAL : 
            return (with_color) ? "\033[1;31m FATAL   \033[0m" : " FATAL   ";
            break;
        case TRACE_LEVEL_ERROR : 
            return (with_color) ? "\033[1;33m ERROR   \033[0m" : " ERROR   ";
            break;
        case TRACE_LEVEL_NOTICE : 
            return (with_color) ? "\033[1;35m NOTICE  \033[0m" : " NOTICE  ";
            break;
        case TRACE_LEVEL_DEBUG : 
            return (with_color) ? "\033[1;36m DEBUG   \033[0m" : " DEBUG   ";
            break;
        case TRACE_LEVEL_VERBOSE : 
            return (with_color) ? "\033[1;32m VERBOSE \033[0m" : " VERBOSE ";
            break;
        default : 
            return " UNKNOWN ";
    }

    return "         ";
}

// Filtering non-printable characters, replace them by given substitute
int filter_non_pritable_char(char *input, ssize_t len, char r)
{
    if (!input)
    {
        return 0;
    }

    if (len < 0)
    {
        len = strlen(input);
    }

    int i, nfiltered = 0;

    for (i = 0; i < len; i ++)
    {
        if (input[i] < 32 || input[i] > 127)
        {
            input[i] = r;
            nfiltered ++;
        }
    }

    return nfiltered;
}

// LUA table real length
size_t lua_table_size(lua_State *s, int idx)
{
    size_t ret = 0;
    idx = lua_absindex(s, idx);
    if (s && lua_istable(s, idx))
    {
        lua_checkstack(s, 2);
        lua_pushnil(s);
        while (0 != lua_next(s, idx))
        {
            ret ++;
            lua_pop(s, 1);
        }
    }

    return ret;
}
