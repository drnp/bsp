dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.
AC_PREREQ([2.59])
AC_INIT(sickycat, 2.1, , bsp, http://bsp.bsgroup.org/)
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/opt/bsp])
AC_PROG_CC
AC_PROG_CXX([gcc c++ g++])
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

test "$prefix" = NONE && prefix=/opt/bsp

AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(malloc_usable_size, malloc)
AC_SEARCH_LIBS(pthread_spin_lock, pthread)
AC_SEARCH_LIBS(log2, m, [], [AC_MSG_ERROR([GNU math library needed])])
AC_SEARCH_LIBS(readline, readline, [], [AC_MSG_ERROR([readline library needed])])
AC_SEARCH_LIBS(MD5_Update, [ssl crypto], [], [AC_MSG_ERROR([Crypto needed])])
AC_SEARCH_LIBS(SHA1_Update, [ssl crypto], [], [AC_MSG_ERROR([Crypto needed])])
AC_SEARCH_LIBS(deflate, z, [], [AC_MSG_ERROR([zlib needed])])
AC_SEARCH_LIBS(gethugepagesizes, hugetlbfs)
AC_SEARCH_LIBS(lt_dlinit, ltdl, [], [AC_MSG_ERROR([libltdl needed])])
AC_SEARCH_LIBS(sqlite3_open, sqlite3, [], [AC_MSG_ERROR([SQLite3 needed])])

AC_CHECK_LIB(stdc++, main, [], [AC_MSG_ERROR([libstdc++ required])])

AC_CHECK_FUNCS([dup2 \
    gettimeofday \
    mlockall \
    getpagesizes \
    memcntl \
    sigignore \
    mmap
])

AC_CHECK_HEADERS([ \
    limits.h \
    malloc.h \
    math.h \
    pthread.h \
    sched.h \
    fcntl.h \
    stdint.h \
    sys/stat.h \
    sys/socket.h \
    sys/syscall.h \
    sys/uio.h \
    sys/mman.h \
    sys/resource.h \
    sys/types.h
], [], [AC_MSG_ERROR([System header missed])])

AC_CHECK_HEADERS([ \
    sys/epoll.h
], [], [AC_MSG_ERROR([This system seemed does not support epoll])])

AC_CHECK_HEADERS([ \
    sys/eventfd.h
], [], [AC_MSG_ERROR([This system seemed does not support eventfd])])

AC_CHECK_HEADERS([ \
    sys/timerfd.h
], [], [AC_MSG_ERROR([This system seemed does not support timerfd])])

AC_CHECK_HEADERS([ \
    openssl/md5.h \
    openssl/sha.h
], [], [AC_MSG_ERROR([OpenSSL header files missed])])

AC_CHECK_HEADERS([ \
    zlib.h
], [], [AC_MSG_ERROR([Zlib header file missed])])

AC_CHECK_HEADERS([ \
    ltdl.h
], [], [AC_MSG_ERROR([Libtool ltdl header file missed])])

AC_CHECK_HEADERS([ \
    sqlite3.h
], [], [AC_MSG_ERROR([SQLite3 header file missed])])

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_SIZE_T
LT_INIT

CPPFLAGS="-I$PWD -I$PWD/deps/lua/ -I$PWD/src/modules -I$PWD/src/lib/bsp-core/"
CFLAGS="-O3 -g -Wall"
LIBS="-L$prefix/lib/ $LIBS"

AC_DEFUN([AC_C_ENDIAN],
[AC_CACHE_CHECK(for endianness, ac_cv_c_endian,
[
    AC_RUN_IFELSE(
        [AC_LANG_PROGRAM([], [
            long val = 1;
            char *c = (char *) &val;
            exit(*c == 1);
        ])
    ],[
        ac_cv_c_endian=big
    ],[
        ac_cv_c_endian=little
    ])
])
if test $ac_cv_c_endian = big; then
    AC_DEFINE(ENDIAN_BIG, 1, [machine is big-endian])
fi
if test $ac_cv_c_endian = little; then
    AC_DEFINE(ENDIAN_LITTLE, 1, [machine is little-endian])
fi
])

