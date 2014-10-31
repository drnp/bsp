/*
 * variable.c
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
 * Varible operator
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/11/2012
 * @changelog 
 *      [06/11/2012] - Creation
 *      [07/09/2013] - set/get pointor added
 */

#include "bsp.h"

BSP_SPINLOCK rand_lock = BSP_SPINLOCK_INITIALIZER;
int rand_fd = -2;

/* Functions for integer/float read and write from general memory address. (BIG_ENDIAN) */
inline void set_bit(int bit, char *addr, size_t idx)
{
    size_t offset = idx >> 3;
    int rears = idx & 7;
    int mask = bit > 0 ? 0xFF : (0xFF - (1 << rears));
    addr[offset] = (char) ((int) addr[offset] & mask);

    return;
}
inline void set_int8(int8_t data, char *addr)
{
    if (addr)
    {
        addr[0] = data;
    }

    return;
}

inline void set_int16(int16_t data, char *addr)
{
    if (addr)
    {
        addr[0] = data >> 8;
        addr[1] = data & 0xFF;
    }

    return;
}

inline void set_int16_le(int16_t data, char *addr)
{
    if (addr)
    {
        addr[1] = data >> 8;
        addr[0] = data & 0xFF;
    }
}

inline void set_int32(int32_t data, char *addr)
{
    if (addr)
    {
        addr[0] = data >> 24;
        addr[1] = (data >> 16) & 0xFF;
        addr[2] = (data >> 8) & 0xFF;
        addr[3] = data & 0xFF;
    }

    return;
}

inline void set_int32_le(int32_t data, char *addr)
{
    if (addr)
    {
        addr[3] = data >> 24;
        addr[2] = (data >> 16) & 0xFF;
        addr[1] = (data >> 8) & 0xFF;
        addr[0] = data & 0xFF;
    }

    return;
}

inline void set_int64(int64_t data, char *addr)
{
    if (addr)
    {
        addr[0] = (int64_t) data >> 56;
        addr[1] = ((int64_t) data >> 48) & 0xFF;
        addr[2] = ((int64_t) data >> 40) & 0xFF;
        addr[3] = ((int64_t) data >> 32) & 0xFF;
        addr[4] = ((int64_t) data >> 24) & 0xFF;
        addr[5] = ((int64_t) data >> 16) & 0xFF;
        addr[6] = ((int64_t) data >> 8) & 0xFF;
        addr[7] = (int64_t) data & 0xFF;
    }

    return;
}

inline void set_int64_le(int64_t data, char *addr)
{
    if (addr)
    {
        addr[7] = (int64_t) data >> 56;
        addr[6] = ((int64_t) data >> 48) & 0xFF;
        addr[5] = ((int64_t) data >> 40) & 0xFF;
        addr[4] = ((int64_t) data >> 32) & 0xFF;
        addr[3] = ((int64_t) data >> 24) & 0xFF;
        addr[2] = ((int64_t) data >> 16) & 0xFF;
        addr[1] = ((int64_t) data >> 8) & 0xFF;
        addr[0] = (int64_t) data & 0xFF;
    }

    return;
}

inline int set_vint29(int data, char *addr)
{
    int len = 0;
    if (addr)
    {
        if (0 == data >> 7)
        {
            // 1 byte
            addr[0] = data & 0x7F;
            len = 1;
        }
        else if (0 == data >> 14)
        {
            // 2 bytes
            addr[0] = ((data >> 7) & 0x7F) | 0x80;
            addr[1] = data & 0x7F;
            len = 2;
        }
        else if (0 == data >> 21)
        {
            // 3 bytes
            addr[0] = ((data >> 14) & 0x7F) | 0x80;
            addr[1] = ((data >> 7) & 0x7F) | 0x80;
            addr[2] = data & 0x7F;
            len = 3;
        }
        else
        {
            // 4 bytes
            addr[0] = ((data >> 22) & 0x7F) | 0x80;
            addr[1] = ((data >> 15) & 0x7F) | 0x80;
            addr[2] = ((data >> 8) & 0x7F) | 0x80;
            addr[3] = data & 0xFF;
            len = 4;
        }
    }

    return len;
}

