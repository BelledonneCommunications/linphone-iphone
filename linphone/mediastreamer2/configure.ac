dnl Process this file with autoconf to produce a configure script.
AC_INIT([mediastreamer],[2.2.2])

AC_MSG_NOTICE([$PACKAGE_NAME-$PACKAGE_VERSION		A mediastreaming library for telephony application.])
AC_MSG_NOTICE([licensed under the terms of the General Public License (GPL)])

AC_CANONICAL_SYSTEM

dnl Source packaging numbers
MEDIASTREAMER_MAJOR_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f1)
MEDIASTREAMER_MINOR_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f2)
MEDIASTREAMER_MICRO_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f3)
MEDIASTREAMER_EXTRA_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f4)

LIBMEDIASTREAMER_SO_CURRENT=0 dnl increment this number when you add/change/remove an interface
LIBMEDIASTREAMER_SO_REVISION=1 dnl increment this number when you change source code, without changing interfaces; set to 0 when incrementing CURRENT
LIBMEDIASTREAMER_SO_AGE=0 dnl increment this number when you add an interface, set to 0 if you remove an interface

LIBMEDIASTREAMER_SO_VERSION=$LIBMEDIASTREAMER_SO_CURRENT:$LIBMEDIASTREAMER_SO_REVISION:$LIBMEDIASTREAMER_SO_AGE
MEDIASTREAMER_VERSION=${MEDIASTREAMER_MAJOR_VERSION}.${MEDIASTREAMER_MINOR_VERSION}.${MEDIASTREAMER_MICRO_VERSION}

if test -n "$MEDIASTREAMER_EXTRA_VERSION" ; then
	MEDIASTREAMER_VERSION="${MEDIASTREAMER_VERSION}.${MEDIASTREAMER_EXTRA_VERSION}"
fi

AC_SUBST(LIBMEDIASTREAMER_SO_VERSION)
AC_SUBST(MEDIASTREAMER_VERSION)

PACKAGE=mediastreamer

OS=`uname|sed -e 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/'`
AC_MSG_RESULT([Building Package on ${OS}])

AM_INIT_AUTOMAKE([tar-ustar])
AC_CONFIG_HEADERS(mediastreamer-config.h)
AC_DEFINE_UNQUOTED(MEDIASTREAMER_MAJOR_VERSION,$MEDIASTREAMER_MAJOR_VERSION, [major version])
AC_DEFINE_UNQUOTED(MEDIASTREAMER_MINOR_VERSION,$MEDIASTREAMER_MINOR_VERSION, [minor version])
AC_DEFINE_UNQUOTED(MEDIASTREAMER_MICRO_VERSION,$MEDIASTREAMER_MICRO_VERSION, [micro version])
AC_DEFINE_UNQUOTED(MEDIASTREAMER_VERSION,"$MEDIASTREAMER_VERSION",[MEDIASTREAMER version number])

AC_SUBST([mkdir_p])

AC_MSG_CHECKING([warning make an error on compilation])
AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=no]],
[wall_werror=$enableval],
[wall_werror=no]
)

CFLAGS="-DORTP_INET6 $CFLAGS "

dnl enable ipv6 support
AC_ARG_ENABLE(ipv6,
      [  --enable-ipv6    Turn on ipv6 support],
      [case "${enableval}" in
        yes)  ipv6=true;;
        no)   ipv6=false;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-ipv6) ;;
      esac],[ipv6=true])
if test x$ipv6 = xtrue ; then
	CFLAGS="$CFLAGS -DINET6"
fi


AC_ARG_ENABLE(debug,
			[  --enable-debug=[yes/no]   enables the display of traces showing the execution of the library. [default=yes]],
			[case "${enableval}" in
				yes) debug_enabled=yes;;
				no) debug_enabled=no;;
				*) AC_MSG_ERROR("Bad value for --enable-debug");;
			esac],
			[debug_enabled=no] )


dnl Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)

if test "$GCC" != "yes" ; then
	case $target_os in
		*hpux*)
			dnl we are probably using HPUX cc compiler, so add a +O2 to CFLAGS
			CFLAGS="$CFLAGS +O2 -g "
			;;
	esac
