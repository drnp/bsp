/*
 * bsp_hash.h
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
 * Hash algorisims header
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/07/2012
 * @changelog 
 *      [06/07/2012] - Creation
 *      [12/18/2012] - Move macros out
 *      [04/07/2013] - IPv4 hash added
 */

#ifndef _LIB_BSP_CORE_HASH_H

#define _LIB_BSP_CORE_HASH_H
/* Headers */

/* Definations */
#define IPV4_HASH_BLOCK                         2038;

/* Macros */
// Macros for string hash
#define rot(x, k) (((x) << (k)) ^ ((x) >> (32 - (k))))

#define mix(a, b, c) \
{ \
    a -= c;  a ^= rot(c, 4);  c += b; \
    b -= a;  b ^= rot(a, 6);  a += c; \
    c -= b;  c ^= rot(b, 8);  b += a; \
    a -= c;  a ^= rot(c, 16); c += b; \
    b -= a;  b ^= rot(a, 19); a += c; \
    c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a, b, c) \
{ \
    c ^= b; c -= rot(b, 14); \
    a ^= c; a -= rot(c, 11); \
    b ^= a; b -= rot(a, 25); \
    c ^= b; c -= rot(b, 16); \
    a ^= c; a -= rot(c, 4);  \
    b ^= a; b -= rot(a, 14); \
    c ^= b; c -= rot(b, 24); \
}

/* Functions */
uint32_t hash(const char *key, ssize_t len);
uint32_t ipv4_hash(uint32_t ip, int bsize);
uint32_t ipv6_hash(uint8_t *ip, int bsize);

#endif  /* _LIB_BSP_CORE_HASH_H */