# Check mysql (header & lib) with mysql_config
trymysqlconfig="mysql_config"
AC_ARG_WITH([mysql-config], 
    [AS_HELP_STRING([--with-mysql-config=FILE], [Specify location of mysql_config])], 
    [
        if test "x$withval" != "xno"; then
            trymysqlconfig=$withval
        fi
    ]
)

AC_CACHE_CHECK([for mysqlclient installation], ac_cv_mysqlclient_dir, [
    saved_LIBS="$LIBS"
    saved_LDFLAGS="$LDFLAGS"
    saved_CPPFLAGS="$CPPFLAGS"
    lm_found=no
    
    LIBS=`$trymysqlconfig --libs`" $LIBS"
    CPPFLAGS=`$trymysqlconfig --include`" $CPPFLAGS"
    AC_TRY_LINK([#include <stdlib.h>
                #include <mysql.h>], 
                [mysql_init(NULL);], 
                [libmysqlclient_linked=yes], 
                [libmysqlclient_linked=no])
    if test $libmysqlclient_linked = yes; then
        ac_cv_mysqlclient_dir="$trymysqlconfig"
        lm_found=yes
    fi
    
    if test $lm_found = no; then
        LIBS="$saved_LIBS"
        LDFLAGS="$saved_LDFLAGS"
        CPPFLAGS="$saved_CPPFLAGS"
        AC_MSG_ERROR([mysql_config is required!])
    fi
])

# Snappy compression
trysnappy="no"
AC_ARG_ENABLE([snappy], 
    [AS_HELP_STRING([--with-snappy=DIR], [Specify location of Google snappy installation])], 
    [
        if test "x$withval" != "xno"; then
            trysnappy=$withval
        else
            trysnappy="/usr"
        fi
    ]
)
if test "x$trysnappy" != "xno"; then
AC_CACHE_CHECK([for libsnappy installation], ac_cv_snappy_dir, [
    saved_LIBS="$LIBS"
    saved_LDFLAGS="$LDFLAGS"
    saved_CPPFLAGS="$CPPFLAGS"
    ls_found=no

    for lsdir in $trysnappy "" /usr /usr/local; do
        LDFLAGS="$saved_LDFLAGS"
        CPPFLAGS="$saved_CPPFLAGS"
        LIBS="-lsnappy $saved_LIBS"

        if test ! -z "$lsdir" -a ! -d "$lsdir"; then
            continue;
        fi
        if test ! -z "$lsdir"; then
            if test -d "$lsdir/lib"; then
                LDFLAGS="-L$lsdir/lib $LDFLAGS"
            else
                LDFLAGS="-L$lsdir $LDFLAGS"
            fi
            if test -d "$lsdir/include"; then
                CPPFLAGS="-I$lsdir/include $CPPFLAGS"
            else
                CPPFLAGS="-I$lsdir $CPPFLAGS"
            fi
        fi
        # Try compile
        AC_TRY_LINK(
            [#include <snappy-c.h>], 
            [size_t s=snappy_max_compressed_length(1024);], 
            [ls_linked=yes], 
            [ls_linked=no])
        if test $ls_linked = yes; then
            if test ! -z "$lsdir"; then
                ac_cv_snappy_dir=$lsdir
            else
                ac_cv_snappy_dir="(system)"
            fi
            ls_found=yes
            break
        fi
    done

    LIBS="$saved_LIBS"
    LDFLAGS="$saved_LDFLAGS"
    CPPFLAGS="$saved_CPPFLAGS"
    if test $ls_found = no; then
        AC_MSG_ERROR([libsnappy not found])
    fi

    LIBS="-lsnappy $LIBS"
    if test $ac_cv_snappy_dir != "(system)"; then
        if test -d "$ac_cv_snappy_dir/lib"; then
            LDFLAGS="-L$ac_cv_snappy_dir/lib $LDFLAGS"
        else
            LDFLAGS="-L$ac_cv_snappy_dir $LDFLAGS"
        fi
        if test -d "$ac_cv_snappy_dir/include"; then
            CPPFLAGS="-I$ac_cv_snappy_dir/include $CPPFLAGS"
        else
            CPPFLAGS="-I$ac_cv_snappy_dir $CPPFLAGS"
        fi
    fi
])
    AC_DEFINE([ENABLE_SNAPPY], 1, [Enable Google.Snappy])
fi

# LZ4 compression
trylz4="no"
AC_ARG_ENABLE([lz4], 
    [AS_HELP_STRING([--with-lz4=DIR], [Specify location of LZ4 installation])], 
    [
        if test "x$withval" != "xno"; then
            trylz4=$withval
        else
            trylz4="/usr"
        fi
    ]
)
if test "x$trylz4" != "xno"; then
AC_CACHE_CHECK([for liblz4 installation], ac_cv_lz4_dir, [
    saved_LIBS="$LIBS"
    saved_LDFLAGS="$LDFLAGS"
    saved_CPPFLAGS="$CPPFLAGS"
    ll_found=no

    for lldir in $trylz4 "" /usr /usr/local; do
        LDFLAGS="$saved_LDFLAGS"
        CPPFLAGS="$saved_CPPFLAGS"
        LIBS="-llz4 $saved_LIBS"

        if test ! -z "$lldir" -a ! -d "$lldir"; then
            continue;
        fi
        if test ! -z "$lldir"; then
            if test -d "$lldir/lib"; then
                LDFLAGS="-L$lldir/lib $LDFLAGS"
            else
                LDFLAGS="-L$lldir $LDFLAGS"
            fi
            if test -d "$lldir/include"; then
                CPPFLAGS="-I$lldir/include $CPPFLAGS"
            else
                CPPFLAGS="-I$lldir $CPPFLAGS"
            fi
        fi
        # Try compile
        AC_TRY_LINK(
            [#include <lz4.h>], 
            [int s=LZ4_compressBound(1024);], 
            [ll_linked=yes], 
            [ll_linked=no])
        if test $ll_linked = yes; then
            if test ! -z "$lldir"; then
                ac_cv_lz4_dir=$lsdir
            else
                ac_cv_lz4_dir="(system)"
            fi
            ll_found=yes
            break
        fi
    done

    LIBS="$saved_LIBS"
    LDFLAGS="$saved_LDFLAGS"
    CPPFLAGS="$saved_CPPFLAGS"
    if test $ll_found = no; then
        AC_MSG_ERROR([liblz4 not found])
    fi

    LIBS="-llz4 $LIBS"
    if test $ac_cv_lz4_dir != "(system)"; then
        if test -d "$ac_cv_lz4_dir/lib"; then
            LDFLAGS="-L$ac_cv_lz4_dir/lib $LDFLAGS"
        else
            LDFLAGS="-L$ac_cv_lz4_dir $LDFLAGS"
        fi
        if test -d "$ac_cv_lz4_dir/include"; then
            CPPFLAGS="-I$ac_cv_lz4_dir/include $CPPFLAGS"
        else
            CPPFLAGS="-I$ac_cv_lz4_dir $CPPFLAGS"
        fi
    fi
])
    AC_DEFINE([ENABLE_LZ4], 1, [Enable LZ4])
fi

# Jemalloc
tryjemalloc="no"
AC_ARG_WITH([jemalloc], 
    [AS_HELP_STRING([--with-jemalloc=DIR], [Specify location of jemalloc installation])], 
    [
        if test "x$withval" != "xno"; then
            tryjemalloc=$withval
        else
            tryjemalloc="/usr"
        fi
    ]
)
if test "x$tryjemalloc" != "xno"; then
AC_CACHE_CHECK([for libjemalloc installation], ac_cv_jemalloc_dir, [
    saved_LIBS="$LIBS"
    saved_LDFLAGS="$LDFLAGS"
    saved_CPPFLAGS="$CPPFLAGS"
    lj_found=no

    for ljdir in $tryjemalloc "" /usr /usr/local; do
        LDFLAGS="$saved_LDFLAGS"
        CPPFLAGS="$saved_CPPFLAGS"
        LIBS="-ljemalloc $saved_LIBS"

        if test ! -z "$ljdir" -a ! -d "$ljdir"; then
            continue;
        fi
        if test ! -z "$ljdir"; then
            if test -d "$ljdir/lib"; then
                LDFLAGS="-L$ljdir/lib $LDFLAGS"
            else
                LDFLAGS="-L$ljdir $LDFLAGS"
            fi
            if test -d "$ljdir/include"; then
                CPPFLAGS="-I$ljdir/include $CPPFLAGS"
            else
                CPPFLAGS="-I$ljdir $CPPFLAGS"
            fi
        fi
        # Try compile
        AC_TRY_LINK(
            [#include <jemalloc/jemalloc.h>], 
            [malloc(1); ], 
            [lj_linked=yes], 
            [lj_linked=no])
        if test $lj_linked = yes; then
            if test ! -z "$ljdir"; then
                ac_cv_jemalloc_dir=$ljdir
            else
                ac_cv_jemalloc_dir="(system)"
            fi
            lj_found=yes
            break
        fi
    done

    LIBS="$saved_LIBS"
    LDFLAGS="$saved_LDFLAGS"
    CPPFLAGS="$saved_CPPFLAGS"
    if test $lj_found = no; then
        AC_MSG_ERROR([libjemalloc not found])
    fi

    LIBS="-ljemalloc $LIBS"
    if test $ac_cv_jemalloc_dir != "(system)"; then
        if test -d "$ac_cv_jemalloc_dir/lib"; then
            LDFLAGS="-L$ac_cv_jemalloc_dir/lib $LDFLAGS"
        else
            LDFLAGS="-L$ac_cv_jemalloc_dir $LDFLAGS"
        fi
        if test -d "$ac_cv_jemalloc_dir/include"; then
            CPPFLAGS="-I$ac_cv_jemalloc_dir/include $CPPFLAGS"
        else
            CPPFLAGS="-I$ac_cv_jemalloc_dir $CPPFLAGS"
        fi
    fi
])
    AC_DEFINE([ENABLE_JEMALLOC], 1, [Enable JeMalloc])
fi

# Spin lock
trybspspin="no"
AC_ARG_ENABLE([bsp-spin], 
    [AS_HELP_STRING([--enable-bsp-spin], [Use BSP.Spinlock instead of pthread spinlock])], 
    [trybspspin=$enableval]
)
if test "$trybspspin" = "yes"; then
    AC_DEFINE([ENABLE_BSP_SPIN], 1, [Enable BSP.Spinlock])
fi

# Memory allocator
trymempool="no"
AC_ARG_ENABLE([mempool], 
    [AS_HELP_STRING([--enable-mempool], [Use BSP.Mempool as default memory allocator])], 
    [trymempool=$enableval]
)
if test "$trymempool" = "yes"; then
    AC_DEFINE([ENABLE_MEMPOOL], 1, [Enable BSP.Mempool])
fi

# Instance mode
trystandalone="no"
AC_ARG_ENABLE([standalone], 
    [AS_HELP_STRING([--enable-standalone], [Stand-alone mode, no manager needed])], 
    [trystandalone=$enableval]
)
if test "$trystandalone" = "yes"; then
    AC_DEFINE([ENABLE_STANDALONE], 1, [Enable stand-alone mode])
fi

AC_C_ENDIAN
# Write perfix to config.h
AC_DEFINE_UNQUOTED([BSP_PERFIX_DIR], ["$prefix"], [Prefix directory of BSP application])

AC_CHECK_SIZEOF(size_t, 8)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(long long int 8)
AC_CHECK_SIZEOF(long, 8)
AC_CHECK_SIZEOF(int, 4)

AC_OUTPUT([
Makefile
deps/Makefile
deps/lua/Makefile
src/Makefile
src/lib/Makefile
src/lib/bsp-core/Makefile
src/modules/Makefile
src/modules/standard/Makefile
src/modules/http/Makefile
src/modules/mysql/Makefile
src/modules/sqlite/Makefile
src/modules/word_filter/Makefile
src/modules/online/Makefile
src/bin/Makefile
src/bin/bsp-server/Makefile
src/bin/bsp-manager/Makefile
])