else
	CFLAGS="$CFLAGS -Wall"
fi



if test $debug_enabled = "yes"; then
	CFLAGS="$CFLAGS -g -DDEBUG"
else
	CFLAGS="$CFLAGS -O2 -g "
fi

dnl Checks for header files.
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_WORDS_BIGENDIAN
if test x$ac_cv_c_bigendian = xyes ; then
	CFLAGS="$CFLAGS -D_BIGENDIAN "
fi

if test $GCC = yes && test $wall_werror = yes;  then
	CFLAGS="$CFLAGS -Werror "
fi

macosx_found=no

dnl add thread flags
case $OS in
  darwin*)
     MSPLUGINS_CFLAGS=""
     MSPLUGINS_LIBS="-dynamiclib"
     macosx_found=yes
     ;;
  *)
     MSPLUGINS_CFLAGS="-pthread"
     MSPLUGINS_LIBS="-shared -pthread"
     CFLAGS="$CFLAGS -pthread -D_REENTRANT"
     LIBS="$LIBS -pthread -lpthread"
     ;;
esac

AM_CONDITIONAL(BUILD_MACOSX, test x$macosx_found = xyes)

AC_SUBST(MSPLUGINS_CFLAGS)
AC_SUBST(MSPLUGINS_LIBS)

AC_CHECK_LIB(rt,clock_gettime,[LIBS="$LIBS -lrt"])

dnl	*********************************
dnl	various checks for soundcard apis
dnl	*********************************

found_sound=no

AC_CHECK_HEADERS(soundcard.h sys/soundcard.h machine/soundcard.h sys/audio.h)
if test "${ac_cv_header_sys_soundcard_h}" = "yes" || \
	test "${ac_cv_header_soundcard_h}" = "yes" || \
		test "${ac_cv_header_sys_audio_h}" = "yes" || \
	test "${ac_cv_header_machine_soundcard_h}" = "yes"; then
	found_sound=yes
fi

AM_CONDITIONAL(BUILD_OSS, test x$found_sound = xyes)

dnl conditionnal build of ALSA support
AC_ARG_ENABLE(alsa,
      [  --enable-alsa    Turn on alsa native support compiling],
      [case "${enableval}" in
        yes) alsa=true ;;
        no)  alsa=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-alsa) ;;
      esac],[alsa=true])

if test "$alsa" = "true"; then
	AC_CHECK_HEADERS(alsa/asoundlib.h,
		[ AC_CHECK_LIB(asound,snd_pcm_open,
			[ ALSA_LIBS="-lasound"
			  found_sound=yes
			AC_DEFINE(__ALSA_ENABLED__,1,[defined if alsa support is available])
			alsa_enabled=true 
			])
		]
	)
fi
AC_SUBST(ALSA_LIBS)

AM_CONDITIONAL(BUILD_ALSA, test x$alsa_enabled = xtrue)


AC_ARG_ENABLE(artsc,
      [  --enable-artsc    Turn on artsc (kde) sound input/output (auto) ],
      [case "${enableval}" in
        yes) artsc=true ;;
        no)  artsc=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-artsc) ;;
      esac],[artsc=true])

arts_enabled=false

PKG_PROG_PKG_CONFIG()

if test "$artsc" = "true" ; then

	dnl check for arts (kde sound daemon) support
	PKG_CHECK_MODULES(ARTS, [artsc],[
		dnl New detection
		arts_enabled=true
	],[
		dnl Old detection
		if test x$artsc = xtrue ; then
			AC_CHECK_HEADERS(kde/artsc/artsc.h,
				[ AC_CHECK_LIB(artsc,arts_init, 
					[ ARTS_LIBS="-lartsc"
					arts_enabled=true
					])
				]
			)
		fi
		AC_SUBST(ARTS_LIBS)
	])
fi

if test x$arts_enabled = xtrue; then
	found_sound=yes
	AC_DEFINE(__ARTS_ENABLED__,1,[defined if arts support is available])
fi

