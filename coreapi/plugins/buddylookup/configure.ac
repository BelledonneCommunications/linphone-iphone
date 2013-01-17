#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(buddylookup, 0.0.1, simon dot morlat at linphone dot org)
AC_CONFIG_SRCDIR([src/lookup.c])

AM_INIT_AUTOMAKE([tar-ustar])

AC_CANONICAL_SYSTEM

case $target_os in
	*mingw*)
		mingw_found=yes
	;;
esac

AC_MSG_CHECKING([warning make an error on compilation])
AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=yes]],
[wall_werror=$enableval],
[wall_werror=yes]
)

dnl AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
dnl LT_INIT([disable-static])
AC_DISABLE_STATIC
AC_PROG_LIBTOOL



AC_CONFIG_COMMANDS([libtool-hacking],[
	if test "$mingw_found" = "yes" ; then
	        echo "Hacking libtool to work with mingw..."
		sed -e 's/\*\" \$a_deplib \"\*/\*/' < ./libtool > libtool.tmp
		cp -f ./libtool.tmp ./libtool
		rm -f ./libtool.tmp
	fi
],[mingw_found=$mingw_found])

# Checks for libraries.

PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4 >= 2.4.0)
PKG_CHECK_MODULES(LIBLINPHONE, linphone >= 3.0.0)

LINPHONE_PLUGINS_DIR="\$(libdir)/liblinphone/plugins/"
AC_SUBST(LINPHONE_PLUGINS_DIR)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

if test "$GCC" = "yes" ; then
CFLAGS="$CFLAGS -Wall"
LDFLAGS="$LDFLAGS -rdynamic "

	if test $wall_werror = yes;  then
		CFLAGS="$CFLAGS -Werror "
	fi
fi

AM_CONDITIONAL(BUILD_WIN32, test x$mingw_found = xyes)

AC_OUTPUT(
Makefile
src/Makefile
)
