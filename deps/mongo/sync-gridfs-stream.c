/* sync-gridfs-stream.c - libmongo-client GridFS streaming implementation
 * Copyright 2011, 2012 Gergely Nagy <algernon@balabit.hu>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file src/sync-gridfs-stream.c
 * MongoDB GridFS Streaming API implementation.
 */

#include "sync-gridfs-stream.h"
#include "libmongo-private.h"

#include <unistd.h>
#include <errno.h>

mongo_sync_gridfs_stream *
mongo_sync_gridfs_stream_find (mongo_sync_gridfs *gfs,
                               const bson *query)
{
  mongo_sync_gridfs_stream *stream;
  bson *meta = NULL;
  bson_cursor *c;
  mongo_packet *p;
  const guint8 *oid;

  if (!gfs)
    {
      errno = ENOTCONN;
      return NULL;
    }
  if (!query)
    {
      errno = EINVAL;
      return NULL;
    }

  p = mongo_sync_cmd_query (gfs->conn, gfs->ns.files, 0, 0, 1, query, NULL);
  if (!p)
    return NULL;

  stream = g_new0 (mongo_sync_gridfs_stream, 1);
  stream->gfs = gfs;
  stream->file.type = LMC_GRIDFS_FILE_STREAM_READER;

  mongo_wire_reply_packet_get_nth_document (p, 1, &meta);
  bson_finish (meta);
  mongo_wire_packet_free (p);

  c = bson_find (meta, "_id");
  if (!bson_cursor_get_oid (c, &oid))
    {
      bson_cursor_free (c);
      bson_free (meta);
      g_free (stream);

      errno = EPROTO;
      return NULL;
    }
  stream->file.id = g_malloc (12);
  memcpy (stream->file.id, oid, 12);

  bson_cursor_find (c, "length");
  bson_cursor_get_int64 (c, &stream->file.length);
  if (stream->file.length == 0)
    {
      gint32 i = 0;

      bson_cursor_get_int32 (c, &i);
      stream->file.length = i;
    }

  bson_cursor_find (c, "chunkSize");
  bson_cursor_get_int32 (c, &stream->file.chunk_size);

  bson_cursor_free (c);
  bson_free (meta);

  if (stream->file.length == 0 ||
      stream->file.chunk_size == 0)
    {
      g_free (stream->file.id);
      g_free (stream);

      errno = EPROTO;
      return NULL;
    }

  return stream;
}

mongo_sync_gridfs_stream *
mongo_sync_gridfs_stream_new (mongo_sync_gridfs *gfs,
                              const bson *metadata)
{
  mongo_sync_gridfs_stream *stream;
  bson_cursor *c;

  if (!gfs)
    {
      errno = ENOTCONN;
      return NULL;
    }

  stream = g_new0 (mongo_sync_gridfs_stream, 1);
  stream->file.type = LMC_GRIDFS_FILE_STREAM_WRITER;
  stream->gfs = gfs;

  stream->file.chunk_size = gfs->chunk_size;

  stream->writer.metadata = bson_new_from_data (bson_data (metadata),
                                                bson_size (metadata) - 1);

  c = bson_find (metadata, "_id");
  if (!c)
    {
      stream->file.id = mongo_util_oid_new
        (mongo_connection_get_requestid ((mongo_connection *)gfs->conn));
      if (!stream->file.id)
        {
          bson_free (stream->writer.metadata);
          g_free (stream);

          errno = EFAULT;
          return NULL;
        }
      bson_append_oid (stream->writer.metadata, "_id", stream->file.id);
    }
  else
    {
      const guint8 *oid;

      if (!bson_cursor_get_oid (c, &oid))
        {
          bson_cursor_free (c);
          bson_free (stream->writer.metadata);
          g_free (stream);

          errno = EPROTO;
          return NULL;
        }

      stream->file.id = g_malloc (12);
      memcpy (stream->file.id, oid, 12);
    }
  bson_cursor_free (c);
  bson_finish (stream->writer.metadata);

  stream->writer.buffer = g_malloc (stream->file.chunk_size);
  stream->writer.checksum = g_checksum_new (G_CHECKSUM_MD5);

  return stream;
}

