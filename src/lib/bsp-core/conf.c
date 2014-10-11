/*
 * conf_parser.c
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
 * Parse config file (linux ini-style) into object
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 07/02/2012
 * @changelog 
 *      [07/02/2012] - Creation
 *      [03/28/2013] - Moved to core
 *      [07/31/2013] - Rebuild
 */

#include "bsp.h"

struct bsp_conf_param_t conf_list[CONF_HASH_SIZE];
struct bsp_conf_param_t **conf_index;
BSP_SPINLOCK conf_lock;
size_t conf_total, conf_index_size;
/*
BSP_CONF conf;

// Parse ini-style expression as values
static void _parse_ini_line(char *line, struct bsp_conf_param_t *p)
{
    int l = strlen(line);
    int ct = 0;
    int in_space = 1;
    int has_key = 0;
    int in_quote = 0;
    //int in_seg = 0;
    //int in_escape = 0;
    p->key = p->value = NULL;

    for (ct = 0; ct < l; ct ++)
    {
        // Quote test
        if (line[ct] == '\'')
        {
            if (0 == in_quote)
            {
                in_quote = 1;
                line[ct] = 0x0;
            }

            else if (1 == in_quote)
            {
                in_quote = 0;
                line[ct] = 0x0;
            }
        }

        else if (line[ct] == '"')
        {
            if (0 == in_quote)
            {
                in_quote = 3;
                line[ct] = 0x0;
            }

            else if (3 == in_quote)
            {
                in_quote = 0;
                line[ct] = 0x0;
            }
        }

        else if (line[ct] == '[')
        {
            if (0 == in_quote)
            {
                in_quote = 5;
                //in_seg = 1;
                has_key = 1;
                line[ct] = 0x0;
            }
        }

        else if (line[ct] == ']')
        {
            if (5 == in_quote)
            {
                in_quote = 0;
                //in_seg = 0;
                if (p->key)
                {
                    p->key = NULL;
                }
                line[ct] = 0x0;

                return;
            }
        }

        // Comments
        if ((';' == line[ct] || '#' == line[ct]) && !in_quote)
        {
            if (!p->value)
            {
                p->key = NULL;
            }
            line[ct] = 0x0;
            
            return;
        }
        if ((!in_quote && line[ct] <= 0x20) || (in_quote && line[ct] < 0x20))
        {
            in_space = 1;
            line[ct] = 0x0;
        }
        
        else if ('=' == line[ct] && p->key && !in_quote)
        {
            // Depart?
            line[ct] = 0x0;
            in_space = 1;
            if (p->key)
            {
                has_key = 1;
            }
        }
        
        else
        {
            // Legal ASCII
            if (1 == in_space)
            {
                in_space = 0;
                if (!has_key)
                {
                    p->key = line + ct;
                }

                else
                {
                    p->value = line + ct;
                }
            }
        }
    }
    
    return;
}
*/
// Initialization
int conf_init(const char *conf_file)
{
    //memset(&conf, 0, sizeof(BSP_CONF));
    // Conf hash
    bsp_spin_init(&conf_lock);
    memset(conf_list, 0, CONF_HASH_SIZE * sizeof(struct bsp_conf_param_t));
    conf_index = bsp_calloc(CONF_INDEX_INITIAL, sizeof(struct bsp_conf_param_t *));
    
    if (!conf_index)
    {
        trigger_exit(BSP_RTN_ERROR_MEMORY, "Configure table alloc error");
    }
    
    conf_total = 0;
    conf_index_size = CONF_INDEX_INITIAL;
    trace_msg(TRACE_LEVEL_VERBOSE, "Conf   : Configure table initialized");
    
    return BSP_RTN_SUCCESS;
}
/*
    // Read config data
    char line[MAX_CONF_LINE_LENGTH];
    struct bsp_conf_param_t p;
    int seg = CONF_SEG_NULL;
    struct bsp_conf_server_t *conf_srv = NULL;
    FILE *fp = fopen(conf_file, "r");
    if (!fp)
    {
        // Cannot open config file
        trace_msg(TRACE_LEVEL_ERROR, "Cannot open configure file <%s>", conf_file);
        return NULL;
    }

    while (!feof(fp))
    {
        if (!fgets(line, MAX_CONF_LINE_LENGTH - 1, fp))
        {
            break;
        }
        
        _parse_ini_line(line, &p);
        if (p.value)
        {
            if (!p.key)
            {
                // In segment
                if (0 == strcasecmp("CORE", p.value))
                {
                    seg = CONF_SEG_CORE;
                }

                else if (0 == strcasecmp("SERVER", p.value))
                {
                    seg = CONF_SEG_SERVER;
                    // Make a new server node
                    conf_srv = mempool_calloc(1, sizeof(struct bsp_conf_server_t));
                    if (conf_srv)
                    {
                        // Default values
                        conf_srv->server_inet = INET_TYPE_ANY;
                        conf_srv->server_sock = SOCK_TYPE_TCP;
                        conf_srv->default_client_type = CLIENT_TYPE_DATA;
                        conf_srv->default_data_type = DATA_TYPE_PACKET;
                        conf_srv->next = conf.conf_servers;
                        conf.conf_servers = conf_srv;
                    }
                }
                else if (0 == strcasecmp("MODULE", p.value))
                {
                    seg = CONF_SEG_MODULE;
                }
            }

            else
            {
                switch (seg)
                {
                    case CONF_SEG_CORE : 
                        if (0 == strcasecmp("APP_ID", p.key))
                        {
                            conf.conf_core.app_id = atoi(p.value);
                        }
                        else if (0 == strcasecmp("STATIC_WORKERS", p.key))
                        {
                            conf.conf_core.static_workers = atoi(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_IDENTIFIER", p.key))
                        {
                            if (conf.conf_core.script_identifier)
                            {
                                mempool_free(conf.conf_core.script_identifier);
                            }
                            conf.conf_core.script_identifier = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_LOAD", p.key))
                        {
                            if (conf.conf_core.script_func_on_load)
                            {
                                mempool_free(conf.conf_core.script_func_on_load);
                            }
                            conf.conf_core.script_func_on_load = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_RELOAD", p.key))
                        {
                            if (conf.conf_core.script_func_on_reload)
                            {
                                mempool_free(conf.conf_core.script_func_on_reload);
                            }
                            conf.conf_core.script_func_on_reload = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_EXIT", p.key))
                        {
                            if (conf.conf_core.script_func_on_exit)
                            {
                                mempool_free(conf.conf_core.script_func_on_exit);
                            }
                            conf.conf_core.script_func_on_exit = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_SUB_LOAD", p.key))
                        {
                            if (conf.conf_core.script_func_on_sub_load)
                            {
                                mempool_free(conf.conf_core.script_func_on_sub_load);
                            }
                            conf.conf_core.script_func_on_sub_load = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_SUB_RELOAD", p.key))
                        {
                            if (conf.conf_core.script_func_on_sub_reload)
                            {
                                mempool_free(conf.conf_core.script_func_on_sub_reload);
                            }
                            conf.conf_core.script_func_on_sub_reload = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_SUB_EXIT", p.key))
                        {
                            if (conf.conf_core.script_func_on_sub_exit)
                            {
                                mempool_free(conf.conf_core.script_func_on_sub_exit);
                            }
                            conf.conf_core.script_func_on_sub_exit = mempool_strdup(p.value);
                        }
                        // For debug
                        else if (0 == strcasecmp("_DEBUG_HEX_CONNECTOR_INPUT_", p.key))
                        {
                            conf.conf_core.debug_hex_connector_input = atoi(p.value);
                        }
                        else if (0 == strcasecmp("_DEBUG_HEX_OUTPUT_", p.key))
                        {
                            conf.conf_core.debug_hex_output = atoi(p.value);
                        }
                        else if (0 == strcasecmp("LOG_DIR", p.key))
                        {
                            conf.conf_core.log_dir = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("_DISABLE_LOG_", p.key))
                        {
                            conf.conf_core.disable_log = atoi(p.value);
                        }
                        else
                        {
                            // Do nothing
                        }
                        break;
                    case CONF_SEG_SERVER : 
                        if (!conf_srv)
                        {
                            continue;
                        }
                        if (0 == strcasecmp("SERVER_ADDR", p.key))
                        {
                            if (conf_srv->server_addr)
                            {
                                mempool_free(conf_srv->server_addr);
                            }
                            conf_srv->server_addr = mempool_strdup(p.value);
                        }

                        else if (0 == strcasecmp("SERVER_PORT", p.key))
                        {
                            conf_srv->server_port = atoi(p.value);
                        }
                        else if (0 == strcasecmp("SERVER_INET", p.key))
                        {
                            if (0 == strncasecmp("IPV4", p.value, 4))
                            {
                                conf_srv->server_inet = INET_TYPE_IPV4;
                            }
                            else if (0 == strncasecmp("IPV6", p.value, 6))
                            {
                                conf_srv->server_inet = INET_TYPE_IPV6;
                            }
                            else
                            {
                                conf_srv->server_inet = INET_TYPE_ANY;
                            }
                        }
                        else if (0 == strcasecmp("SERVER_SOCK", p.key))
                        {
                            if (0 == strncasecmp("TCP", p.value, 3))
                            {
                                conf_srv->server_sock = SOCK_TYPE_TCP;
                            }
                            else if (0 == strncasecmp("UDP", p.value, 3))
                            {
                                conf_srv->server_sock = SOCK_TYPE_UDP;
                            }
                            else
                            {
                                conf_srv->server_sock = SOCK_TYPE_ANY;
                            }
                        }
                        else if (0 == strcasecmp("HEARTBEAT_CHECK", p.key))
                        {
                            conf_srv->heartbeat_check = atoi(p.value);
                        }
                        else if (0 == strcasecmp("DEFAULT_CLIENT_TYPE", p.key))
                        {
                            if (0 == strncasecmp("WEBSOCKET", p.value, 9))
                            {
                                conf_srv->default_client_type = CLIENT_TYPE_WEBSOCKET_HANDSHAKE;
                            }

                            else
                            {
                                conf_srv->default_client_type = CLIENT_TYPE_DATA;
                            }
                        }
                        else if (0 == strcasecmp("DEFAULT_DATA_TYPE", p.key))
                        {
                            if (0 == strncasecmp("STREAM", p.value, 6))
                            {
                                conf_srv->default_data_type = DATA_TYPE_STREAM;
                            }
                            else
                            {
                                conf_srv->default_data_type = DATA_TYPE_PACKET;
                            }
                        }
                        else if (0 == strcasecmp("_MAX_PACKET_LENGTH_", p.key))
                        {
                            conf_srv->max_packet_length = (size_t) atoll(p.value);
                        }
                        else if (0 == strcasecmp("MAX_CLIENTS", p.key))
                        {
                            conf_srv->max_clients = (size_t) atoll(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_CONNECT", p.key))
                        {
                            if (conf_srv->script_func_on_connect)
                            {
                                mempool_free(conf_srv->script_func_on_connect);
                            }
                            conf_srv->script_func_on_connect = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_CLOSE", p.key))
                        {
                            if (conf_srv->script_func_on_close)
                            {
                                mempool_free(conf_srv->script_func_on_close);
                            }
                            conf_srv->script_func_on_close = mempool_strdup(p.value);
                        }
                        else if (0 == strcasecmp("SCRIPT_FUNC_ON_DATA", p.key))
                        {
                            if (conf_srv->script_func_on_data)
                            {
                                mempool_free(conf_srv->script_func_on_data);
                            }
                            conf_srv->script_func_on_data = mempool_strdup(p.value);
                        }
                        // For debug
                        else if (0 == strcasecmp("_DEBUG_HEX_CLIENT_INPUT_", p.key))
                        {
                            conf_srv->debug_hex_client_input = atoi(p.value);
                        }
                        else
                        {
                            // Do nothing
                        }
                        break;
                    case CONF_SEG_MODULE : 
                        if (0 == strcasecmp("SCRIPT_LOAD_MODULE", p.key))
                        {
                            // New module
                            struct bsp_conf_module_t *curr = conf.conf_modules;
                            while (curr)
                            {
                                // Search for loaded module
                                if (0 == strcmp(curr->module_name, p.value))
                                {
                                    // Already loaded
                                    break;
                                }
                                curr = curr->next;
                            }
                            if (!curr)
                            {
                                // New node
                                curr = mempool_calloc(1, sizeof(struct bsp_conf_module_t));
                                curr->module_name = mempool_strdup(p.value);
                                curr->next = conf.conf_modules;
                                conf.conf_modules = curr;
                            }
                        }
                        break;
                    default : 
                        break;
                }
            }
        }
    }

    fclose(fp);
    
    return &conf;
}
*/
// Set a configure item
void conf_set(const char *key, const char *value, int level)
{
    char modified = 0x0;
    int hash_key = (int) bsp_hash(key, strlen(key)) % CONF_HASH_SIZE;
    struct bsp_conf_param_t *last = &conf_list[hash_key];
    struct bsp_conf_param_t *head = last->next, **tmp;
    
    if (!key || !conf_index)
    {
        return;
    }
    
    if (!value)
    {
        value = "";
    }
    
    bsp_spin_lock(&conf_lock);
    
    while (head)
    {
        if (0 == strcmp(head->key, key))
        {
            if (level >= head->level)
            {
                if (head->value)
                {
                    bsp_free(head->value);
                }
                
                head->value = bsp_strdup(value);
                head->level = level;
            }
            
            modified = 0x1;
            
            break;
        }
        
        last = head;
        head = head->next;
    }
    
    if (!modified)
    {
        // Add new node
        head = bsp_malloc(sizeof(struct bsp_conf_param_t));
        if (!head)
        {
            return;
        }
        
        head->key = bsp_strdup(key);
        head->value = bsp_strdup(value);
        head->level = level;
        head->next = NULL;
        last->next = head;
        
        while (conf_total >= conf_index_size)
        {
            tmp = bsp_realloc(conf_index, sizeof(struct bsp_conf_param_t *) * conf_index_size * 2);
            if (!tmp)
            {
                continue;
            }
            
            conf_index = tmp;
            conf_index_size *= 2;
        }
        
        conf_index[conf_total ++] = head;
    }
    
    bsp_spin_unlock(&conf_lock);
    
    return;
}

// Get configure
char * conf_get(const char *key)
{
    char *value = "";
    int hash_key;
    struct bsp_conf_param_t *curr = NULL;
    
    if (key && conf_index)
    {
        hash_key = (int) bsp_hash(key, strlen(key)) % CONF_HASH_SIZE;
        curr = conf_list[hash_key].next;
        
        while (curr)
        {
            if (0 == strcmp(key, curr->key))
            {
                value = curr->value;
                break;
            }

            curr = curr->next;
        }
    }

    return value;
}
