dnl -*- mode: m4 -*-
AC_PREREQ(2.63)
AC_INIT([gnome-dictionary], [3.20.0],
	[http://bugzilla.gnome.org/enter_bug.cgi?product=gnome-dictionary],
	[gnome-dictionary],
	[http://live.gnome.org/GnomeUtils])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.14 subdir-objects tar-ustar dist-xz no-dist-gzip foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_HEADER_STDC
PKG_PROG_PKG_CONFIG([0.22])

IT_PROG_INTLTOOL([0.40])

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The name of the gettext domain])
AC_SUBST(GETTEXT_PACKAGE)

AM_GLIB_GNU_GETTEXT

GLIB_MKENUMS=`$PKG_CONFIG --variable glib_mkenums glib-2.0`
GLIB_GENMARSHAL=`$PKG_CONFIG --variable glib_genmarshal glib-2.0`
GLIB_COMPILE_RESOURCES=`$PKG_CONFIG --variable glib_compile_resources gio-2.0`
AC_SUBST(GLIB_MKENUMS)
AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(GLIB_COMPILE_RESOURCES)

# Before making a release, the LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
LIBGDICT_LT_VERSION=10:0:0
AC_SUBST(LIBGDICT_LT_VERSION)

LT_PREREQ([2.2.6])
LT_INIT([disable-static win32-dll])
LT_LIB_M

## don't rerun to this point if we abort
AC_CACHE_SAVE

# For each cycle:
# 	first release: increment major += 1, minor = micro = 0;
# 	each release before API freeze: minor += 1;
# 	each release after API freeze: micro += 1;
# Even if this library is not part of the developers platform, we
# follow the same rules: no ABI breakage (unless unavoidable) and
# no API breakage past the API freeze.
m4_define([gdict_major_version], [0])
m4_define([gdict_minor_version], [12])
m4_define([gdict_micro_version], [0])
m4_define([gdict_version], [gdict_major_version.gdict_minor_version.gdict_micro_version])

GDICT_MAJOR_VERSION=gdict_major_version
GDICT_MINOR_VERSION=gdict_minor_version
GDICT_MICRO_VERSION=gdict_micro_version
GDICT_VERSION=gdict_version

AC_SUBST(GDICT_MAJOR_VERSION)
AC_SUBST(GDICT_MINOR_VERSION)
AC_SUBST(GDICT_MICRO_VERSION)
AC_SUBST(GDICT_VERSION)

dnl Enable debug messages
m4_define([debug_default], [m4_if(m4_eval(gdict_minor_version % 2), [1], [yes], [minimum])])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                              [Enable debug messages @<:@default=debug_default@:>@])],
              [],
              [enable_debug=debug_default])

AS_CASE([$enable_debug],

        [yes],
        [
          test "$cflags_set" = set || CFLAGS="$CFLAGS -g"
          GDICT_DEBUG_CFLAGS="-DGDICT_ENABLE_DEBUG"
        ],

        [minimum],
        [GDICT_DEBUG_CFLAGS="-DG_DISABLE_CAST_CHECKS"],

        [no],
        [GDICT_DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"],

        [*], [AC_MSG_ERROR([Unknown argument to --enable-debug])]
)

AC_SUBST(GDICT_DEBUG_CFLAGS)

dnl IPv6 support
msg_ipv6=no

AC_MSG_CHECKING([whether to enable IPv6])
AC_ARG_ENABLE([ipv6],
              [AS_HELP_STRING([--enable-ipv6=@<:@yes/no@:>@],
                              [Enables compilation of IPv6 code])],
              [],
              [enable_ipv6=yes])

AS_IF([test "x$enable_ipv6" = "xyes"],
      [
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
            #include <sys/socket.h>
            #include <sys/types.h>
          ]], [[
            struct sockaddr_storage ss;
            socket(AF_INET6, SOCK_STREAM, 0)
          ]])],
          [have_ipv6=yes],
          [have_ipv6=no]
        )
      ]
)

AS_IF([test "x$have_ipv6" = "xyes"],
      [
        have_getaddrinfo=no
        AC_CHECK_FUNC(getaddrinfo, have_getaddrinfo=yes)

        AS_IF([test "x$have_getaddrinfo" != "xyes"],
              [
                for lib in bsd socket inet; do
                  AC_CHECK_LIB($lib,
                               [getaddrinfo],
                               ["LIBS=$LIBS -l$lib"; have_getaddrinfo=yes; break])
                done
              ]
        )

        AS_IF([test "x$have_getaddrinfo" = "xyes"],
              [
              AC_DEFINE([ENABLE_IPV6], [1], [Define whether IPv6 support is enabled])
              msg_ipv6=yes
              ]
        )
      ]
)

GLIB_GSETTINGS

GTK_DOC_CHECK([1.15], [--flavour no-tmpl])

YELP_HELP_INIT

GLIB_MIN_VERSION=2.42.0
GTK_MIN_VERSION=3.21.1

PKG_CHECK_MODULES(GDICT,
                  glib-2.0 >= $GLIB_MIN_VERSION
                  gtk+-3.0 >= $GTK_MIN_VERSION)

# Introspection
GOBJECT_INTROSPECTION_CHECK([1.42.0])

AC_CONFIG_FILES([
        Makefile
        libgdict/Makefile
        libgdict/gdict-version.h
        libgdict/gdict-1.0.pc
        data/Makefile
        docs/Makefile
        docs/reference/Makefile
        docs/reference/gdict/Makefile
        docs/reference/gdict/version.xml
        help/Makefile
        src/Makefile
	po/Makefile.in
])

AC_OUTPUT

dnl ==========================================================================
echo "

        GNOME Dictionary $VERSION
        =========================

        prefix: ${prefix}
        compiler: ${CC}

        IPv6 support: ${msg_ipv6}
"
