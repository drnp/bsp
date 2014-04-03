/*
 * bsp_ip_list.h
 *
 * Copyright (C) 2013 - Dr.NP
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
 * IPv4 / IPv6 list header, used for whitelist / blacklist
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 04/07/2013
 * @changelog
 *      [04/07/2013] - Creation
 */

#ifndef _LIB_BSP_CORE_IP_FILTER_H

#define _LIB_BSP_CORE_IP_FILTER_H
/* Headers */

/* Definations */
#define IPLIST_SIZE                             65536

/* Macros */

/* Structs */
struct bsp_ipv4_item_t
{
    uint32_t            addr;
    struct bsp_ipv4_item_t
                        *next;
};

struct bsp_ipv6_item_t
{
    uint8_t             addr[16];
    struct bsp_ipv6_item_t
                        *next;
};

typedef struct bsp_ipv4_list_t
{
    struct bsp_ipv4_item_t
                        *slab[IPLIST_SIZE][2];
} BSP_IPV4_LIST;

typedef struct bsp_ipv6_list_t
{
    struct bsp_ipv6_item_t
                        *slab[IPLIST_SIZE][2];
} BSP_IPV6_LIST;

/* Functions */
// Create ip list
BSP_IPV4_LIST * new_ipv4_list();
BSP_IPV6_LIST * new_ipv6_list();

// Find if an IPv4 address exists in list, 1 for true
int search_ipv4(BSP_IPV4_LIST *list, uint32_t addr);

// Add an IPv4 address to list
void add_ipv4(BSP_IPV4_LIST *list, uint32_t addr);

// Delete an IPv4 address from list
void del_ipv4(BSP_IPV4_LIST *list, uint32_t addr);

// Find if an IPv6 address exists in list, 1 for true
int search_ipv6(BSP_IPV6_LIST *list, uint8_t *addr);

// Add an IPv6 address to list
void add_ipv6(BSP_IPV6_LIST *list, uint8_t *addr);

// Delete an IPv6 address from list
void del_ipv6(BSP_IPV6_LIST *list, uint8_t *addr);

#endif  /* _LIB_BSP_CORE_IP_FILTER_H */
