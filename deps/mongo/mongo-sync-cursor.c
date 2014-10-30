/* mongo-sync-cursor.c - libmongo-client cursor API on top of Sync
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

/** @file src/mongo-sync-cursor.c
 * MongoDB Cursor API implementation.
 */

#include "config.h"
#include "mongo.h"
#include "libmongo-private.h"

#include <errno.h>

mongo_sync_cursor *
mongo_sync_cursor_new (mongo_sync_connection *conn, const gchar *ns,
                       mongo_packet *packet)
{
  mongo_sync_cursor *c;

  if (!conn)
    {
      errno = ENOTCONN;
      return NULL;
    }
  if (!ns || !packet)
    {
      errno = EINVAL;
      return NULL;
    }

  c = g_new0 (mongo_sync_cursor, 1);
  c->conn = conn;
  c->ns = g_strdup (ns);
  c->results = packet;
  c->offset = -1;

  mongo_wire_reply_packet_get_header (c->results, &c->ph);

  return c;
}

gboolean
mongo_sync_cursor_next (mongo_sync_cursor *cursor)
{
  if (!cursor)
    {
      errno = EINVAL;
      return FALSE;
    }
  errno = 0;

  if (cursor->offset >= cursor->ph.returned - 1)
    {
      gint32 ret = cursor->ph.returned;
      gint64 cid = cursor->ph.cursor_id;

      mongo_wire_packet_free (cursor->results);
      cursor->offset = -1;
      cursor->results = mongo_sync_cmd_get_more (cursor->conn, cursor->ns,
                                                 ret, cid);
      if (!cursor->results)
        return FALSE;
      mongo_wire_reply_packet_get_header (cursor->results, &cursor->ph);
    }
  cursor->offset++;
  return TRUE;
}

void
mongo_sync_cursor_free (mongo_sync_cursor *cursor)
{
  if (!cursor)
    {
      errno = ENOTCONN;
      return;
    }
  errno = 0;

  mongo_sync_cmd_kill_cursors (cursor->conn, 1, cursor->ph.cursor_id);
  g_free (cursor->ns);
  mongo_wire_packet_free (cursor->results);
  g_free (cursor);
}

bson *
mongo_sync_cursor_get_data (mongo_sync_cursor *cursor)
{
  bson *r;

  if (!cursor)
    {
      errno = EINVAL;
      return NULL;
    }

  if (!mongo_wire_reply_packet_get_nth_document (cursor->results,
                                                 cursor->offset + 1,
                                                 &r))
    {
      errno = ERANGE;
      return NULL;
    }
  bson_finish (r);
  return r;
}
