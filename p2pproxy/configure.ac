dnl Process this file with autoconf to produce a configure script.
AC_INIT([p2pproxy],[0.0.1])

AM_INIT_AUTOMAKE([tar-ustar])

AC_CANONICAL_SYSTEM

AC_MSG_CHECKING([warning make an error on compilation])
AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=no]],
[wall_werror=$enableval],
[wall_werror=no]
)


dnl Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)


CFLAGS="$CFLAGS -Wall"


dnl Checks for header files.
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_WORDS_BIGENDIAN
if test x$ac_cv_c_bigendian = xyes ; then
	CFLAGS="$CFLAGS -D_BIGENDIAN"
fi

if test $GCC = yes && test $wall_werror = yes;  then
	CFLAGS="$CFLAGS -Werror "
fi

AC_ARG_WITH(java,
		[  --with-java	Sets directory of java home,  [default=/usr/lib/jvm/java-1.5.0-sun] ],
		[ javadir=${withval}],[javadir=/usr/lib/jvm/java-1.5.0-sun/])

AC_SUBST(javadir)

AC_PATH_PROG(ANT,ant,false)

if test x$ANT = xfalse ; then
	AC_MSG_ERROR([ant not found: you must install ant to build this package.])
fi

if test x$prefix = xNONE ; then
	jarsdir="/usr/local/share/java/fonis"
else
	jarsdir="$prefix/share/java/fonis"
fi

AC_DEFINE_UNQUOTED(P2PPROXY_INSTALLDIR,"$jarsdir",[path of jars])

dnl ##################################################
dnl # Check for ESP Packager
dnl ##################################################

AC_PATH_PROG(EPM,epm,false)
AC_PATH_PROG(MKEPMLIST,mkepmlist,false)
AC_PATH_PROG(EPMINSTALL,epminstall,false)
AM_CONDITIONAL(WITH_EPM,test $EPM != false && test $MKEPMLIST != false && test $EPMINSTALL != false)


# Preferred packaging system, as per EPM terminology
case $target in
*-*-linux*)
	if test -f /etc/debian_version ; then
		EPM_PKG_EXT=deb
	else
     		EPM_PKG_EXT=rpm
     	fi
     	;;
*-hp-hpux*)
     	EPM_PKG_EXT=depot.gz;;
*-dec-osf*)
	EPM_PKG_EXT=setld;;
esac
AC_SUBST(EPM_PKG_EXT)

# System software User & Group names
case $target in
*-*-linux*)
	SYS_USER=root
	SYS_GROUP=root
	;;
*-*-hpux*|*-dec-osf*)
	SYS_USER=bin
	SYS_GROUP=bin
	;;
esac
AC_SUBST(SYS_USER)
AC_SUBST(SYS_GROUP)

# CPU Architecture
case $target_cpu in
i?86)		ARCH=i386;;
*)		ARCH=$target_cpu;;
esac
AC_SUBST(ARCH)

# Various other packaging variables, that can be over-ridden ad `make
# package' time
SUMMARY="A SIP P2P proxy and toolkit"
AC_SUBST(SUMMARY)
PACKAGER=anonymous
AC_SUBST(PACKAGER)
LICENSE=GPL
AC_SUBST(LICENSE)
VENDOR=Linphone
AC_SUBST(VENDOR)
RELEASE=1
AC_SUBST(RELEASE)


AC_OUTPUT(
Makefile
launcher/Makefile
launcher/src/Makefile
)
