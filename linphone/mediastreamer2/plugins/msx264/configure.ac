dnl Process this file with autoconf to produce a configure script.
AC_INIT([msx264],[1.1.8])

AM_INIT_AUTOMAKE([tar-ustar])

AC_CANONICAL_SYSTEM

AC_MSG_CHECKING([warning make an error on compilation])
AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=yes]],
[wall_werror=$enableval],
[wall_werror=yes]
)


dnl Checks for programs.
AC_PROG_CC

AC_LIBTOOL_WIN32_DLL
AC_DISABLE_STATIC
AC_PROG_LIBTOOL


case $target_os in 
	*mingw*)
		mingw_found=yes
	;;
esac

if test "$mingw_found" = "yes" ; then
	AC_MSG_NOTICE([Hacking libtool to work with mingw...])
	sed -e 's/\*\" \$a_deplib \"\*/\*/' < ./libtool > libtool.tmp
	cp -f ./libtool.tmp ./libtool
	rm -f ./libtool.tmp
fi

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

PKG_CHECK_MODULES(MEDIASTREAMER, mediastreamer >= 2.1.0)

PKG_CHECK_MODULES(X264, x264 >= 0.58.0)

AC_ARG_ENABLE(hacked-x264,
[  --enable-hacked-x264       Turn on compilation over a patched x264 that allows multislicing [default=no]],
[hacked_x264=$enableval],
[hacked_x264=no]
)

dnl test for ffmpeg presence
PKG_CHECK_MODULES(FFMPEG, [libavcodec >= 50.0.0 ],ffmpeg_found=yes , ffmpeg_found=no)
dnl workaround for debian...
PKG_CHECK_MODULES(FFMPEG, [libavcodec >= 0d.50.0.0 ], ffmpeg_found=yes, ffmpeg_found=no)
if test x$ffmpeg_found = xno ; then
	AC_MSG_ERROR([Could not find ffmpeg headers and library. This is mandatory for video support])
fi

dnl check for new/old ffmpeg header file layout
CPPFLAGS_save=$CPPFLAGS
CPPFLAGS=$FFMPEG_CFLAGS
AC_CHECK_HEADERS(libavcodec/avcodec.h)
CPPFLAGS=$CPPFLAGS_save

CFLAGS="$CFLAGS \$(MEDIASTREAMER_CFLAGS) \$(FFMPEG_CFLAGS)"
if test "$hacked_x264" = "yes" ; then 
	AC_MSG_WARN([Trying to compile with multislicing patched version of X264])
	CFLAGS="$CFLAGS -DHACKED_X264"
fi

LIBS="$LIBS \$(X264_LIBS) "
LDFLAGS="$LDFLAGS -rdynamic "

dnl define path of plugins:
PACKAGE_PLUGINS_DIR="\$(libdir)/mediastreamer/plugins"
AC_SUBST(PACKAGE_PLUGINS_DIR)

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
SUMMARY="A H264 codec mediastreamer plugin"
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
src/Makefile
)
