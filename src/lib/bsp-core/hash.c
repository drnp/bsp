/*
 * hash.c
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
 * Hash algorisims
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/07/2012
 * @changelog 
 *      [06/07/2012] - Creation
 *      [12/18/2012] - Move macros here
 *      [04/07/2013] - IPv4 hash added
 */

#include "config.h"

#include "bsp.h"

/* String hash */
uint32_t hash(const char *key, ssize_t len)
{
    if (!key)
    {
        return 0;
    }

    if (len <= 0)
    {
        len = strlen(key);
    }
    
	uint32_t a, b, c;
	uint32_t *k = (uint32_t *) key;
	a = (uint32_t) 0xdeadface + (uint32_t) len;
	b = (uint32_t) 0xfacebeef + (uint32_t) len;
	c = (uint32_t) 0xbeefdead + (uint32_t) len;

	// Pass every 12 bytes
	while (len > 12)
	{
		a += k[0];
		b += k[1];
		c += k[2];

		mix(a, b, c);
			
		len -= 12;
		k += 3;
	}

	mix(a, b, c);

	// Mix data
#ifdef ENDIAN_LITTLE
	switch (len)
    {
        case 12: c += k[2]; b += k[1]; a += k[0]; break;
        case 11: c += k[2] & 0xffffff; b += k[1]; a += k[0]; break;
        case 10: c += k[2] & 0xffff; b += k[1]; a += k[0]; break;
        case 9 : c += k[2] & 0xff; b += k[1]; a += k[0]; break;
        case 8 : b += k[1]; a += k[0]; break;
        case 7 : b += k[1] & 0xffffff; a += k[0]; break;
        case 6 : b += k[1] & 0xffff; a += k[0]; break;
        case 5 : b += k[1] & 0xff; a += k[0]; break;
        case 4 : a += k[0]; break;
        case 3 : a += k[0] & 0xffffff; break;
        case 2 : a += k[0] & 0xffff; break;
        case 1 : a += k[0] & 0xff; break;
        case 0 : return c;
    }
#else
    switch (len)
    {
        case 12: c += k[2]; b += k[1]; a += k[0]; break;
        case 11: c += k[2] & 0xffffff00; b += k[1]; a += k[0]; break;
        case 10: c += k[2] & 0xffff0000; b += k[1]; a += k[0]; break;
        case 9 : c += k[2] & 0xff000000; b += k[1]; a += k[0]; break;
        case 8 : b += k[1]; a += k[0]; break;
        case 7 : b += k[1] & 0xffffff00; a += k[0]; break;
        case 6 : b += k[1] & 0xffff0000; a += k[0]; break;
        case 5 : b += k[1] & 0xff000000; a += k[0]; break;
        case 4 : a += k[0]; break;
        case 3 : a += k[0] & 0xffffff00; break;
        case 2 : a += k[0] & 0xffff0000; break;
        case 1 : a += k[0] & 0xff000000; break;
        case 0 : return c;
    }
#endif  /* ENDIAN */
    final(a, b, c);
    
    return c;
}

/* IPv4 (uint32_t) hash */
uint32_t ipv4_hash(uint32_t ip, int bsize)
{
    uint32_t h = 0;

    h = ip % bsize;
    ip /= bsize;
    h ^= ip % bsize;
    h ^= ip / bsize;

    return h;
}

/* IPv6 (uint8_t * 16) hash */
uint32_t ipv6_hash(uint8_t *ip, int bsize)
{
    uint32_t h = 0;

    h = ((ip[0] << 24) + (ip[2] << 16) + (ip[4] << 8) + ip[6]) % bsize;
    h ^= ((ip[1] << 24) + (ip[3] << 16) + (ip[5] << 8) + ip[7]) % bsize;
    h ^= ((ip[8] << 24) + (ip[10] << 16) + (ip[12] << 8) + ip[14]) % bsize;
    h ^= ((ip[9] << 24) + (ip[11] << 16) + (ip[13] << 8) + ip[15]) % bsize;
    h ^= ((ip[0] << 24) + 
          (ip[1] << 23) + 
          (ip[2] << 22) + 
          (ip[3] << 21) + 
          (ip[4] << 20) + 
          (ip[5] << 19) + 
          (ip[6] << 18) + 
          (ip[7] << 17) + 
          (ip[8] << 16) + 
          (ip[9] << 15) + 
          (ip[10] << 14) + 
          (ip[11] << 13) + 
          (ip[12] << 12) + 
          (ip[13] << 11) + 
          (ip[14] << 10) + 
          (ip[15] << 9)) / bsize;

    return h;
}