inline int set_vint(int64_t data, char *addr)
{
    int len = 0;
    if (addr)
    {
        if (0 == data >> 7)
        {
            // 1 byte
            addr[0] = data & 0x7F;
            len = 1;
        }
        else if (0 == data >> 14)
        {
            // 2 bytes
            addr[0] = ((data >>  7) & 0x7F) | 0x80;
            addr[1] = data & 0x7F;
            len = 2;
        }
        else if (0 == data >> 21)
        {
            // 3 bytes
            addr[0] = ((data >> 14) & 0x7F) | 0x80;
            addr[1] = ((data >>  7) & 0x7F) | 0x80;
            addr[2] = data & 0x7F;
            len = 3;
        }
        else if (0 == data >> 28)
        {
            // 4 bytes
            addr[0] = ((data >> 21) & 0x7F) | 0x80;
            addr[1] = ((data >> 14) & 0x7F) | 0x80;
            addr[2] = ((data >>  7) & 0x7F) | 0x80;
            addr[3] = data & 0x7F;
            len = 4;
        }
        else if (0 == data >> 35)
        {
            // 5 bytes
            addr[0] = ((data >> 28) & 0x7F) | 0x80;
            addr[1] = ((data >> 21) & 0x7F) | 0x80;
            addr[2] = ((data >> 14) & 0x7F) | 0x80;
            addr[3] = ((data >>  7) & 0x7F) | 0x80;
            addr[4] = data & 0x7F;
            len = 5;
        }
        else if (0 == data >> 42)
        {
            // 6 bytes
            addr[0] = ((data >> 35) & 0x7F) | 0x80;
            addr[1] = ((data >> 28) & 0x7F) | 0x80;
            addr[2] = ((data >> 21) & 0x7F) | 0x80;
            addr[3] = ((data >> 14) & 0x7F) | 0x80;
            addr[4] = ((data >>  7) & 0x7F) | 0x80;
            addr[5] = data & 0x7F;
            len = 6;
        }
        else if (0 == data >> 49)
        {
            // 7 bytes
            addr[0] = ((data >> 42) & 0x7F) | 0x80;
            addr[1] = ((data >> 35) & 0x7F) | 0x80;
            addr[2] = ((data >> 28) & 0x7F) | 0x80;
            addr[3] = ((data >> 21) & 0x7F) | 0x80;
            addr[4] = ((data >> 14) & 0x7F) | 0x80;
            addr[5] = ((data >>  7) & 0x7F) | 0x80;
            addr[6] = data & 0x7F;
            len = 7;
        }
        else if (0 == data >> 56)
        {
            // 8 bytes
            addr[0] = ((data >> 49) & 0x7F) | 0x80;
            addr[1] = ((data >> 42) & 0x7F) | 0x80;
            addr[2] = ((data >> 35) & 0x7F) | 0x80;
            addr[3] = ((data >> 28) & 0x7F) | 0x80;
            addr[4] = ((data >> 21) & 0x7F) | 0x80;
            addr[5] = ((data >> 14) & 0x7F) | 0x80;
            addr[6] = ((data >>  7) & 0x7F) | 0x80;
            addr[7] = data & 0x7F;
            len = 8;
        }
        else
        {
            // 9 bytes (Full length int64)
            addr[0] = ((data >> 57) & 0x7F) | 0x80;
            addr[1] = ((data >> 50) & 0x7F) | 0x80;
            addr[2] = ((data >> 43) & 0x7F) | 0x80;
            addr[3] = ((data >> 36) & 0x7F) | 0x80;
            addr[4] = ((data >> 29) & 0x7F) | 0x80;
            addr[5] = ((data >> 22) & 0x7F) | 0x80;
            addr[6] = ((data >> 15) & 0x7F) | 0x80;
            addr[7] = ((data >>  8) & 0x7F) | 0x80;
            addr[8] = data & 0xFF;
            len = 9;
        }
    }

    return len;
}

