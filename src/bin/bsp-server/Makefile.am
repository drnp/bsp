## Process this file with automake to produce Makefile.in
bin_PROGRAMS = \
	bsp-server

bsp_server_SOURCES = \
	bsp-server.h \
	bsp-server.c

bsp_server_LDADD = -L../../lib/bsp-core/.libs -lbsp-core -L../../../deps/mongo/.libs -lbsp-mongo -L../../../deps/lua/.libs -lbsp-lua
