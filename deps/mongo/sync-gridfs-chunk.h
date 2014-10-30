/* sync-gridfs-chunk.h - libmong-client GridFS chunk API
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

/** @file src/sync-gridfs-chunk.h
 * MongoDB GridFS Chunk API.
 *
 * @addtogroup mongo_sync_gridfs_api
 * @{
 */

#ifndef LIBMONGO_SYNC_GRIDFS_CHUNK_H
#define LIBMONGO_SYNC_GRIDFS_CHUNK_H 1

#include <sync-gridfs.h>
#include <glib.h>

G_BEGIN_DECLS

/** @defgroup mongo_sync_gridfs_chunk_api Mongo GridFS Chunk API
 *
 * This submodule provides chunk-based access to GridFS
 * files. Chunk-based access has the advantage of being reasonably
 * lightweight and fast, and the disadvantage of making it harder to
 * do arbitrary reads or multi-part writes.
 *
 * It's best used when the whole file needs to be retrieved, or when
 * uploading files that either fit in a buffer, or can be mmapped.
 *
 * @addtogroup mongo_sync_gridfs_chunk_api
 * @{
 */

/** Opaque GridFS chunked file object. */
typedef struct _mongo_sync_gridfs_chunked_file mongo_sync_gridfs_chunked_file;

/** Find a file on GridFS.
 *
 * Finds a file on GridFS, based on a custom query.
 *
 * @param gfs is the GridFS to find the file in.
 * @param query is the custom query based on which the file shall be
 * sought.
 *
 * @returns A newly allocated chunked file object, or NULL on
 * error. It is the responsibility of the caller to free the returned
 * object once it is no longer needed.
 */
mongo_sync_gridfs_chunked_file *mongo_sync_gridfs_chunked_find (mongo_sync_gridfs *gfs,
                                                                const bson *query);

/** Upload a file to GridFS from a buffer.
 *
 * Create a new file on GridFS from a buffer, using custom meta-data.
 *
 * @param gfs is the GridFS to create the file on.
 * @param metadata is the (optional) file metadata.
 * @param data is the data to store on GridFS.
 * @param size is the size of the data.
 *
 * @returns A newly allocated file object, or NULL on error. It is the
 * responsibility of the caller to free the returned object once it is
 * no longer needed.
 *
 * @note The metadata MUST NOT contain any of the required GridFS
 * metadata fields (_id, length, chunkSize, uploadDate, md5),
 * otherwise a conflict will occurr, against which the function does
 * not guard by design.
 */
mongo_sync_gridfs_chunked_file *mongo_sync_gridfs_chunked_file_new_from_buffer (mongo_sync_gridfs *gfs,
                                                                                const bson *metadata,
                                                                                const guint8 *data,
                                                                                gint64 size);
/** Free a GridFS chunked file object.
 *
 * @param gfile is the file object to free.
 */
void mongo_sync_gridfs_chunked_file_free (mongo_sync_gridfs_chunked_file *gfile);

/* Data access */

/** Create a cursor for a GridFS chunked file.
 *
 * The cursor can be used (via
 * mongo_sync_gridfs_file_cursor_get_chunk()) to retrieve a GridFS
 * file chunk by chunk.
 *
 * @param gfile is the GridFS chunked file to work with.
 * @param start is the starting chunk.
 * @param num is the total number of chunks to make a cursor for.
 *
 * @returns A newly allocated cursor object, or NULL on error. It is
 * the responsibility of the caller to free the cursor once it is no
 * longer needed.
 */
mongo_sync_cursor *mongo_sync_gridfs_chunked_file_cursor_new (mongo_sync_gridfs_chunked_file *gfile,
                                                              gint start, gint num);

/** Get the data of a GridFS file chunk, via a cursor.
 *
 * Once we have a cursor, it can be iterated over with
 * mongo_sync_cursor_next(), and its data can be conveniently accessed
 * with this function.
 *
 * @param cursor is the cursor object to work with.
 * @param size is a pointer to a variable where the chunk's actual
 * size can be stored.
 *
 * @returns A pointer to newly allocated memory that holds the current
 * chunk's data, or NULL on error. It is the responsibility of the
 * caller to free this once it is no longer needed.
 */
guint8 *mongo_sync_gridfs_chunked_file_cursor_get_chunk (mongo_sync_cursor *cursor,
                                                         gint32 *size);

/** @} */

G_END_DECLS

/** @} */

#endif