inline void set_float(float data, char *addr)
{
    if (addr)
    {
        memcpy(addr, &data, sizeof(float));
    }

    return;
}

inline void set_double(double data, char *addr)
{
    if (addr)
    {
        memcpy(addr, &data, sizeof(double));
    }

    return;
}

inline void set_string(const char *data, ssize_t len, char *addr)
{
    if (len < 0)
    {
        len = strlen(data);
    }
    
    if (addr)
    {
        memcpy(addr, data, len);
    }

    return;
}

inline void set_pointer(const void *p, char *addr)
{
    intptr_t ptr = (intptr_t) p;
    if (addr)
    {
        memcpy(addr, &ptr, sizeof(intptr_t));
    }

    return;
}

inline int get_bit(const char *addr, size_t idx)
{
    size_t offset = idx >> 3;
    int rears = idx & 7;

    return ((int) addr[offset] >> rears) & 1;
}

inline int8_t get_int8(const char *addr)
{
    return addr ? (int8_t) addr[0] : 0;
}

inline int16_t get_int16(const char *addr)
{
    return addr ? ((int8_t) addr[0] << 8 | (uint8_t) addr[1]) : 0;
}

inline int16_t get_int16_le(const char *addr)
{
    return addr ? ((int8_t) addr[1] << 8 | (uint8_t) addr[0]) : 0;
}

inline int32_t get_int32(const char *addr)
{
    return addr ? ((int8_t) addr[0] << 24 | (uint8_t) addr[1] << 16 | (uint8_t) addr[2] << 8 | (uint8_t) addr[3]) : 0;
}

inline int32_t get_int32_le(const char *addr)
{
    return addr ? ((int8_t) addr[3] << 24 | (uint8_t) addr[2] << 16 | (uint8_t) addr[1] << 8 | (uint8_t) addr[0]) : 0;
}

inline int64_t get_int64(const char *addr)
{
    return addr ? (int64_t) (((int64_t) ((uint8_t) addr[0] << 24 | (uint8_t) addr[1] << 16 | (uint8_t) addr[2] << 8 | (uint8_t) addr[3])) << 32) + 
                    ((uint32_t) ((uint8_t) addr[4] << 24 | (uint8_t) addr[5] << 16 | (uint8_t) addr[6] << 8 | (uint8_t) addr[7]))
        : 0;
}

inline int64_t get_int64_le(const char *addr)
{
    return addr ? (int64_t) (((int64_t) ((uint8_t) addr[7] << 24 | (uint8_t) addr[6] << 16 | (uint8_t) addr[5] << 8 | (uint8_t) addr[4])) << 32) + 
                    ((uint32_t) ((uint8_t) addr[3] << 24 | (uint8_t) addr[2] << 16 | (uint8_t) addr[1] << 8 | (uint8_t) addr[0]))
        : 0;
}

inline int get_vint29(const char *addr, int *len)
{
    int ret = 0;
    int vlen = -1;
    int safe = (len && *len > 0) ? *len : 4;

    while (addr && safe > 0)
    {
        vlen = 0;

        // c[0]
        ret = addr[0] & 0x7F;
        vlen ++;
        if (0 == (addr[0] & 0x80))
            break;

        // c[1]
        if (safe < 2)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[1] & 0x7F);
        vlen ++;
        if (0 == (addr[1] & 0x80))
            break;

        // c[2]
        if (safe < 3)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[2] & 0x7F);
        vlen ++;
        if (0 == (addr[2] & 0x80))
            break;

        // c[3]
        if (safe < 4)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 8) | (addr[3] & 0xFF);
        vlen ++;
        break;
    }

    if (len)
    {
        *len = vlen;
    }

    return ret;
}

