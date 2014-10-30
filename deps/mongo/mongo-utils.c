/* mongo-utils.c - libmongo-client utility functions
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

/** @file src/mongo-utils.c
 * Implementation for various libmongo-client helper functions.
 */

#include <glib.h>
#include <glib/gprintf.h>

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "mongo-client.h"

static guint32 machine_id = 0;
static gint16 pid = 0;

void
mongo_util_oid_init (gint32 mid)
{
  pid_t p = getpid ();

  if (mid == 0)
    {
      srand (time (NULL));
      machine_id = rand ();
    }
  else
    machine_id = mid;

  /*
   * If our pid has more than 16 bits, let half the bits modulate the
   * machine_id.
   */
  if (sizeof (pid_t) > 2)
    {
      machine_id ^= pid >> 16;
    }
  pid = (gint16)p;
}

guint8 *
mongo_util_oid_new_with_time (gint32 ts, gint32 seq)
{
  guint8 *oid;
  gint32 t = GINT32_TO_BE (ts);
  gint32 tmp = GINT32_TO_BE (seq);

  if (machine_id == 0 || pid == 0)
    return NULL;

  oid = (guint8 *)g_new0 (guint8, 12);

  /* Sequence number, last 3 bytes
   * For simplicity's sake, we put this in first, and overwrite the
   * first byte later.
   */
  memcpy (oid + 4 + 2 + 2, &tmp, 4);
  /* First four bytes: the time, BE byte order */
  memcpy (oid, &t, 4);
  /* Machine ID, byte order doesn't matter, 3 bytes */
  memcpy (oid + 4, &machine_id, 3);
  /* PID, byte order doesn't matter, 2 bytes */
  memcpy (oid + 4 + 3, &pid, 2);

  return oid;
}

guint8 *
mongo_util_oid_new (gint32 seq)
{
  return mongo_util_oid_new_with_time (time (NULL), seq);
}

gchar *
mongo_util_oid_as_string (const guint8 *oid)
{
  gchar *str;
  gint j;

  if (!oid)
    return NULL;

  str = g_new (gchar, 26);
  for (j = 0; j < 12; j++)
    g_sprintf (&str[j * 2], "%02x", oid[j]);
  str[25] = 0;
  return str;
}

gboolean
mongo_util_parse_addr (const gchar *addr, gchar **host, gint *port)
{
  gchar *port_s, *ep;
  glong p;

  if (!addr || !host || !port)
    {
      if (host)
        *host = NULL;
      if (port)
        *port = -1;
      errno = EINVAL;
      return FALSE;
    }

  /* Check for IPv6 literal */
  if (addr[0] == '[')
    {
      /* Host is everything between [] */
      port_s = strchr (addr + 1, ']');
      if (!port_s || port_s - addr == 1)
        {
          *host = NULL;
          *port = -1;
          errno = EINVAL;
          return FALSE;
        }
      *host = g_strndup (addr + 1, port_s - addr - 1);

      port_s += 2;
      if (port_s - addr >= (glong)strlen (addr))
        {
          *port = -1;
          return TRUE;
        }
    }
  else
    {
      /* Dealing with something that's not an IPv6 literal */

      /* Split up to host:port */
      port_s = g_strrstr (addr, ":");
      if (!port_s)
        {
          *host = g_strdup (addr);
          *port = -1;
          return TRUE;
        }
      if (port_s == addr)
        {
          *host = NULL;
          *port = -1;
          errno = EINVAL;
          return FALSE;
        }
      port_s++;
      *host = g_strndup (addr, port_s - addr - 1);
    }

  p = strtol (port_s, &ep, 10);
  if (p == LONG_MIN || p == LONG_MAX)
    {
      g_free (*host);
      *host = NULL;
      *port = -1;
      errno = ERANGE;
      return FALSE;
    }
  if ((p != MONGO_CONN_LOCAL) && (p < 0 || p > INT_MAX))
    {
      g_free (*host);
      *host = NULL;
      *port = -1;
      errno = ERANGE;
      return FALSE;
    }
  *port = (gint)p;

  if (ep && *ep)
    {
      g_free (*host);
      *host = NULL;
      *port = -1;
      errno = EINVAL;
      return FALSE;
    }
  return TRUE;
}
