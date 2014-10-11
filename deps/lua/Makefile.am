## Process this file with automake to produce Makefile.in
lib_LTLIBRARIES = libbsp-lua.la

libbsp_lua_la_SOURCES = \
	lapi.c \
	lapi.h \
	lauxlib.c \
	lauxlib.h \
	lbaselib.c \
	lbitlib.c \
	lcode.c \
	lcode.h \
	lcorolib.c \
	lctype.c \
	lctype.h \
	ldblib.c \
	ldebug.c \
	ldebug.h \
	ldo.c \
	ldo.h \
	ldump.c \
	lfunc.c \
	lfunc.h \
	lgc.c \
	lgc.h \
	linit.c \
	liolib.c \
	llex.c \
	llex.h \
	llimits.h \
	lmathlib.c \
	lmem.c \
	lmem.h \
	loadlib.c \
	lobject.c \
	lobject.h \
	lopcodes.c \
	lopcodes.h \
	loslib.c \
	lparser.c \
	lparser.h \
	lstate.c \
	lstate.h \
	lstring.c \
	lstring.h \
	lstrlib.c \
	ltable.c \
	ltable.h \
	ltablib.c \
	ltm.c \
	ltm.h \
	lua.c \
	lua.h \
	luaconf.h \
	lualib.h \
	lundump.c \
	lundump.h \
	lvm.c \
	lvm.h \
	lzio.c \
	lzio.h

libbsp_lua_la_CFLAGS = -DLUA_COMPAT_ALL -DLUA_USE_LINUX
libbsp_lua_la_LDFLAGS = $(AM_LDFLAGS) -avoid-version