inline int64_t get_vint(const char *addr, int *len)
{
    int64_t ret = 0;
    int vlen = -1;
    int safe = (len && *len > 0) ? *len : 9;

    while (addr && safe > 0)
    {
        vlen = 0;

        // c[0]
        ret = addr[0] & 0x7F;
        vlen ++;
        if (0 == (addr[0] & 0x80))
            break;

        // c[1]
        if (safe < 2)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[1] & 0x7F);
        vlen ++;
        if (0 == (addr[1] & 0x80))
            break;

        // c[2]
        if (safe < 3)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[2] & 0x7F);
        vlen ++;
        if (0 == (addr[2] & 0x80))
            break;

        // c[3]
        if (safe < 4)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[3] & 0x7F);
        vlen ++;
        if (0 == (addr[3] & 0x80))
            break;

        // c[4]
        if (safe < 5)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[4] & 0x7F);
        vlen ++;
        if (0 == (addr[4] & 0x80))
            break;

        // c[5]
        if (safe < 6)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[5] & 0x7F);
        vlen ++;
        if (0 == (addr[5] & 0x80))
            break;

        // c[6]
        if (safe < 7)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[6] & 0x7F);
        vlen ++;
        if (0 == (addr[6] & 0x80))
            break;

        // c[7]
        if (safe < 8)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 7) | (addr[7] & 0x7F);
        vlen ++;
        if (0 == (addr[7] & 0x80))
            break;

        // c[8]
        if (safe < 9)
        {
            vlen = -1;
            break;
        }
        ret = (ret << 8) | (addr[8] & 0xFF);
        vlen ++;
        break;
    }

    if (len)
    {
        *len = vlen;
    }

    return ret;
}

inline float get_float(const char *addr)
{
    float ret = 0.0f;
    if (addr)
    {
        memcpy(&ret, addr, sizeof(float));
    }

    return ret;
}


inline double get_double(const char *addr)
{
    double ret = 0.0f;
    if (addr)
    {
        memcpy(&ret, addr, sizeof(double));
    }

    return ret;
}

inline char * get_string(const char *addr)
{
    return (char *) addr;
}

inline void * get_pointer(const char *addr)
{
    intptr_t ptr = 0;
    void *ret = NULL;
    if (addr)
    {
        memcpy(&ptr, addr, sizeof(intptr_t));
        ret = (void *) ptr;
    }

    return ret;
}

inline size_t vintlen(const char *addr, ssize_t safe)
{
    size_t ret = 0;
    if (addr)
    {
        int i;
        int n = (safe < 0 || safe > 9) ? 9 : safe;
        for (i = 0; i < n; i ++)
        {
            ret ++;
            if (0 == (addr[i] & 0x80))
            {
                break;
            }
        }
    }

    return ret;
}

inline size_t vint29len(const char *addr, ssize_t safe)
{
    size_t ret = 0;
    if (addr)
    {
        int i;
        int n = (safe < 0 || safe > 4) ? 4 : safe;
        for (i = 0; i < n; i ++)
        {
            ret ++;
            if (0 == (addr[i] & 0x80))
            {
                break;
            }
        }
    }

    return ret;
}

/* Misc */
// Random value
void get_rand(char *data, size_t len)
{
    uint32_t seed = 0;
    struct timeval tm;

    bsp_spin_lock(&rand_lock);
    if (-2 == rand_fd)
    {
        // First try
        rand_fd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);

        if (-1 == rand_fd)
        {
            // An old old old linux system has no urandom block device
            rand_fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
        }

        if (rand_fd > 0)
        {
            reg_fd(rand_fd, FD_TYPE_GENERAL, NULL);
        }
    }
    bsp_spin_unlock(&rand_lock);
    if (-1 == rand_fd)
    {
        return;
    }

    int i, nbytes = len;
    while (nbytes > 0)
    {
        i = read(rand_fd, data + (len - nbytes), nbytes);
        if (i <= 0)
        {
            continue;
        }

        nbytes -= i;
    }

    gettimeofday(&tm, NULL);
    seed = (getpid() << 0x10) ^ getuid() ^ tm.tv_sec ^ tm.tv_usec;

    for (i = 0; i < len; i ++)
    {
        data[i] ^= rand_r(&seed) & 0xFF;
    }

    return;
}

