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
 * Binary-safe string
 * 
 * @package bsp::libbsp-core
 * @author Dr.NP <np@bsgroup.org>
 * @update 05/09/2013
 * @changelog 
 *      [06/14/2012] - Creation
 *      [04/10/2013] - string_fill() added
 *      [04/16/2013] - Remove free list
 *      [05/09/2013] - Patch for zlib < 1.2.7
 *      [05/21/2014] - lz4 instead of mini-lzo
 */

#define _GNU_SOURCE

#include "bsp.h"
#include "zlib.h"

#ifdef ENABLE_SNAPPY
    #include <snappy-c.h>
#endif

#ifdef ENABLE_LZ4
    #include <lz4.h>
#endif

#if defined(ZLIB_CONST) && !defined(z_const)
#   define z_const const
#else
#   define z_const
#endif

// New string
BSP_STRING * new_string(const char *data, ssize_t len)
{
    BSP_STRING *ret = bsp_malloc(sizeof(BSP_STRING));
    if (!ret)
    {
        trace_msg(TRACE_LEVEL_ERROR, "String : Create string error");
        return NULL;
    }

    ret->str = NULL;
    ret->original_len = 0;
    ret->compressed_len = 0;
    ret->compress_type = COMPRESS_TYPE_NONE;
    
    if (data)
    {
        if (len < 0)
        {
            len = strlen(data);
        }

        ret->str = (char *) bsp_malloc(len);
        if (ret->str)
        {
            memcpy(ret->str, data, len);
            ret->original_len = len;
        }
    }
    else
    {
        if (len > 0)
        {
            ret->str = (char *) bsp_malloc(len);
        }
    }

    return ret;
}

// Create string from an ordinary file
BSP_STRING * new_string_from_file(const char *path)
{
    if (!path)
    {
        return NULL;
    }
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        trace_msg(TRACE_LEVEL_ERROR, "String : Cannot open file for input");
        return NULL;
    }
    
    BSP_STRING *ret = new_string(NULL, 0);
    if (!ret)
    {
        fclose(fp);
        return NULL;
    }
    char buffer[8192];
    size_t len;
    while (!feof(fp) && !ferror(fp))
    {
        len = fread(buffer, 1, 8192, fp);
        string_append(ret, buffer, len);
    }
    fclose(fp);
    
    return ret;
}

// Return string to free list
void del_string(BSP_STRING *str)
{
    if (!str)
    {
        return;
    }
    if (STR_STR(str))
    {
        bsp_free(STR_STR(str));
    }
    bsp_free(str);
    
    return;
}

// All data gone?
void clean_string(BSP_STRING *str)
{
    if (str)
    {
        STR_LEN(str) = 0;
        str->compressed_len = 0;
        str->compress_type = COMPRESS_TYPE_NONE;

        if (STR_STR(str))
        {
            bsp_free(STR_STR(str));
            STR_STR(str) = NULL;
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

    BSP_STRING *new = new_string(NULL, 0);
    if (new)
    {
        char *new_str = bsp_malloc(STR_LEN(str));
        if (new_str)
        {
            memcpy(new_str, STR_STR(str), STR_LEN(str));
            new->original_len = STR_LEN(str);
            new->compress_type = str->compress_type;
            new->compressed_len = str->compressed_len;
            new->str = new_str;
        }
    }
    
    return new;
}

/* Operators */
// Append data to string
ssize_t string_append(BSP_STRING *str, const char *data, ssize_t len)
{
    if (!str || !data || !len || COMPRESS_TYPE_NONE != str->compress_type)
    {
        return 0;
    }

    if (len < 0)
    {
        len = strlen(data);
    }

    // Try to realloc
    char *new_str = (char *) bsp_realloc(STR_STR(str), STR_LEN(str) + len);
    if (!new_str)
    {
        // Alloc error
        return -1;
    }

    memcpy(new_str + STR_LEN(str), data, len);
    STR_STR(str) = new_str;
    STR_LEN(str) += len;

    return len;
}

// Fill (enlarge) string
ssize_t string_fill(BSP_STRING *str, int code, size_t len)
{
    if (!str || !len || COMPRESS_TYPE_NONE != str->compress_type)
    {
        // Nothing to do
        return 0;
    }

    // Try to realloc
    char *new_str = (char *) bsp_realloc(STR_STR(str), STR_LEN(str) + len);
    if (!new_str)
    {
        // Alloc error
        return -1;
    }

    memset(new_str + STR_LEN(str), (char) code, len);
    STR_STR(str) = new_str;
    STR_LEN(str) += len;

    return len;
}

// Formatted append
ssize_t string_printf(BSP_STRING *str, const char *fmt, ...)
{
    if (!str || !fmt || COMPRESS_TYPE_NONE != str->compress_type)
    {
        return 0;
    }
    
    char *tmp = NULL;
    va_list ap;
    va_start(ap, fmt);
    int len = vasprintf(&tmp, fmt, ap);
    va_end(ap);

    if (tmp && len > 0)
    {
        len = string_append(str, tmp, len);
        free(tmp);
    }
    
    return len;
}

// Replace needle
void string_replace(BSP_STRING *str, const char *search, ssize_t search_len, const char *replace, ssize_t replace_len)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_NONE != str->compress_type)
    {
        return;
    }

    if (search_len < 0)
    {
        search_len = strlen(search);
    }
    if (replace_len < 0)
    {
        replace_len = strlen(replace);
    }

    // Make a copy first
    char *dup = STR_STR(str);
    size_t dup_len = STR_LEN(str);
    STR_LEN(str) = 0;
    STR_STR(str) = NULL;

    size_t i, s = 0;
    for (i = 0; i < dup_len - search_len; i ++)
    {
        if (0 == memcmp(str->str + i, search, search_len))
        {
            // Found
            if (i > s)
            {
                string_append(str, dup + s, i - s);
            }
            string_append(str, replace, replace_len);
            i += (search_len - 1);
            s = i + 1;
        }
    }
    if (s < dup_len)
    {
        string_append(str, dup + s, dup_len - s);
    }

    return;
}

