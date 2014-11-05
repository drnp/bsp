## Process this file with automake to produce Makefile.in
lib_LTLIBRARIES = libbsp-mongo.la

libbsp_mongo_la_SOURCES = \
	bson.c \
	bson.h \
	compat.c \
	compat.h \
	mongo-client.c \
	mongo-client.h \
	mongo.h \
	mongo-sync.c \
	mongo-sync-cursor.c \
	mongo-sync-cursor.h \
	mongo-sync.h \
	mongo-sync-pool.c \
	mongo-sync-pool.h \
	mongo-utils.c \
	mongo-utils.h \
	mongo-wire.c \
	mongo-wire.h \
	sync-gridfs.c \
	sync-gridfs-chunk.c \
	sync-gridfs-chunk.h \
	sync-gridfs.h \
	sync-gridfs-stream.c \
	sync-gridfs-stream.h \
	libmongo-macros.h \
	libmongo-private.h

libbsp_mongo_la_CFLAGS = -DHAVE_MSG_NOSIGNAL
libbsp_mongo_la_LDFLAGS = $(AM_LDFLAGS) -avoid-version