// Caculate trimmed string length
ssize_t trimmed_strlen(const char *input)
{
    int i;
    ssize_t ret = 0;
    int started = 0;

    if (!input)
    {
        return 0;
    }

    for (i = 0; i < strlen(input); i ++)
    {
        if (!started)
        {
            if ((unsigned char) input[i] > 32)
            {
                started = 1;
            }
        }

        if (started)
        {
            ret ++;
        }
    }

    // Reverse
    if (ret > 0)
    {
        for (i = strlen(input) - 1; i >= 0; i --)
        {
            if ((unsigned char) input[i] <= 32)
            {
                ret --;
            }
            else
            {
                break;
            }
        }
    }

    return ret;
}
// Escape charactors by backslash
const char * escape_char(unsigned char c)
{
    static char *escape_char_list[128] = {
        "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007", 
        "\\b", "\\t", "\\n", "\\u000b", "\\f", "\\r", "\\u000e", "\\u000f", 
        "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017", 
        "\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f", 
        NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\/", 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, "\\\\", NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    };

    if (c >= 0 && c < 127)
    {
        return escape_char_list[c];
    }

    return NULL;
}

// UTF-8 encoder and decoder
int32_t utf8_to_value(const char *data, ssize_t len, int *size)
{
    int32_t value = 0;
    
    if (len < 0)
    {
        len = strlen(data);
    }

    if (!data || !len)
    {
        *size = 0;
        return 0;
    }

    unsigned char c = *data;
    unsigned char u1 = 0, u2 = 0, u3 = 0;
    // Check head first
    if (c >= 0xc2 && c <= 0xcf)
    {
        // Two bytes sequence
        if (len >= 2)
        {
            *size = 2;
            u1 = data[1];
            if (u1 < 0x80 || u1 > 0xbf)
            {
                // Invalid continuation byte
                value = c;
                *size = 1;
            }
            
            value = c & 0x1f;
            value = (value << 6) + (u1 & 0x3f);
        }
        else
        {
            value = c;
            *size = 1;
        }
    }
    else if (c >= 0xe0 && c <= 0xef)
    {
        // Three bytes sequence
        if (len >= 3)
        {
            *size = 3;
            u1 = data[1];
            u2 = data[2];
            if (u1 < 0x80 || u1 > 0xbf || u2 < 0x80 || u2 > 0xbf)
            {
                value = c;
                *size = 1;
            }
            
            value = c & 0x1f;
            value = (value << 6) + (u1 & 0x3f);
            value = (value << 6) + (u2 & 0x3f);
        }
        else
        {
            value = c;
            *size = 1;
        }
    }
    else if (c >= 0xf0 && c <= 0xf4)
    {
        // Four bytes sequence
        if (len >= 4)
        {
            *size = 4;
            u1 = data[1];
            u2 = data[2];
            u3 = data[3];
            if (u1 < 0x80 || u1 > 0xbf || u2 < 0x80 || u2 > 0xbf || u3 < 0x80 || u3 > 0xbf)
            {
                value = c;
                *size = 1;
            }

            value = c & 0x1f;
            value = (value << 6) + (u1 & 0x3f);
            value = (value << 6) + (u2 & 0x3f);
            value = (value << 6) + (u3 & 0x3f);
        }
        else
        {
            value = c;
            *size = 1;
        }
    }
    else if (c >= 0x80)
    {
        // Invalid utf head
        value = c;
        *size = 1;
    }

    if (value > 0x10ffff)
    {
        // Not in unicode range
        value = c;
        *size = 1;
    }
    else if (value >= 0xd800 && value <= 0xdfff)
    {
        // Two UTF-16
        value = c;
        *size = 1;
    }
    else if ((*size == 2 && value < 0x80) || (*size == 3 && value < 0x800) || (*size == 4 && value < 0x10000))
    {
        // Overlong
        value = c;
        *size = 1;
    }

    return value;
}