AM_CONDITIONAL(BUILD_ARTS, test x$arts_enabled = xtrue)

AC_ARG_ENABLE(portaudio,
      [  --enable-portaudio    Turn on portaudio native support compiling],
      [case "${enableval}" in
        yes) portaudio=true ;;
        no)  portaudio=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-portaudio) ;;
      esac],[portaudio=false])

if test "$portaudio" = "true"; then
	AC_CHECK_HEADERS(portaudio.h,
		[ AC_CHECK_LIB(portaudio,Pa_Initialize,
			[ PORTAUDIO_LIBS="-lportaudio"
			  found_sound=yes
			AC_DEFINE(__PORTAUDIO_ENABLED__,1,[defined if portaudio support is available])
			portaudio_enabled=true 
			])
		]
	)
fi

AC_SUBST(PORTAUDIO_LIBS)
AM_CONDITIONAL(BUILD_PORTAUDIO, test x$portaudio_enabled = xtrue)

AC_ARG_ENABLE(macsnd,
      [  --enable-macsnd    Turn on native macosx sound support],
      [case "${enableval}" in
        yes) macsnd=true ;;
        no)  macsnd=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-macsnd) ;;
      esac],[macsnd=false])

if test "$macsnd" = "true"; then
	AC_DEFINE(__MACSND_ENABLED__,1,[defined if native macosx sound support is available])
	macsnd_enabled=true
	found_sound=yes
fi

AM_CONDITIONAL(BUILD_MACSND, test x$macsnd_enabled = xtrue)

AC_ARG_ENABLE(macaqsnd,
      [  --enable-macaqsnd    Turn on native macosx Audio Queue sound support],
      [case "${enableval}" in
        yes) macaqsnd=true ;;
        no)  macaqsnd=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-macaqsnd) ;;
      esac],[macaqsnd=false])

if test "$macaqsnd" = "true"; then
	AC_DEFINE(__MAC_AQ_ENABLED__,1,[defined if native macosx AQ sound support is available])
	macaqsnd_enabled=true
	found_sound=yes
fi

AM_CONDITIONAL(BUILD_MACAQSND, test x$macaqsnd_enabled = xtrue)

dnl Check for samplerate libraries
dnl Check for jack libraries (sound output plugin)
PKG_CHECK_MODULES(JACK,jack >= 0.15.0, 
[ 
	dnl we' found jack devel files
	PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.0.13,
		[AC_DEFINE(__JACK_ENABLED__,1,[Jack support])
		found_sound=yes 
		jack_found=yes] ,
		[echo "libsamplerate not found, jack support disabled."])
	AC_SUBST(SAMPLERATE_CFLAGS)
	AC_SUBST(SAMPLERATE_LIBS)
], 
[echo "No jack support."] )

if test x$jack_found = xno ; then
	dnl reset flags 
	JACK_CFLAGS=
	JACK_LIBS=
	SAMPLERATE_CFLAGS=
	SAMPLERATE_LIBS=
fi

AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS) 

if test "$found_sound" = "no"; then
	AC_MSG_ERROR([Could not find a support sound driver API])
fi


dnl	*************************************
dnl	check for various codecs libraries
dnl	*************************************

dnl check for installed version of speex
PKG_CHECK_MODULES(SPEEX, speex >= 1.1.12,
	[ AC_DEFINE(HAVE_SPEEX_NOISE,1,[tells whether the noise arg of speex_echo_cancel can be used]) ],
	[try_other_speex=yes]
)
PKG_CHECK_MODULES(SPEEX, speex >= 1.1.6, build_speex=yes)
PKG_CHECK_MODULES(SPEEXDSP, speexdsp >= 1.2beta3,
    [SPEEX_LIBS="$SPEEX_LIBS $SPEEXDSP_LIBS"], [
	AC_MSG_NOTICE([No speexdsp library found.])
]
)
AC_SUBST(SPEEX_CFLAGS)
AC_SUBST(SPEEX_LIBS)
AM_CONDITIONAL(BUILD_SPEEX, test x$build_speex = xyes )