static inline gboolean
_stream_seek_chunk (mongo_sync_gridfs_stream *stream,
                    gint64 chunk)
{
  bson *b;
  mongo_packet *p;
  bson_cursor *c;
  bson_binary_subtype subt = BSON_BINARY_SUBTYPE_USER_DEFINED;
  gboolean r;

  b = bson_new_sized (32);
  bson_append_oid (b, "files_id", stream->file.id);
  bson_append_int64 (b, "n", chunk);
  bson_finish (b);

  p = mongo_sync_cmd_query (stream->gfs->conn,
                            stream->gfs->ns.chunks, 0,
                            0, 1, b, NULL);
  bson_free (b);

  bson_free (stream->reader.bson);
  stream->reader.bson = NULL;
  stream->reader.chunk.data = NULL;

  mongo_wire_reply_packet_get_nth_document (p, 1, &stream->reader.bson);
  mongo_wire_packet_free (p);
  bson_finish (stream->reader.bson);

  c = bson_find (stream->reader.bson, "data");
  r = bson_cursor_get_binary (c, &subt, &stream->reader.chunk.data,
                              &stream->reader.chunk.size);
  if (!r || (subt != BSON_BINARY_SUBTYPE_GENERIC &&
             subt != BSON_BINARY_SUBTYPE_BINARY))
    {
      bson_cursor_free (c);
      bson_free (stream->reader.bson);
      stream->reader.bson = NULL;
      stream->reader.chunk.data = NULL;

      errno = EPROTO;
      return FALSE;
    }
  bson_cursor_free (c);

  if (subt == BSON_BINARY_SUBTYPE_BINARY)
    {
      stream->reader.chunk.start_offset = 4;
      stream->reader.chunk.size -= 4;
    }
  stream->reader.chunk.offset = 0;

  return TRUE;
}

gint64
mongo_sync_gridfs_stream_read (mongo_sync_gridfs_stream *stream,
                               guint8 *buffer,
                               gint64 size)
{
  gint64 pos = 0;

  if (!stream)
    {
      errno = ENOENT;
      return -1;
    }
  if (stream->file.type != LMC_GRIDFS_FILE_STREAM_READER)
    {
      errno = EOPNOTSUPP;
      return -1;
    }
  if (!buffer || size <= 0)
    {
      errno = EINVAL;
      return -1;
    }

  if (!stream->reader.chunk.data)
    {
      if (!_stream_seek_chunk (stream, 0))
        return -1;
    }

  while (pos < size && stream->file.offset +
         stream->reader.chunk.start_offset < stream->file.length)
    {
      gint32 csize = stream->reader.chunk.size - stream->reader.chunk.offset;

      if (size - pos < csize)
        csize = size - pos;

      memcpy (buffer + pos,
              stream->reader.chunk.data +
              stream->reader.chunk.start_offset +
              stream->reader.chunk.offset, csize);

      stream->reader.chunk.offset += csize;
      stream->file.offset += csize;
      pos += csize;

      if (stream->reader.chunk.offset + stream->reader.chunk.start_offset >=
          stream->reader.chunk.size &&
          stream->file.offset + stream->reader.chunk.start_offset <
          stream->file.length)
        {
          stream->file.current_chunk++;
          if (!_stream_seek_chunk (stream, stream->file.current_chunk))
            return -1;
        }
    }

  return pos;
}

static gboolean
_stream_chunk_write (mongo_sync_gridfs *gfs,
                     const guint8 *oid, gint64 n,
                     const guint8 *buffer, gint32 size)
{
  bson *chunk;

  chunk = bson_new_sized (size + 128);
  bson_append_oid (chunk, "files_id", oid);
  bson_append_int64 (chunk, "n", n);
  bson_append_binary (chunk, "data", BSON_BINARY_SUBTYPE_GENERIC,
                      buffer, size);
  bson_finish (chunk);

  if (!mongo_sync_cmd_insert (gfs->conn, gfs->ns.chunks, chunk, NULL))
    {
      int e = errno;

      bson_free (chunk);
      errno = e;
      return FALSE;
    }
  bson_free (chunk);

  return TRUE;
}

gboolean
mongo_sync_gridfs_stream_write (mongo_sync_gridfs_stream *stream,
                                const guint8 *buffer,
                                gint64 size)
{
  gint64 pos = 0;

  if (!stream)
    {
      errno = ENOENT;
      return FALSE;
    }
  if (stream->file.type != LMC_GRIDFS_FILE_STREAM_WRITER)
    {
      errno = EOPNOTSUPP;
      return FALSE;
    }
  if (!buffer || size <= 0)
    {
      errno = EINVAL;
      return FALSE;
    }

  while (pos < size)
    {
      gint32 csize = stream->file.chunk_size - stream->writer.buffer_offset;

      if (size - pos < csize)
        csize = size - pos;

      memcpy (stream->writer.buffer + stream->writer.buffer_offset,
              buffer + pos, csize);
      stream->writer.buffer_offset += csize;
      stream->file.offset += csize;
      stream->file.length += csize;
      pos += csize;

      if (stream->writer.buffer_offset == stream->file.chunk_size)
        {
          if (!_stream_chunk_write (stream->gfs,
                                    stream->file.id,
                                    stream->file.current_chunk,
                                    stream->writer.buffer,
                                    stream->file.chunk_size))
            return FALSE;
          g_checksum_update (stream->writer.checksum, stream->writer.buffer,
                             stream->file.chunk_size);

          stream->writer.buffer_offset = 0;
          stream->file.current_chunk++;
        }
    }

  return TRUE;
}

