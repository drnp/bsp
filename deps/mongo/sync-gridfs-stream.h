/* sync-gridfs-stream.h - libmong-client GridFS streaming API
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

/** @file src/sync-gridfs-stream.h
 * MongoDB GridFS Streaming API.
 *
 * @addtogroup mongo_sync_gridfs_api
 * @{
 */

#ifndef LIBMONGO_SYNC_GRIDFS_STREAM_H
#define LIBMONGO_SYNC_GRIDFS_STREAM_H 1

#include <sync-gridfs.h>
#include <glib.h>

G_BEGIN_DECLS

/** @defgroup mongo_sync_gridfs_stream_api Mongo GridFS Streaming API
 *
 * Ths submodule provides stream-based access to GridFS files. Stream
 * based access has the advantage of allowing arbitrary reads and
 * multi-part writes, at the cost of slightly higher memory usage and
 * lower performance speed.
 *
 * It's best used when one needs only part of a file (and not
 * neccessarily a full chunk, or the parts cross chunk boundaries), or
 * when uploading a file from a source that cannot be fully stored in
 * a memory buffer, and cannot be mmapped. Such as a network
 * connection.
 *
 * @addtogroup mongo_sync_gridfs_stream_api
 * @{
 */

/** Opaque GridFS file stream object type. */
typedef struct _mongo_sync_gridfs_stream mongo_sync_gridfs_stream;

/** Create a stream reader by finding the file matching a query.
 *
 * @param gfs is the GridFS to search on.
 * @param query is the query based on which the file should be
 * searched.
 *
 * @returns A newly allocated read-only stream object, or NULL on
 * error.
 *
 * @note It is the responsiblity of the caller to free the stream once
 * it is no longer needed.
 */
mongo_sync_gridfs_stream *mongo_sync_gridfs_stream_find (mongo_sync_gridfs *gfs,
                                                         const bson *query);

/** Create a new GridFS stream writer.
 *
 * @param gfs is the GridFS to create a file on.
 * @param metadata is the optional extra file metadata to use.
 *
 * @returns A newly allocated write-only stream object, or NULL on
 * error.
 *
 * @note It is the responsiblity of the caller to free the stream once
 * it is no longer needed.
 */
mongo_sync_gridfs_stream *mongo_sync_gridfs_stream_new (mongo_sync_gridfs *gfs,
                                                        const bson *metadata);

/** Read an arbitrary number of bytes from a GridFS stream.
 *
 * @param stream is the read-only stream to read from.
 * @param buffer is the buffer to store the read data in.
 * @param size is the maximum number of bytes to read.
 *
 * @returns The number of bytes read, or -1 on error.
 *
 * @note The @a buffer parameter must have enough space allocated to
 * hold at most @a size bytes.
 */
gint64 mongo_sync_gridfs_stream_read (mongo_sync_gridfs_stream *stream,
                                      guint8 *buffer,
                                      gint64 size);

/** Write an arbitrary number of bytes to a GridFS stream.
 *
 * @param stream is the write-only stream to write to.
 * @param buffer is the data to write.
 * @param size is the amount of data to write.
 *
 * @returns TRUE on success, FALSE otherwise.
 */
gboolean mongo_sync_gridfs_stream_write (mongo_sync_gridfs_stream *stream,
                                         const guint8 *buffer,
                                         gint64 size);

/** Seek to an arbitrary position in a GridFS stream.
 *
 * @param stream is the read-only stream to seek in.
 * @param pos is the position to seek to.
 * @param whence is used to determine how to seek. Possible values are
 * @b SEEK_SET which means seek to the given position, @b SEEK_CUR
 * meaning seek to the current position plus @a pos and @b SEEK_END
 * which will seek from the end of the file.
 *
 * @returns TRUE on success, FALSE otherwise.
 */
gboolean mongo_sync_gridfs_stream_seek (mongo_sync_gridfs_stream *stream,
                                        gint64 pos,
                                        gint whence);

/** Close a GridFS stream.
 *
 * Closes the GridFS stream, by writing out the buffered data, and the
 * metadata if it's a write stream, and freeing up all resources in
 * all cases.
 *
 * @param stream is the GridFS stream to close and free.
 *
 * @returns TRUE on success, FALSE otherwise.
 */
gboolean mongo_sync_gridfs_stream_close (mongo_sync_gridfs_stream *stream);

/** @} */

G_END_DECLS

/** @} */

#endif
