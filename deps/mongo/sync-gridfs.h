/* sync-gridfs.h - libmong-client GridFS API
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

/** @file src/sync-gridfs.h
 * MongoDB GridFS API.
 *
 * @addtogroup mongo_sync
 * @{
 */

#ifndef LIBMONGO_SYNC_GRIDFS_H
#define LIBMONGO_SYNC_GRIDFS_H 1

#include <mongo-sync.h>
#include <mongo-sync-cursor.h>
#include <glib.h>

G_BEGIN_DECLS

/** @defgroup mongo_sync_gridfs_api Mongo GridFS API
 *
 * The GridFS API - and related modules, like @ref
 * mongo_sync_gridfs_chunk_api and @ref mongo_sync_gridfs_stream_api -
 * provide a conveneint way to work with GridFS, and files stored on
 * it.
 *
 * This module implements the GridFS support functions, which allow
 * one to connect to or create new GridFS instances, list or remove
 * files, or retrieve metadata about files opened by one of the
 * sub-modules.
 *
 * @addtogroup mongo_sync_gridfs_api
 * @{
 */

/** Opaque GridFS object. */
typedef struct _mongo_sync_gridfs mongo_sync_gridfs;

/** Create a new GridFS object.
 *
 * @param conn is the MongoDB connection to base the filesystem object
 * on.
 * @param ns_prefix is the prefix the GridFS collections should be
 * under.
 *
 * @returns A newly allocated GridFS object, or NULL on error.
 */
mongo_sync_gridfs *mongo_sync_gridfs_new (mongo_sync_connection *conn,
                                          const gchar *ns_prefix);

/** Close and free a GridFS object.
 *
 * @param gfs is the GridFS object to free up.
 * @param disconnect signals whether to free the underlying connection
 * aswell.
 */
void mongo_sync_gridfs_free (mongo_sync_gridfs *gfs, gboolean disconnect);

/** Get the default chunk size of a GridFS object.
 *
 * @param gfs is the GridFS object to get the default chunk size of.
 *
 * @returns The chunk size in bytes, or -1 on error.
 */
gint32 mongo_sync_gridfs_get_chunk_size (mongo_sync_gridfs *gfs);

/** Set the default chunk size of a GridFS object.
 *
 * @param gfs is the GridFS object to set the default chunk size of.
 * @param chunk_size is the desired default chunk size.
 *
 * @returns TRUE on success, FALSE otherwise.
 */
gboolean mongo_sync_gridfs_set_chunk_size (mongo_sync_gridfs *gfs,
                                           gint32 chunk_size);

/** List GridFS files matching a query.
 *
 * Finds all files on a GridFS, based on a custom query.
 *
 * @param gfs is the GridFS to list files from.
 * @param query is the custom query based on which files shall be
 * sought. Passing a NULL query will find all files, without
 * restriction.
 *
 * @returns A newly allocated cursor object, or NULL on error. It is
 * the responsibility of the caller to free the returned cursor once
 * it is no longer needed.
 */
mongo_sync_cursor *mongo_sync_gridfs_list (mongo_sync_gridfs *gfs,
                                           const bson *query);

/** Delete files matching a query from GridFS.
 *
 * Finds all files on a GridFS, based on a custom query, and removes
 * them.
 *
 * @param gfs is the GridFS to delete files from.
 * @param query is the custom query based on which files shall be
 * sought. Passing a NULL query will find all files, without
 * restriction.
 *
 * @returns TRUE if all files were deleted successfully, FALSE
 * otherwise.
 */
gboolean mongo_sync_gridfs_remove (mongo_sync_gridfs *gfs,
                                   const bson *query);

/* Metadata */

/** Get the file ID of a GridFS file.
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns The ObjectID of the file, or NULL on error. The returned
 * pointer points to an internal area, and should not be modified or
 * freed, and is only valid as long as the file object is valid.
 */
const guint8 *mongo_sync_gridfs_file_get_id (gpointer gfile);

/** Get the length of a GridFS file.
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns The length of the file, or -1 on error.
 */
gint64 mongo_sync_gridfs_file_get_length (gpointer gfile);

/** Get the chunk size of a GridFS file.
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns The maximum size of the chunks of the file, or -1 on error.
 */
gint32 mongo_sync_gridfs_file_get_chunk_size (gpointer gfile);

/** Get the MD5 digest of a GridFS file.
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns The MD5 digest of the file, or NULL on error. The returned
 * pointer points to an internal area, and should not be modified or
 * freed, and is only valid as long as the file object is valid.
 */
const gchar *mongo_sync_gridfs_file_get_md5 (gpointer gfile);

/** Get the upload date of a GridFS file.
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns The upload date of the file, or -1 on error.
 */
gint64 mongo_sync_gridfs_file_get_date (gpointer gfile);

/** Get the full metadata of a GridFS file
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns A BSON object containing the full metadata, or NULL on
 * error. The returned pointer points to an internal area, and should
 * not be modified or freed, and is only valid as long as the file
 * object is valid.
 */
const bson *mongo_sync_gridfs_file_get_metadata (gpointer gfile);

/** Get the number of chunks in a GridFS file.
 *
 * @param gfile is the GridFS file to work with.
 *
 * @returns The number of chunks in the GridFS file, or -1 on error.
 */
gint64 mongo_sync_gridfs_file_get_chunks (gpointer gfile);

/** @} */

G_END_DECLS

/** @} */

#endif
