/*
 * string.c
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
 * String operator
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/09/2013
 * @changelog 
 *      [06/14/2012] - Creation
 *      [04/10/2013] - string_fill() added
 *      [04/16/2013] - Remove free list
 *      [05/09/2013] - Patch for zlib < 1.2.7
 */

#define _GNU_SOURCE

#include "bsp.h"
#include "snappy-c.h"
#include "minilzo.h"
#include "zlib.h"

#if defined(ZLIB_CONST) && !defined(z_const)
#   define z_const const
#else
#   define z_const
#endif

int lzo_initialized = 0;

// New string
BSP_STRING * new_string(const char *data, ssize_t len)
{
    BSP_STRING *ret = mempool_alloc(sizeof(BSP_STRING));
    if (!ret)
    {
        trace_msg(TRACE_LEVEL_ERROR, "String : Create string error");
        return NULL;
    }

    ret->str_len = 0;
    ret->ori_len = 0;
    ret->str = NULL;
    ret->compressed = COMPRESS_TYPE_NONE;
    
    if (data)
    {
        if (len < 0)
        {
            len = strlen(data) + 1;
        }

        ret->str = (char *) mempool_alloc(len);
        
        if (ret->str)
        {
            if (0 < strlen(data))
            {
                // Available data. If the first byte of data is NULL (0x0), means jst hold space for comming data
                memcpy(ret->str, data, len);
            }
            
            ret->str_len = strlen(data);
            ret->ori_len = len;
            if (ret->str_len >= len)
            {
                ret->str_len = len - 1;
            }
            
            ret->str[len] = 0x0;
        }
    }

    else
    {
        if (len > 0)
        {
            ret->str = (char *) mempool_alloc(len);
        }
    }

    return ret;
}

// Return string to free list
void del_string(BSP_STRING *str)
{
    if (!str)
    {
        return;
    }
    
    if (str->str)
    {
        mempool_free(str->str);
        str->str = NULL;
    }

    mempool_free(str);
    
    return;
}

// All data gone?
void clean_string(BSP_STRING *str)
{
    if (str)
    {
        str->str_len = 0;
        str->ori_len = 0;
        str->compressed = 0;

        if (str->str)
        {
            mempool_free(str->str);
            str->str = NULL;
        }
    }
    
    return;
}

// Duplicate
BSP_STRING * clone_string(BSP_STRING *str)
{
    if (!str)
    {
        return NULL;
    }

    return new_string(str->str, str->ori_len);
}

