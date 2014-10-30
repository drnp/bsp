/* sync-gridfs.c - libmongo-client GridFS implementation
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

/** @file src/sync-gridfs.c
 * MongoDB GridFS implementation.
 */

#include "sync-gridfs.h"
#include "libmongo-private.h"

#include <errno.h>

mongo_sync_gridfs *
mongo_sync_gridfs_new (mongo_sync_connection *conn,
                       const gchar *ns_prefix)
{
  mongo_sync_gridfs *gfs;
  bson *index;
  gchar *db;

  if (!conn)
    {
      errno = ENOTCONN;
      return NULL;
    }
  if (!ns_prefix)
    {
      errno = EINVAL;
      return NULL;
    }
  db = strchr (ns_prefix, '.');
  if (!db)
    {
      errno = EINVAL;
      return NULL;
    }

  gfs = g_new (mongo_sync_gridfs, 1);
  gfs->conn = conn;

  gfs->ns.prefix = g_strdup (ns_prefix);
  gfs->ns.files = g_strconcat (gfs->ns.prefix, ".files", NULL);
  gfs->ns.chunks = g_strconcat (gfs->ns.prefix, ".chunks", NULL);
  gfs->ns.db = g_strndup (ns_prefix, db - ns_prefix);

  gfs->chunk_size = 256 * 1024;

  index = bson_new_sized (256);
  bson_append_int32 (index, "files_id", 1);
  bson_append_int32 (index, "n", 1);
  bson_finish (index);

  if (!mongo_sync_cmd_index_create (conn, gfs->ns.chunks, index,
                                    MONGO_INDEX_UNIQUE))
    {
      bson_free (index);
      mongo_sync_gridfs_free (gfs, FALSE);

      errno = EPROTO;
      return NULL;
    }
  bson_free (index);

  return gfs;
}

void
mongo_sync_gridfs_free (mongo_sync_gridfs *gfs, gboolean disconnect)
{
  if (!gfs)
    {
      errno = ENOTCONN;
      return;
    }

  g_free (gfs->ns.prefix);
  g_free (gfs->ns.files);
  g_free (gfs->ns.chunks);
  g_free (gfs->ns.db);

  if (disconnect)
    mongo_sync_disconnect (gfs->conn);

  g_free (gfs);
  errno = 0;
}

gint32
mongo_sync_gridfs_get_chunk_size (mongo_sync_gridfs *gfs)
{
  if (!gfs)
    {
      errno = ENOTCONN;
      return -1;
    }
  return gfs->chunk_size;
}

gboolean
mongo_sync_gridfs_set_chunk_size (mongo_sync_gridfs *gfs,
                                  gint32 chunk_size)
{
  if (!gfs)
    {
      errno = ENOTCONN;
      return FALSE;
    }
  if (chunk_size < 1)
    {
      errno = EINVAL;
      return FALSE;
    }

  gfs->chunk_size = chunk_size;
  return TRUE;
}

mongo_sync_cursor *
mongo_sync_gridfs_list (mongo_sync_gridfs *gfs,
                        const bson *query)
{
  mongo_sync_cursor *cursor;
  bson *q = NULL;

  if (!gfs)
    {
      errno = ENOTCONN;
      return NULL;
    }

  if (!query)
    {
      q = bson_new ();
      bson_finish (q);
    }

  cursor = mongo_sync_cursor_new
    (gfs->conn, gfs->ns.files,
     mongo_sync_cmd_query (gfs->conn, gfs->ns.files, 0, 0, 0,
                           (q) ? q : query, NULL));
  if (!cursor)
    {
      int e = errno;

      bson_free (q);
      errno = e;
      return NULL;
    }
  bson_free (q);
  return cursor;
}

const guint8 *
mongo_sync_gridfs_file_get_id (gpointer gfile)
{
  mongo_sync_gridfs_chunked_file *c = (mongo_sync_gridfs_chunked_file *)gfile;
  mongo_sync_gridfs_stream *s = (mongo_sync_gridfs_stream *)gfile;

  if (!gfile)
    {
      errno = ENOTCONN;
      return NULL;
    }
  if (c->meta.type == LMC_GRIDFS_FILE_CHUNKED)
    return c->meta.oid;
  else
    return s->file.id;
}

