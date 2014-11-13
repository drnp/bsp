/*
 * logger.c
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
 * Text message logger
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/14/2012
 * @chagelog 
 *      [06/14/2012] - Creation
 */

#include "bsp.h"

int log_fd = -1;
BSP_SPINLOCK log_lock = BSP_SPINLOCK_INITIALIZER;

// Record one line into log file
void log_add(time_t now, int level, const char *msg)
{
    struct tm *loctime;
    char tgdate[64];
    char line[MAX_LOG_LINE_LENGTH];

    if (!level || !msg)
    {
        return;
    }

    if (log_fd > 0)
    {
        loctime = localtime(&now);
        strftime(tgdate, 63, "%m/%d/%Y %H:%M:%S", loctime);
        bsp_spin_lock(&log_lock);
        size_t nbytes = snprintf(line, MAX_LOG_LINE_LENGTH - 1, "[%s] - [%s] : %s\n", tgdate, get_trace_level_str(level, 0), msg);
        line[nbytes] = 0;
        write(log_fd, line, nbytes);
        bsp_spin_unlock(&log_lock);
    }

    return;
}
// Open log file
void log_open()
{
    BSP_CORE_SETTING *settings = get_core_setting();
    char path[_POSIX_PATH_MAX];
    char *dir = settings->log_dir ? settings->log_dir : DEFAULT_LOG_DIR;
    memset(path, 0, _POSIX_PATH_MAX);

    // Check sub-directory
    snprintf(path, _POSIX_PATH_MAX - 1, "%s/%d/", dir, settings->instance_id);
    if (0 != mkdir(path, 0755) && errno != EEXIST)
    {
        // Dir error
        return;
    }

    time_t now = time(NULL);
    struct tm *loctime = localtime(&now);
    char tgdate[64];
    strftime(tgdate, 63, "%Y%m%d%H%M", loctime);
    snprintf(path, _POSIX_PATH_MAX - 1, "%s/%d/%s%s.log", 
             dir, 
             settings->instance_id, 
             LOG_FILENAME_PREFIX, 
             tgdate);
    log_close();
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (log_fd > 0)
    {
        reg_fd(log_fd, FD_TYPE_LOG, NULL);
    }

    return;
}

// Close log file
void log_close()
{
    if (log_fd > 0)
    {
        unreg_fd(log_fd);
        close(log_fd);
        log_fd = -1;
    }

    return;
}