dnl check for gsm
build_gsm=no
AC_ARG_WITH( gsm,
		  [  --with-gsm		Sets the installation prefix of gsm codec library [default=/usr] ],
		  [ gsmdir=${withval}],[ gsmdir=/usr ])

if test x"$gsmdir" != xno ; then
	test x"$gmsdir" = xyes && gsmdir=/usr
	MS_CHECK_DEP([gsm codec],[GSM],[${gsmdir}/include],
		[${gsmdir}/lib],[gsm/gsm.h],[gsm],[gsm_create])
	if test "$GSM_found" = "yes" ; then
		build_gsm=yes
	fi
else
	build_gsm=no
fi
AM_CONDITIONAL(BUILD_GSM, test x$build_gsm = xyes )

build_resample=false
AC_CHECK_LIB(resample,resample_open,[LIBS="$LIBS -lresample"; build_resample=yes])
AM_CONDITIONAL(BUILD_RESAMPLE, test x$build_resample = xyes )

MS_CHECK_VIDEO
AM_CONDITIONAL(BUILD_VIDEO, test "$video" = "true")
AM_CONDITIONAL(BUILD_THEORA, test "$have_theora" = "yes")

dnl	*********************************************
dnl	setup oRTP dependency
dnl	*********************************************
AC_ARG_ENABLE(external-ortp,
      [  --enable-external-ortp    Use external oRTP library],
      [case "${enableval}" in
        yes) external_ortp=true ;;
        no)  external_ortp=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-external-ortp) ;;
      esac],[external_ortp=false])

if test "$external_ortp" = 'false'; then
	if test -e $srcdir/../oRTP/include/ortp/ortp.h ; then 
		echo "building from linphone source tree, using ../oRTP/include/ortp/ortp.h"
		ORTP_CFLAGS="-I\$(top_srcdir)/../oRTP/include"
		ORTP_LIBS="\$(top_builddir)/../oRTP/src/libortp.la"
		if test x$ac_cv_c_bigendian = xyes ; then
			ORTP_CFLAGS="$ORTP_CFLAGS -DORTP_BIGENDIAN"
		fi
	else
		external_ortp=true
	fi
fi
if test "$external_ortp" = 'true'; then
	PKG_CHECK_MODULES(ORTP, ortp >= 0.9.0, ,
		[ AC_MSG_ERROR([Couldn't find ortp library]) ] )
fi
AC_SUBST(ORTP_CFLAGS)
AC_SUBST(ORTP_LIBS)


dnl check dlopen support in headers and libraries, so that we can use mediastreamer plugins
AC_CHECK_HEADERS(dlfcn.h)
have_dlopen=false
AC_CHECK_LIB(dl,dlopen,[LIBS="$LIBS -ldl"; have_dlopen=true])
AC_CHECK_FUNC(dlopen,[have_dlopen=true])
if test "$have_dlopen" = "true" ; then
	AC_DEFINE(HAVE_DLOPEN,1,[Defined if dlopen() is availlable])
fi

dnl check various things
AC_FUNC_ALLOCA

dnl define path of plugins:
PACKAGE_PLUGINS_DIR=`eval echo $prefix/lib/mediastreamer/plugins`
AC_DEFINE_UNQUOTED(PACKAGE_PLUGINS_DIR,  "$PACKAGE_PLUGINS_DIR" ,[path of plugins])
AC_SUBST(PACKAGE_PLUGINS_DIR)

PACKAGE_DATA_DIR=`eval echo $prefix/share`
AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,  "$PACKAGE_DATA_DIR" ,[path of data])
AC_SUBST(PACKAGE_DATA_DIR)

dnl check for video4linux headers
AC_CHECK_HEADERS(linux/videodev.h linux/videodev2.h)


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
SUMMARY="A mediastreaming library."
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
include/Makefile
include/mediastreamer2/Makefile
src/Makefile
tests/Makefile
build/Makefile
build/win32native/Makefile
build/win32-novideo/Makefile
build/wince/Makefile
mediastreamer.pc
mediastreamer2.spec
help/Makefile
help/DoxyFile
help/doxygen.dox
)