// Append data to string
ssize_t string_append(BSP_STRING *str, const char *data, ssize_t len)
{
    if (!str || !data)
    {
        return 0;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    // Try to realloc
    char *new_str = (char *) mempool_realloc(str->str, str->ori_len + len + 1);
    if (!new_str)
    {
        // Alloc error
        return -1;
    }

    memcpy(new_str + str->ori_len, data, len);
    new_str[str->ori_len + len] = 0x0;
    str->str = new_str;
    str->ori_len += len;
    str->str_len = strlen(str->str);

    return len;
}

// Fill (enlarge) string
ssize_t string_fill(BSP_STRING *str, int code, size_t len)
{
    if (!str || len <= str->ori_len)
    {
        // Nothing to do
        return 0;
    }

    // Try to realloc
    char *new_str = (char *) mempool_realloc(str->str, len);
    if (!new_str)
    {
        // Alloc error
        return -1;
    }

    if (code >= 0)
    {
        memset(new_str + str->ori_len, (char) code, len - str->ori_len);
    }

    str->str = new_str;
    str->ori_len = len;
    str->str_len = 0;

    return len;
}

// Formatted append
size_t string_printf(BSP_STRING *str, const char *fmt, ...)
{
    if (!str || !fmt)
    {
        return 0;
    }
    
    char *tmp = NULL;
    va_list ap;
    va_start(ap, fmt);
    int len = vasprintf(&tmp, fmt, ap);
    va_end(ap);

    len = string_append(str, tmp, len);
    free(tmp);
    
    return len;
}

// Replace needle
void string_replace(BSP_STRING *str, const char *search, const char *replace)
{
    if (!str || !str->str)
    {
        return;
    }
    
    char *p, *pi;
    size_t lens = strlen(search);
    size_t lenr = strlen(replace);

    // Make a copy first
    char *dup = (char *) mempool_alloc(str->ori_len);
    strncpy(dup, str->str, str->ori_len);
    clean_string(str);

    pi = dup;
    p = strstr(pi, search);

    while (p)
    {
        string_append(str, pi, (p - pi));
        string_append(str, replace, lenr);
        
        pi = p + lens;
        p = strstr(pi, search);
    }

    // Last~~
    string_append(str, pi, -1);

    return;
}

// Compress / Uncompress with zlib deflate (stream without header)
int string_compress_deflate(BSP_STRING *str)
{
    if (!str || !str->str || COMPRESS_TYPE_NONE != str->compressed)
    {
        // You should not compress a string twice
        return BSP_RTN_FATAL;
    }

    z_stream strm;
    unsigned char chunk[COMPRESS_ZLIB_CHUNK_SIZE];
    ssize_t chunk_data_size = 0;
    int ret;
    
    /* TODO : Switch to mempool_* functions */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    BSP_STRING *new_str = new_string(NULL, 0);
    if (new_str)
    {
        if (Z_OK == deflateInit(&strm, Z_DEFAULT_COMPRESSION))
        {
            strm.avail_in = str->ori_len;
            strm.next_in = (z_const Bytef *) str->str;

            do
            {
                strm.avail_out = COMPRESS_ZLIB_CHUNK_SIZE;
                strm.next_out = chunk;

                ret = deflate(&strm, Z_FINISH);
                if (Z_STREAM_ERROR == ret)
                {
                    // Stream error
                    (void) deflateEnd(&strm);
                    return BSP_RTN_ERROR_GENERAL;
                }

                chunk_data_size = COMPRESS_ZLIB_CHUNK_SIZE - strm.avail_out;
                string_append(new_str, (const char *) chunk, chunk_data_size);
            } while (strm.avail_out == 0);

            // Copy data
            mempool_free(str->str);
            str->str = new_str->str;
            new_str->str = NULL;
            str->compressed = COMPRESS_TYPE_DEFLATE;
            str->ori_len = new_str->ori_len;
            str->str_len = 0;

            del_string(new_str);
            (void) deflateEnd(&strm);

            return BSP_RTN_SUCCESS;
        }

        del_string(new_str);

        return BSP_RTN_ERROR_GENERAL;
    }

    return BSP_RTN_ERROR_MEMORY;
}

int string_uncompress_deflate(BSP_STRING *str)
{
    if (!str || !str->str || COMPRESS_TYPE_DEFLATE != str->compressed)
    {
        return BSP_RTN_FATAL;
    }

    z_stream strm;
    unsigned char chunk[COMPRESS_ZLIB_CHUNK_SIZE];
    ssize_t chunk_data_size = 0;
    int ret;
    
    /* TODO : Switch to mempool_* functions */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    BSP_STRING *new_str = new_string(NULL, 0);
    if (new_str)
    {
        if (Z_OK == inflateInit(&strm))
        {
            strm.avail_in = str->ori_len;
            strm.next_in = (z_const Bytef *) str->str;

            do
            {
                strm.avail_out = COMPRESS_ZLIB_CHUNK_SIZE;
                strm.next_out = chunk;

                ret = inflate(&strm, Z_NO_FLUSH);
                switch (ret)
                {
                    case Z_OK : 
                    case Z_STREAM_END : 
                        // Data OK
                        chunk_data_size = COMPRESS_ZLIB_CHUNK_SIZE - strm.avail_out;
                        string_append(new_str, (const char *) chunk, chunk_data_size);
                        break;
                    case Z_MEM_ERROR : 
                        (void) inflateEnd(&strm);
                        return BSP_RTN_ERROR_MEMORY;
                        break;
                    case Z_NEED_DICT : 
                        ret = Z_DATA_ERROR;
                    case Z_DATA_ERROR : 
                    case Z_STREAM_ERROR : 
                    default : 
                        // Data error
                        (void) inflateEnd(&strm);
                        return BSP_RTN_ERROR_GENERAL;
                }
            } while (strm.avail_out == 0);

            // Copy data
            string_append(new_str, "\0", 1);
            mempool_free(str->str);
            str->str = new_str->str;
            new_str->str = NULL;
            str->compressed = COMPRESS_TYPE_NONE;
            str->ori_len = new_str->ori_len;
            str->str_len = strlen(str->str);

            del_string(new_str);
            (void) inflateEnd(&strm);

            return BSP_RTN_SUCCESS;
        }

        del_string(new_str);

        return BSP_RTN_ERROR_MEMORY;
    }

    return BSP_RTN_ERROR_MEMORY;
}

// Compress / Uncompress with mini-lzo
int string_compress_lzo(BSP_STRING *str)
{
    if (!str || !str->str || COMPRESS_TYPE_NONE != str->compressed)
    {
        // You should not compress a string twice
        return BSP_RTN_FATAL;
    }

    if (0 == lzo_initialized)
    {
        // Initialize first
        lzo_initialized = (lzo_init() == LZO_E_OK) ? 1 : 2;
    }

    if (1 != lzo_initialized)
    {
        // LZO failed
        trace_msg(TRACE_LEVEL_ERROR, "String : LZO initialize failed");
        return BSP_RTN_ERROR_GENERAL;
    }

    lzo_align_t __LZO_MMODEL wkrmem[(LZO1X_1_MEM_COMPRESS + sizeof(lzo_align_t)) / sizeof(lzo_align_t)];
    size_t compressed_size = str->ori_len + (str->ori_len >> 4) + 67;
    // Additional 8 bytes for orininal length, 1 byte for string terminal
    char *new_str = mempool_alloc(compressed_size + 8);
    if (new_str)
    {
        set_int64((int64_t) str->ori_len, new_str);
        if (LZO_E_OK == lzo1x_1_compress((const unsigned char *) str->str, str->ori_len, (unsigned char *) new_str + 8, &compressed_size, wkrmem))
        {
            mempool_free(str->str);
            str->str = new_str;
            str->compressed = COMPRESS_TYPE_LZO;
            str->ori_len = compressed_size + 8;
            str->str_len = 0;

            return BSP_RTN_SUCCESS;
        }

        else
        {
            mempool_free(new_str);
        }

        return BSP_RTN_ERROR_GENERAL;
    }

    return BSP_RTN_ERROR_MEMORY;
}

int string_uncompress_lzo(BSP_STRING *str)
{
    if (!str || !str->str || COMPRESS_TYPE_LZO != str->compressed)
    {
        return BSP_RTN_FATAL;
    }

    if (0 == lzo_initialized)
    {
        // Initialize first
        lzo_initialized = (lzo_init() == LZO_E_OK) ? 1 : 2;
    }

    if (1 != lzo_initialized)
    {
        // LZO failed
        trace_msg(TRACE_LEVEL_ERROR, "String : LZO initialize failed");
        return BSP_RTN_ERROR_GENERAL;
    }

    size_t uncompressed_size = (size_t) get_int64(str->str);
    char *new_str = mempool_alloc(uncompressed_size + 1);
    if (new_str)
    {
        if (LZO_E_OK == lzo1x_decompress((const unsigned char *) str->str + 8, str->ori_len - 8, (unsigned char *) new_str, &uncompressed_size, NULL))
        {
            new_str[uncompressed_size] = 0x0;
            mempool_free(str->str);
            str->str = new_str;
            str->compressed = COMPRESS_TYPE_NONE;
            str->ori_len = uncompressed_size;
            str->str_len = strlen(str->str);

            return BSP_RTN_SUCCESS;
        }

        else
        {
            mempool_free(new_str);

            return BSP_RTN_ERROR_GENERAL;
        }
    }

    return BSP_RTN_ERROR_MEMORY;
}

// Compress / Uncompress with Google snappy
int string_compress_snappy(BSP_STRING *str)
{
    if (!str || !str->str || COMPRESS_TYPE_NONE != str->compressed)
    {
        // You should not compress a string twice
        return BSP_RTN_FATAL;
    }

    size_t compressed_size = snappy_max_compressed_length(str->ori_len);
    char *new_str = mempool_alloc(compressed_size);
    if (new_str)
    {
        if (SNAPPY_OK == snappy_compress(str->str, str->ori_len, new_str, &compressed_size))
        {
            mempool_free(str->str);
            str->str = new_str;
            str->compressed = COMPRESS_TYPE_SNAPPY;
            str->ori_len = compressed_size;
            str->str_len = 0;

            return BSP_RTN_SUCCESS;
        }

        else
        {
            mempool_free(new_str);
        }

        return BSP_RTN_ERROR_GENERAL;
    }
    
    return BSP_RTN_ERROR_MEMORY;
}

int string_uncompress_snappy(BSP_STRING *str)
{
    if (!str || !str->str || COMPRESS_TYPE_SNAPPY != str->compressed)
    {
        return BSP_RTN_FATAL;
    }

    size_t uncompressed_size;
    if (SNAPPY_OK == snappy_uncompressed_length(str->str, str->ori_len, &uncompressed_size))
    {
        char *new_str = mempool_alloc(uncompressed_size + 1);
        if (new_str)
        {
            if (SNAPPY_OK == snappy_uncompress(str->str, str->ori_len, new_str, &uncompressed_size))
            {
                new_str[uncompressed_size] = 0x0;
                mempool_free(str->str);
                str->str = new_str;
                str->compressed = COMPRESS_TYPE_NONE;
                str->ori_len = uncompressed_size;
                str->str_len = strlen(str->str);

                return BSP_RTN_SUCCESS;
            }

            else
            {
                mempool_free(new_str);

                return BSP_RTN_ERROR_GENERAL;
            }
        }

        else
        {
            return BSP_RTN_ERROR_MEMORY;
        }
    }
    
    return BSP_RTN_ERROR_GENERAL;
}

// Base64
char *base64_enc_idx = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int base64_dec_idx[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 64, -1, -1, 
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

int string_base64_encode(BSP_STRING *str, const char *data, ssize_t len)
{
    if (!str || !data)
    {
        return BSP_RTN_FATAL;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    size_t i;
    size_t remaining = len;
    int c1, c2, c3;
    char tmp[4];

    while (remaining > 0)
    {
        i = len - remaining;
        
        c1 = (unsigned char) data[i];
        c2 = (remaining > 1) ? (unsigned char) data[i + 1] : 0;
        c3 = (remaining > 2) ? (unsigned char) data[i + 2] : 0;
        
        tmp[0] = base64_enc_idx[                  (c1 >> 2) & 63];
        tmp[1] = base64_enc_idx[(((c1 & 3) << 4) | c2 >> 4) & 63];
        tmp[2] = (remaining > 1) ? base64_enc_idx[(((c2 & 15) << 2) | c3 >> 6) & 63] : '=';
        tmp[3] = (remaining > 2) ? base64_enc_idx[                          c3 & 63] : '=';

        string_append(str, tmp, 4);

        if (remaining < 3)
        {
            break;
        }
        
        remaining -= 3;
    }

    return BSP_RTN_SUCCESS;
}

int string_base64_decode(BSP_STRING *str, const char *data, ssize_t len)
{
    if (!str || !data)
    {
        return BSP_RTN_FATAL;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    size_t i;
    size_t remaining = len;
    int c1, c2, c3, c4;
    char tmp[3] = {0, 0, 0};

    while (remaining > 0)
    {
        i = len - remaining;
        
        c1 = (unsigned char) data[i];
        c2 = (remaining > 1) ? (unsigned char) data[i + 1] : 61;
        c3 = (remaining > 2) ? (unsigned char) data[i + 2] : 61;
        c4 = (remaining > 3) ? (unsigned char) data[i + 3] : 61;

        if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0)
        {
            // Invalid base64 stream
            return BSP_RTN_ERROR_GENERAL;
        }

        c1 = base64_dec_idx[c1];
        c2 = base64_dec_idx[c2];
        c3 = base64_dec_idx[c3];
        c4 = base64_dec_idx[c4];

        tmp[0] = (c2 == 64) ? 0 : (c1 << 2 | c2 >> 4);
        tmp[1] = (c3 == 64) ? 0 : (c2 << 4 | c3 >> 2);
        tmp[2] = (c4 == 64) ? 0 : (c3 << 6 | c4);

        string_append(str, tmp, 3);

        if (remaining < 4)
        {
            break;
        }

        remaining -= 4;
    }

    return BSP_RTN_SUCCESS;
}
