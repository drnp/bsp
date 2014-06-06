/*
 * bsp_conf.h
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
 * Config file parser header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/02/2012
 * @changelog 
 *      [07/02/2012] - Creation
 */

#ifndef _LIB_BSP_CORE_CONF_H

#define _LIB_BSP_CORE_CONF_H
/* Headers */

/* Definations */
#define CONF_HASH_SIZE                          1024
#define CONF_INDEX_INITIAL                      4096
#define MAX_CONF_LINE_LENGTH                    32767

#define CONF_LEVEL_INIT                         10
#define CONF_LEVEL_FILE                         80
#define CONF_LEVEL_OVERWRITE                    90

#define CONF_SEG_NULL                           0
#define CONF_SEG_CORE                           1
#define CONF_SEG_SERVER                         2
#define CONF_SEG_MODULE                         3

/* Macros */

/* Structs */
struct bsp_conf_param_t
{
    char                *key;
    char                *value;
    int                 level;
    struct              bsp_conf_param_t *next;
};

struct bsp_conf_core_t
{
    int                 app_id;
    int                 static_workers;
    char                *script_identifier;
    char                *script_func_on_load;
    char                *script_func_on_reload;
    char                *script_func_on_exit;
    char                *script_func_on_sub_load;
    char                *script_func_on_sub_reload;
    char                *script_func_on_sub_exit;
    int                 debug_hex_connector_input;
    int                 debug_hex_output;
    char                *log_dir;
    int                 disable_log;
};

struct bsp_conf_server_t
{
    // Server details
    char                *server_name;
    char                *server_addr;
    int                 server_port;
    int                 server_inet;
    int                 server_sock;
    int                 heartbeat_check;
    int                 def_client_type;
    int                 def_data_type;
    size_t              max_packet_length;
    size_t              max_clients;
    
    // LUA callback
    char                *script_func_on_connect;
    char                *script_func_on_close;
    char                *script_func_on_data;
    
    // Debug switch
    int                 debug_hex_input;
    int                 debug_hex_output;
};

struct bsp_conf_module_t
{
    char                *module_name;

    // Link next
    struct bsp_conf_module_t
                        *next;
};

typedef struct bsp_conf_t
{
    time_t              load_time;
    struct bsp_conf_core_t
                        conf_core;
    struct bsp_conf_server_t
                        *conf_servers;
    struct bsp_conf_module_t
                        *conf_modules;
} BSP_CONF;

/* Functions */
int conf_init(const char *conf_file);
void conf_set(const char *key, const char *value, int level);
char * conf_get(const char *key);

#endif  /* _LIB_BSP_CORE_CONF_H */