gboolean
mongo_sync_gridfs_stream_seek (mongo_sync_gridfs_stream *stream,
                               gint64 pos,
                               gint whence)
{
  gint64 real_pos = 0;
  gint64 chunk;
  gint32 offs;

  if (!stream)
    {
      errno = ENOENT;
      return FALSE;
    }
  if (stream->file.type != LMC_GRIDFS_FILE_STREAM_READER)
    {
      errno = EOPNOTSUPP;
      return FALSE;
    }

  switch (whence)
    {
    case SEEK_SET:
      if (pos == stream->file.offset)
        return TRUE;
      if (pos < 0 || pos > stream->file.length)
        {
          errno = ERANGE;
          return FALSE;
        }
      real_pos = pos;
      break;
    case SEEK_CUR:
      if (pos + stream->file.offset < 0 ||
          pos + stream->file.offset > stream->file.length)
        {
          errno = ERANGE;
          return FALSE;
        }
      if (pos == 0)
        return TRUE;
      real_pos = pos + stream->file.offset;
      break;
    case SEEK_END:
      if (pos > 0 || pos + stream->file.length < 0)
        {
          errno = ERANGE;
          return FALSE;
        }
      real_pos = pos + stream->file.length;
      break;
    default:
      errno = EINVAL;
      return FALSE;
    }

  chunk = real_pos / stream->file.chunk_size;
  offs = real_pos % stream->file.chunk_size;

  if (!_stream_seek_chunk (stream, chunk))
    return FALSE;

  stream->reader.chunk.offset = offs;
  stream->file.current_chunk = chunk;
  stream->file.offset = real_pos;

  return TRUE;
}

gboolean
mongo_sync_gridfs_stream_close (mongo_sync_gridfs_stream *stream)
{
  if (!stream)
    {
      errno = ENOENT;
      return FALSE;
    }

  if (stream->file.type != LMC_GRIDFS_FILE_STREAM_READER &&
      stream->file.type != LMC_GRIDFS_FILE_STREAM_WRITER)
    {
      errno = EINVAL;
      return FALSE;
    }

  if (stream->file.type == LMC_GRIDFS_FILE_STREAM_WRITER)
    {
      bson *meta;
      gint64 upload_date;
      GTimeVal tv;
      gboolean closed = FALSE;

      if (stream->writer.buffer_offset > 0)
        {
          closed = _stream_chunk_write (stream->gfs,
                                        stream->file.id,
                                        stream->file.current_chunk,
                                        stream->writer.buffer,
                                        stream->writer.buffer_offset);

          if (closed)
            g_checksum_update (stream->writer.checksum,
                               stream->writer.buffer,
                               stream->writer.buffer_offset);
        }

      if (closed)
        {
          g_get_current_time (&tv);
          upload_date =  (((gint64) tv.tv_sec) * 1000) +
            (gint64)(tv.tv_usec / 1000);

          /* _id is guaranteed by _stream_new() */
          meta = bson_new_from_data (bson_data (stream->writer.metadata),
                                     bson_size (stream->writer.metadata) - 1);
          bson_append_int64 (meta, "length", stream->file.length);
          bson_append_int32 (meta, "chunkSize", stream->file.chunk_size);
          bson_append_utc_datetime (meta, "uploadDate", upload_date);
          if (stream->file.length)
            bson_append_string (meta, "md5",
                                g_checksum_get_string (stream->writer.checksum), -1);
          bson_finish (meta);

          if (!mongo_sync_cmd_insert (stream->gfs->conn,
                                      stream->gfs->ns.files, meta, NULL))
            {
              int e = errno;

              bson_free (meta);
              errno = e;
              return FALSE;
            }
          bson_free (meta);
        }

      bson_free (stream->writer.metadata);
      g_checksum_free (stream->writer.checksum);
      g_free (stream->writer.buffer);
    }
  else
    bson_free (stream->reader.bson);

  g_free (stream->file.id);
  g_free (stream);
  return TRUE;
}
