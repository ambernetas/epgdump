AC_PREREQ([2.63])
AC_INIT([epgdump], [0.2], [support@ambernet.lt])

AC_CONFIG_SRCDIR([src/epgdump.c])

AM_INIT_AUTOMAKE([foreign check-news silent-rules -Wall -Werror])
AM_CONFIG_HEADER([src/config.h])

AC_PROG_CC
AC_PROG_INSTALL

AC_SEARCH_LIBS([argp_parse], [argp], , [
        AC_MSG_ERROR([Your libc is missing argp_parse funcion. If you \
are not using GNU libc you can use argp-standalone package. Specify argp \
library search path in LDFLAGS variable with -L option. Example: ./configure \
LDFLAGS="-L/usr/local/lib"])
])

AC_CHECK_HEADERS([argp.h], , [
        AC_MSG_ERROR([Unable to find argp.h header. If you are using \
argp-standalone package specify argp include search path in CPPFLAGS \
variable with -I option. Example: ./configure CPPFLAGS="-I/usr/local/include"])
])

AC_SEARCH_LIBS([mxmlNewXML], [mxml], , [
	AC_MSG_ERROR([Unable to find libmxml. Please install libmxml package or \
set LDFLAGS variable to include libraray path. Example: ./configure \
LDFLAGS="-L/usr/src/mxml-2.7"])
], [-lpthread])

AC_CHECK_HEADERS([mxml.h], , [
	AC_MSG_ERROR([Unable to find libmxml header files. Please install \
libmxml development package or set CPPFLAGS variable to include header search \
path. Example: ./configure CPPFLAGS="-I/usr/src/mxml-2.7"])
])


PKG_CHECK_MODULES(LIBEVENT, [libevent >= 2.0], , [ AC_MSG_ERROR([Please install libevent-2.0 or newer])	])

AC_TYPE_UINT32_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

#AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])

AC_OUTPUT