// String length (\0 terminated)
ssize_t string_strlen(BSP_STRING *str)
{
    if (!str)
    {
        return -1;
    }

    if (!STR_STR(str) || !STR_LEN(str))
    {
        return 0;
    }
    
    size_t i;
    for (i = 0; i < STR_LEN(str); i ++)
    {
        if (0 == str->str[i])
        {
            return i;
        }
    }

    return STR_LEN(str);
}

// Compress / Decompress with zlib deflate (stream without header)
int string_compress_deflate(BSP_STRING *str)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_NONE != str->compress_type)
    {
        // You should not compress a string twice
        return BSP_RTN_FATAL;
    }

    z_stream strm;
    unsigned char chunk[COMPRESS_ZLIB_CHUNK_SIZE];
    ssize_t chunk_data_size = 0;
    int ret;
    
#ifdef ENABLE_MEMPOOL
    strm.zalloc = mempool_alloc;
    strm.zfree = mempool_free;
#else
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
#endif
    strm.opaque = Z_NULL;
    
    char *dup = STR_STR(str);
    size_t dup_len = STR_LEN(str);
    STR_LEN(str) = 0;
    STR_STR(str) = NULL;
    
    if (Z_OK == deflateInit(&strm, Z_DEFAULT_COMPRESSION))
    {
        strm.avail_in = dup_len;
        strm.next_in = (z_const Bytef *) dup;
        
        do
        {
            strm.avail_out = COMPRESS_ZLIB_CHUNK_SIZE;
            strm.next_out = chunk;
            
            ret = deflate(&strm, Z_FINISH);
            if (Z_STREAM_ERROR == ret)
            {
                // Stream error
                (void) deflateEnd(&strm);
                STR_STR(Str) = dup;
                STR_LEN(str) = dup_len;
                return BSP_RTN_ERROR_GENERAL;
            }
            
            chunk_data_size = COMPRESS_ZLIB_CHUNK_SIZE - strm.avail_out;
            string_append(str, (const char *) chunk, chunk_data_size);
        } while (strm.avail_out == 0);
        
        bsp_free(dup);
        str->compress_type = COMPRESS_TYPE_DEFLATE;
        str->compressed_len = STR_LEN(str);
        STR_LEN(str) = dup_len;
        (void) deflateEnd(&strm);
        
        return BSP_RTN_SUCCESS;
    }
    
    return BSP_RTN_ERROR_GENERAL;
}

int string_decompress_deflate(BSP_STRING *str)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_DEFLATE != str->compress_type)
    {
        return BSP_RTN_FATAL;
    }

    z_stream strm;
    unsigned char chunk[COMPRESS_ZLIB_CHUNK_SIZE];
    ssize_t chunk_data_size = 0;
    int ret;
#ifdef ENABLE_MEMPOOL
    strm.zalloc = mempool_alloc;
    strm.zfree = mempool_free;
#else
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
#endif
    strm.opaque = Z_NULL;
    
    char *dup = STR_STR(str);
    size_t dup_len = str->compressed_len;
    size_t old_ori = STR_LEN(str);
    STR_LEN(str) = 0;
    STR_STR(str) = NULL;
    
    if (Z_OK == inflateInit(&strm))
    {
        strm.avail_in = dup_len;
        strm.next_in = (z_const Bytef *) dup;
        
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
                    string_append(str, (const char *) chunk, chunk_data_size);
                    break;
                case Z_MEM_ERROR : 
                    (void) inflateEnd(&strm);
                    STR_STR(str) = dup;
                    str->compressed_len = dup_len;
                    STR_LEN(str) = old_ori;
                    return BSP_RTN_ERROR_MEMORY;
                    break;
                case Z_NEED_DICT : 
                    ret = Z_DATA_ERROR;
                case Z_DATA_ERROR : 
                case Z_STREAM_ERROR : 
                default : 
                    // Data error
                    (void) inflateEnd(&strm);
                    STR_STR(str) = dup;
                    str->compressed_len = dup_len;
                    STR_LEN(str) = old_ori;
                    return BSP_RTN_ERROR_GENERAL;
            }
        } while (strm.avail_out == 0);

        bsp_free(dup);
        str->compress_type = COMPRESS_TYPE_NONE;
        str->compressed_len = 0;
        (void) inflateEnd(&strm);

        return BSP_RTN_SUCCESS;
    }
    
    return BSP_RTN_ERROR_GENERAL;
}

