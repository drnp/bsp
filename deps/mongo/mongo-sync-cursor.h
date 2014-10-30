/* mongo-sync-cursor.h - libmongo-client cursor API on top of Sync
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

/** @file src/mongo-sync-cursor.h
 * MongoDB cursor API public header.
 *
 * @addtogroup mongo_sync
 * @{
 */

#ifndef LIBMONGO_SYNC_CURSOR_H
#define LIBMONGO_SYNC_CURSOR_H 1

#include <glib.h>
#include <mongo-sync.h>

G_BEGIN_DECLS

/** @defgroup mongo_sync_cursor Mongo Sync Cursor API
 *
 * @addtogroup mongo_sync_cursor
 * @{
 */

/** Opaque Mongo Cursor object. */
typedef struct _mongo_sync_cursor mongo_sync_cursor;

/** Create a new MongoDB Cursor.
 *
 * This function can be used to create a new cursor, with which one
 * can conveniently iterate over using mongo_sync_cursor_next().
 *
 * The @a packet argument is supposed to be the output of - for
 * example - mongo_sync_cmd_query().
 *
 * @param conn is the connection to associate with the cursor.
 * @param ns is the namespace to use with the cursor.
 * @param packet is a reply packet on which the cursor should be
 * based. The packet should not be freed or touched by the application
 * afterwards, it will be handled by the cursor functions.
 *
 * @returns A newly allocated cursor, or NULL on error.
 */
mongo_sync_cursor *mongo_sync_cursor_new (mongo_sync_connection *conn,
                                          const gchar *ns,
                                          mongo_packet *packet);

/** Iterate a MongoDB cursor.
 *
 * Iterating the cursor will move its position to the next document in
 * the result set, querying the database if so need be.
 *
 * Queries will be done in bulks, provided that the original query was
 * done so aswell.
 *
 * @param cursor is the cursor to advance.
 *
 * @returns TRUE if the cursor could be advanced, FALSE otherwise. If
 * the cursor could not be advanced due to an error, then errno will
 * be set appropriately.
 */
gboolean mongo_sync_cursor_next (mongo_sync_cursor *cursor);

/** Retrieve the BSON document at the cursor's position.
 *
 * @param cursor is the cursor to retrieve data from.
 *
 * @returns A newly allocated BSON object, or NULL on failure. It is
 * the responsiblity of the caller to free the BSON object once it is
 * no longer needed.
 */
bson *mongo_sync_cursor_get_data (mongo_sync_cursor *cursor);

/** Free a MongoDB cursor.
 *
 * Freeing a MongoDB cursor involves destroying the active cursor the
 * database is holding, and then freeing up the resources allocated
 * for it.
 *
 * @param cursor is the cursor to destroy.
 */
void mongo_sync_cursor_free (mongo_sync_cursor *cursor);

/** @} */

/** @} */

G_END_DECLS

#endif