gint64
mongo_sync_gridfs_file_get_length (gpointer gfile)
{
  mongo_sync_gridfs_file_common *f = (mongo_sync_gridfs_file_common *)gfile;

  if (!gfile)
    {
      errno = ENOTCONN;
      return -1;
    }
  return f->length;
}

gint32
mongo_sync_gridfs_file_get_chunk_size (gpointer gfile)
{
  mongo_sync_gridfs_file_common *f = (mongo_sync_gridfs_file_common *)gfile;

  if (!gfile)
    {
      errno = ENOTCONN;
      return -1;
    }
  return f->chunk_size;
}

const gchar *
mongo_sync_gridfs_file_get_md5 (gpointer gfile)
{
  mongo_sync_gridfs_chunked_file *f = (mongo_sync_gridfs_chunked_file *)gfile;

  if (!gfile)
    {
      errno = ENOTCONN;
      return NULL;
    }
  if (f->meta.type != LMC_GRIDFS_FILE_CHUNKED)
    {
      errno = EOPNOTSUPP;
      return NULL;
    }

  return f->meta.md5;
}

gint64
mongo_sync_gridfs_file_get_date (gpointer gfile)
{
  mongo_sync_gridfs_chunked_file *f = (mongo_sync_gridfs_chunked_file *)gfile;

  if (!gfile)
    {
      errno = ENOTCONN;
      return -1;
    }
  if (f->meta.type != LMC_GRIDFS_FILE_CHUNKED)
    {
      errno = EOPNOTSUPP;
      return -1;
    }

  return f->meta.date;
}

const bson *
mongo_sync_gridfs_file_get_metadata (gpointer gfile)
{
  mongo_sync_gridfs_chunked_file *f = (mongo_sync_gridfs_chunked_file *)gfile;

  if (!gfile)
    {
      errno = ENOTCONN;
      return NULL;
    }
  if (f->meta.type != LMC_GRIDFS_FILE_CHUNKED)
    {
      errno = EOPNOTSUPP;
      return NULL;
    }

  return f->meta.metadata;
}

gint64
mongo_sync_gridfs_file_get_chunks (gpointer gfile)
{
  mongo_sync_gridfs_file_common *f = (mongo_sync_gridfs_file_common *)gfile;
  double chunk_count;

  if (!gfile)
    {
      errno = ENOTCONN;
      return -1;
    }

  chunk_count = (double)f->length / (double)f->chunk_size;
  return (chunk_count - (gint64)chunk_count > 0) ?
    (gint64)(chunk_count + 1) : (gint64)(chunk_count);
}

gboolean
mongo_sync_gridfs_remove (mongo_sync_gridfs *gfs,
                          const bson *query)
{
  mongo_sync_cursor *fc;

  fc = mongo_sync_gridfs_list (gfs, query);
  if (!fc)
    {
      if (errno != ENOTCONN)
        errno = ENOENT;
      return FALSE;
    }

  while (mongo_sync_cursor_next (fc))
    {
      bson *meta = mongo_sync_cursor_get_data (fc), *q;
      bson_cursor *c;
      const guint8 *ooid;
      guint8 oid[12];

      c = bson_find (meta, "_id");
      if (!bson_cursor_get_oid (c, &ooid))
        {
          bson_free (meta);
          bson_cursor_free (c);
          mongo_sync_cursor_free (fc);

          errno = EPROTO;
          return FALSE;
        }
      bson_cursor_free (c);
      memcpy (oid, ooid, 12);
      bson_free (meta);

      /* Delete metadata */
      q = bson_build (BSON_TYPE_OID, "_id", oid,
                      BSON_TYPE_NONE);
      bson_finish (q);

      if (!mongo_sync_cmd_delete (gfs->conn, gfs->ns.files, 0, q))
        {
          bson_free (q);
          mongo_sync_cursor_free (fc);
          return FALSE;
        }
      bson_free (q);

      /* Delete chunks */
      q = bson_build (BSON_TYPE_OID, "files_id", oid,
                      BSON_TYPE_NONE);
      bson_finish (q);

      /* Chunks may or may not exist, an error in this case is
         non-fatal. */
      mongo_sync_cmd_delete (gfs->conn, gfs->ns.chunks, 0, q);
      bson_free (q);
    }

  mongo_sync_cursor_free (fc);

  return TRUE;
}