#ifdef ENABLE_SNAPPY
// Compress / Decompress with Google snappy
int string_compress_snappy(BSP_STRING *str)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_NONE != str->compress_type)
    {
        // You should not compress a string twice
        return BSP_RTN_FATAL;
    }

    size_t compressed_size = snappy_max_compressed_length(STR_LEN(str));
    char *new_str = bsp_malloc(compressed_size);
    if (new_str)
    {
        if (SNAPPY_OK == snappy_compress(STR_STR(str), STR_LEN(str), new_str, &compressed_size))
        {
            bsp_free(STR_STR(str));
            STR_STR(str) = new_str;
            str->compress_type = COMPRESS_TYPE_SNAPPY;
            str->compressed_len = compressed_size;

            return BSP_RTN_SUCCESS;
        }
        else
        {
            bsp_free(new_str);
        }
        return BSP_RTN_ERROR_GENERAL;
    }
    
    return BSP_RTN_ERROR_MEMORY;
}

int string_decompress_snappy(BSP_STRING *str)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_SNAPPY != str->compress_type)
    {
        return BSP_RTN_FATAL;
    }

    size_t uncompressed_size;
    if (SNAPPY_OK == snappy_uncompressed_length(STR_STR(str), STR_LEN(str), &uncompressed_size))
    {
        char *new_str = bsp_malloc(uncompressed_size);
        if (new_str)
        {
            if (SNAPPY_OK == snappy_uncompress(STR_STR(str), str->compressed_len, new_str, &uncompressed_size))
            {
                bsp_free(STR_STR(str));
                STR_STR(str) = new_str;
                str->compress_type = COMPRESS_TYPE_NONE;
                str->compressed_len = 0;
                STR_LEN(str) = uncompressed_size;

                return BSP_RTN_SUCCESS;
            }
            else
            {
                bsp_free(new_str);
            }
            return BSP_RTN_ERROR_GENERAL;
        }

        else
        {
            return BSP_RTN_ERROR_MEMORY;
        }
    }
    
    return BSP_RTN_ERROR_GENERAL;
}
#endif

#ifdef ENABLE_LZ4
// Compress / Decompress with LZ4
int string_compress_lz4(BSP_STRING *str)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_NONE != str->compress_type)
    {
        // You should not compress a string twice
        return BSP_RTN_FATAL;
    }

    int compressed_size = LZ4_compressBound(STR_LEN(str));
    int state_size = LZ4_sizeofState();
    
    char *new_str = bsp_malloc(compressed_size);
    char *state = bsp_malloc(state_size);
    if (new_str && state)
    {
        if ((compressed_size = LZ4_compress_withState((void *) state, STR_STR(str), new_str, STR_LEN(str))) > 0)
        {
            bsp_free(STR_STR(str));
            STR_STR(str) = new_str;
            str->compress_type = COMPRESS_TYPE_LZ4;
            str->compressed_len = compressed_size;
            bsp_free(state);
            
            return BSP_RTN_SUCCESS;
        }

        else
        {
            bsp_free(new_str);
            bsp_free(state);
        }

        return BSP_RTN_ERROR_GENERAL;
    }

    if (new_str)
    {
        bsp_free(new_str);
    }
    if (state)
    {
        bsp_free(state);
    }
    
    return BSP_RTN_ERROR_MEMORY;
}

int string_decompress_lz4(BSP_STRING *str)
{
    if (!str || !STR_STR(str) || COMPRESS_TYPE_LZ4 != str->compress_type)
    {
        return BSP_RTN_FATAL;
    }

    char *new_str = bsp_malloc(STR_LEN(str));
    if (new_str)
    {
        if (LZ4_decompress_fast(STR_STR(str), new_str, STR_LEN(str)) == str->compressed_len)
        {
            // Decompress successfully
            bsp_free(STR_STR(str));
            STR_STR(str) = new_str;
            str->compress_type = COMPRESS_TYPE_NONE;
            str->compressed_len = 0;

            return BSP_RTN_SUCCESS;
        }
        else
        {
            bsp_free(new_str);
        }
        return BSP_RTN_ERROR_GENERAL;
    }
    else
    {
        return BSP_RTN_ERROR_MEMORY;
    }
}
#endif

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
