dnl Process this file with autoconf to produce a configure script.

AC_INIT([linphone],[3.3.99.10],[linphone-developers@nongnu.org])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([coreapi/linphonecore.c])

dnl Source packaging numbers

LINPHONE_MAJOR_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f1)
LINPHONE_MINOR_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f2)
LINPHONE_MICRO_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f3)
LINPHONE_EXTRA_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f4)

dnl program extension
LINPHONE_VERSION=$LINPHONE_MAJOR_VERSION.$LINPHONE_MINOR_VERSION.${LINPHONE_MICRO_VERSION}
if test "$LINPHONE_EXTRA_VERSION" != "" ;then
	LINPHONE_VERSION=$LINPHONE_VERSION.${LINPHONE_EXTRA_VERSION}
fi

LIBLINPHONE_SO_CURRENT=4 dnl increment this number when you add/change/remove an interface
LIBLINPHONE_SO_REVISION=0 dnl increment this number when you change source code, without changing interfaces; set to 0 when incrementing CURRENT
LIBLINPHONE_SO_AGE=0 dnl increment this number when you add an interface, set to 0 if you remove an interface

LIBLINPHONE_SO_VERSION=$LIBLINPHONE_SO_CURRENT:$LIBLINPHONE_SO_REVISION:$LIBLINPHONE_SO_AGE

AC_SUBST(LIBLINPHONE_SO_VERSION, $LIBLINPHONE_SO_VERSION)
AC_SUBST(LINPHONE_VERSION)

AC_MSG_NOTICE([$PACKAGE_NAME-$PACKAGE_VERSION		A full featured audio/video sip phone.])
AC_MSG_NOTICE([licensed under the terms of the General Public License (GPL)])

AM_INIT_AUTOMAKE
AC_SUBST([LIBTOOL_DEPS])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],)
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([mkdir_p])
AC_ISC_POSIX
AC_PROG_CC
AC_C_INLINE
AM_PROG_CC_STDC
AC_HEADER_STDC
AM_PROG_CC_C_O

case $target_os in
	*mingw32ce)
       	CFLAGS="$CFLAGS -D_WIN32_WCE -DORTP_STATIC -D_WIN32_WINNT=0x0501"
       	CXXFLAGS="$CXXFLAGS -DORTP_STATIC -D_WIN32_WINNT=0x0501"
       	LIBS="$LIBS -lws2 -liphlpapi"
		mingw_found=yes
		mingwce_found=yes
		;;
	*mingw*)
		CFLAGS="$CFLAGS -DORTP_STATIC -D_WIN32_WINNT=0x0501 "
		CXXFLAGS="$CXXFLAGS -DORTP_STATIC -D_WIN32_WINNT=0x0501"
		LIBS="$LIBS -lws2_32"
		GUI_FLAGS="-mwindows"
		CONSOLE_FLAGS="-mconsole"
		mingw_found=yes
	;;
	*darwin*)
		dnl use macport installation
		ACLOCAL_MACOS_FLAGS="-I /opt/local/share/aclocal"
	;;
esac

AC_SUBST(ACLOCAL_MACOS_FLAGS)
AC_SUBST(CONSOLE_FLAGS)
AC_SUBST(GUI_FLAGS)

dnl localization tools
IT_PROG_INTLTOOL([0.40], [no-xml])

dnl Initialize libtool
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)


AC_CONFIG_COMMANDS([libtool-hacking],[
if test "$mingw_found" = "yes" ; then
	echo "Hacking libtool to work with mingw..."
	sed -e 's/\*\" \$a_deplib \"\*/\*/' < ./libtool > libtool.tmp
	cp -f ./libtool.tmp ./libtool
	rm -f ./libtool.tmp
fi
],[mingw_found=$mingw_found])

dnl Add the languages which your application supports here.
PKG_PROG_PKG_CONFIG
ALL_LINGUAS="fr it de ja es pl cs nl sv pt_BR hu ru zh_CN"
AC_SUBST(ALL_LINGUAS)
AC_DEFINE_UNQUOTED(LINPHONE_ALL_LANGS, "$ALL_LINGUAS", [All supported languages])

if test "$mingw_found" != "yes" ; then
dnl gettext macro does not work properly under mingw.
AM_GNU_GETTEXT([external])
LIBS="$LIBS $LIBINTL"
else
	if test "$mingwce_found" != "yes" ; then 
		AC_DEFINE(ENABLE_NLS,1,[Tells whether localisation is possible])
		AC_DEFINE(HAVE_GETTEXT,1,[Tells wheter localisation is possible])
		LIBS="$LIBS -L/usr/lib -lintl"
	else
		dnl gettext macro does not work properly under mingw.
		AM_GNU_GETTEXT([external])
		LIBS="$LIBS $LIBINTL"
	fi
fi

GETTEXT_PACKAGE=linphone
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[The name of the gettext package name])
AC_CHECK_LIB(intl,libintl_gettext)

AC_CHECK_FUNCS([get_current_dir_name strndup stpcpy] )

dnl conditionnal build of console interface.
AC_ARG_ENABLE(console_ui,
      [  --enable-console_ui=[yes/no]    Turn on or off compilation of console interface [default=yes]],
      [case "${enableval}" in
        yes) console_ui=true ;;
        no)  console_ui=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-console_ui) ;;
      esac],[console_ui=true])

dnl conditionnal build of gtk interface.
AC_ARG_ENABLE(gtk_ui,
      [  --enable-gtk_ui=[yes/no]    Turn on or off compilation of gtk interface [default=yes]],
      [case "${enableval}" in
        yes) gtk_ui=true ;;
        no)  gtk_ui=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-gtk_ui) ;;
      esac],[gtk_ui=true])


if test "$gtk_ui" = "true" ; then
	PKG_CHECK_MODULES(LIBGTK, gtk+-2.0 >= 2.4.0 gthread-2.0)
	AC_SUBST(LIBGTK_CFLAGS)
	AC_SUBST(LIBGTK_LIBS)
else
	echo "GTK interface compilation is disabled."
fi


dnl os-specific problems not handled by existing macros.
case "$host_os" in
	*freebsd*)
		LDFLAGS="$LDFLAGS -pthread"
		;;
esac

case "$host_cpu" in
	*arm*)
		AC_DEFINE(__ARM__,1,[Defined if we are compiling for arm processor])
		use_arm_toolchain=yes
		;;
esac

AC_ARG_WITH( configdir,
      [  --with-configdir      Set a APPDATA subdir where linphone is supposed to find its config (windows only) ],
      [ configdir=${withval}],[ configdir="Linphone" ])

AC_DEFINE_UNQUOTED(LINPHONE_CONFIG_DIR,"$configdir",[Windows appdata subdir where linphonerc can be found])

AC_ARG_ENABLE(manual,
      [  --disable-manual    Do not attempt to build html linphone's user documentation],
      [case "${enableval}" in
        yes) build_manual=yes ;;
        no)  build_manual=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-manual) ;;
      esac],[build_manual=yes])

AC_ARG_ENABLE(date,
      [  --enable-date    Use build date in internal version number],
      [case "${enableval}" in
        yes) use_date=yes ;;
        no)  use_date=no ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-date) ;;
      esac],[use_date=no])

if test x$use_date =  xyes ; then
	AC_DEFINE(USE_BUILDDATE_VERSION,1,[Tell whether date_version.h must be used])
fi


dnl enable ipv6 support
AC_ARG_ENABLE(ipv6,
      [  --enable-ipv6    Turn on ipv6 support],
      [case "${enableval}" in
        yes)  ipv6=true;;
        no)   ipv6=false;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-ipv6) ;;
      esac],[ipv6=true])
IPV6_CFLAGS=
if test x$ipv6 = xtrue ; then
	IPV6_CFLAGS=-DINET6
fi
AC_SUBST(IPV6_CFLAGS)

dnl enable truespeech codec support
AC_ARG_ENABLE(truespeech,
      [  --enable-truespeech    Turn on TrueSpeech support (x86 only)],
      [case "${enableval}" in
        yes)  truespeech=true;;
        no)   truespeech=false;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-truespeech) ;;
      esac],[truespeech=false])
TRUESPEECH_CFLAGS=
if test x$truespeech = xtrue ; then
	TRUESPEECH_CFLAGS=-DTRUESPEECH
fi
AC_SUBST(TRUESPEECH_CFLAGS)
AM_CONDITIONAL([BUILD_TRUESPEECH], [test x$truespeech = xtrue])

AC_ARG_ENABLE(nonstandard-gsm,
      [  --enable-nonstandard-gsm    Enable GSM codec at nonstandard rates (11025hz, 16000hz)],
      [case "${enableval}" in
        yes) exotic_gsm=yes
	AC_DEFINE(ENABLE_NONSTANDARD_GSM,1,[Defined when using gsm at nonstandard rates])
	 ;;
        no)  exotic_gsm=no ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-nonstandard-gsm) ;;
      esac],[exotic_gsm=no])


dnl support for RSVP (by Vincent Maury)
AC_ARG_ENABLE(rsvp,
[  --enable-rsvp           enable support for QoS reservations.],
AC_DEFINE(VINCENT_MAURY_RSVP,1,[Tell whether RSVP support
should be compiled.]) )

if test "x${prefix}" = "xNONE"; then
	package_prefix=${ac_default_prefix}
else
	package_prefix=${prefix}
fi

if test x$mingw_found = xyes ; then
dnl allow binaries to install everywhere
package_prefix="."
fi

dnl Set PACKAGE_LOCALE_DIR in config.h.
DATADIRNAME=share
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${package_prefix}/${DATADIRNAME}/locale",[Defines the place where locales can be found])

AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${package_prefix}/${DATADIRNAME}",[Defines the place where data are found])

dnl Set PACKAGE_SOUND_DIR in config.h.
AC_DEFINE_UNQUOTED(PACKAGE_SOUND_DIR, "${package_prefix}/${DATADIRNAME}/sounds/linphone",[Defines the place where linphone sounds are found])


dnl check if we have the getifaddrs() sytem call
AC_CHECK_FUNCS(getifaddrs)

dnl check for osip2
LP_CHECK_OSIP2

dnl setup flags for exosip library
LP_SETUP_EXOSIP

if test "$console_ui" = "true" ; then
dnl check gnu readline
LP_CHECK_READLINE
else
echo "Console interface compilation is disabled."
fi

AC_WORDS_BIGENDIAN

dnl normaly this should only by done by mediastreamer2/configure.ac
dnl but to workaround bugs when cross-compiling for arm-linux,
dnl we need to have SPEEX_LIBS defined
dnl Furthermore it is good to repeat here all mediastreamer2 toggles
dnl since top-level configure --help will not print them.

PKG_CHECK_MODULES(SPEEX, speex >= 1.1.6, build_speex=yes)
AC_SUBST(SPEEX_LIBS)

dnl conditionnal build of video support
AC_ARG_ENABLE(video,
		[  --enable-video    Turn on video support compiling],
		[case "${enableval}" in
		yes) video=true ;;
		no)  video=false ;;
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-video) ;;
		esac],[video=true])
		
AC_ARG_WITH( ffmpeg,
		[  --with-ffmpeg		Sets the installation prefix of ffmpeg, needed for video support. [default=/usr] ],
		[ ffmpegdir=${withval}],[ ffmpegdir=/usr ])

AC_ARG_ENABLE(x11,
			  [  --disable-x11    Disable X11 support],
		 	  [case "${enableval}" in
			  yes) enable_x11=true ;;
			  no)  enable_x11=false ;;
			  *) AC_MSG_ERROR(bad value ${enableval} for --disable-x11) ;;
		  	  esac],[enable_x11=true])

if test "$video" = "true"; then
	
	if test "$enable_x11" = "true"; then
	   AC_CHECK_HEADERS(X11/Xlib.h)
	fi
	AC_DEFINE(VIDEO_ENABLED,1,[defined if video support is available])
fi

AC_ARG_ENABLE(alsa,
      [  --enable-alsa    Turn on alsa native support compiling],
      [case "${enableval}" in
        yes) alsa=true ;;
        no)  alsa=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-alsa) ;;
      esac],[alsa=true])

AC_ARG_ENABLE(artsc,
      [  --enable-artsc    Turn on artsc (kde) sound input/output (auto) ],
      [case "${enableval}" in
        yes) artsc=true ;;
        no)  artsc=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-artsc) ;;
      esac],[artsc=false])

AC_ARG_ENABLE(portaudio,
      [  --enable-portaudio    Turn on portaudio native support compiling],
      [case "${enableval}" in
        yes) portaudio=true ;;
        no)  portaudio=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-portaudio) ;;
      esac],[portaudio=false])

dnl build console if required
AM_CONDITIONAL(BUILD_CONSOLE, test x$console_ui = xtrue)
dnl special things for arm-linux cross compilation toolchain
AM_CONDITIONAL(ARMBUILD, test x$use_arm_toolchain = xyes)
dnl compilation of gtk user interface
AM_CONDITIONAL(BUILD_GTK_UI, [test x$gtk_ui = xtrue ] )
AM_CONDITIONAL(BUILD_WIN32, test x$mingw_found = xyes )

dnl check getenv
AH_TEMPLATE([HAVE_GETENV])
AC_CHECK_FUNC([getenv], AC_DEFINE([HAVE_GETENV], [1], [If present, the getenv function allows fim to read environment variables.]))

dnl
AC_MSG_CHECKING([for sighandler_t])
AC_TRY_COMPILE([#include <signal.h>],[sighandler_t *f;],
has_sighandler_t=yes,has_sighandler_t=no)
AC_MSG_RESULT($has_sighandler_t)
if test "$has_sighandler_t" = "yes" ; then
    AC_DEFINE( HAVE_SIGHANDLER_T, 1, [Define if sighandler_t available] )
fi

##################################################
# Stricter build options (after external packages)
##################################################


AC_ARG_ENABLE(strict,
	AC_HELP_STRING([--enable-strict],
		       [Build with stricter options (gcc only) @<:@yes@:>@]),[
	strictness="${enableval}"],[strictness=yes]
)

if test "$GCC$strictness" = "yesyes" ; then
	STRICT_OPTIONS="-Wall "
	STRICT_OPTIONS="$STRICT_OPTIONS -Werror"
	CFLAGS="$CFLAGS -fno-strict-aliasing"
fi

AC_SUBST(STRICT_OPTIONS)

top_srcdir=`dirname $0`

AC_ARG_ENABLE([external-mediastreamer],
      [AS_HELP_STRING([--enable-external-mediastreamer],[Use external mediastreamer library])],,
      [enable_external_mediastreamer=no])

AS_CASE($enable_external_mediastreamer,
		[yes],[
			PKG_CHECK_MODULES([MEDIASTREAMER], [mediastreamer])
			MS2_VERSION=`$PKG_CONFIG --modversion mediastreamer`
			AM_CONDITIONAL(EXTERNAL_MEDIASTREAMER, [true])],
		[no],[
			AC_CONFIG_SUBDIRS( mediastreamer2 )
			MEDIASTREAMER_DIR=${top_srcdir}/mediastreamer2
			MEDIASTREAMER_CFLAGS="-I\$(top_srcdir)/mediastreamer2/include"
			MEDIASTREAMER_LIBS="\$(top_builddir)/mediastreamer2/src/libmediastreamer.la"
dnl need to temporary change quotes to allow square brackets
		 	changequote(<<, >>)
			MS2_VERSION=`grep -e '^.C_INIT(' $MEDIASTREAMER_DIR/configure.ac | sed -e 's:\([^(]\+\)(\[mediastreamer\],\[\(.*\)\]):\2:g'`
			changequote([, ])
			AM_CONDITIONAL(EXTERNAL_MEDIASTREAMER, [false])],
        [AC_MSG_ERROR([bad value '${enable_external_mediastreamer}' for --enable-external-mediastreamer])])

AC_SUBST(MEDIASTREAMER_CFLAGS)
AC_SUBST(MEDIASTREAMER_LIBS)
AC_SUBST([MS2_VERSION])

dnl check for db2html (docbook) to generate html user manual
AC_CHECK_PROG(have_sgmltools,sgmltools, yes, no)
AM_CONDITIONAL(ENABLE_MANUAL, test x$have_sgmltools$build_manual = xyesyes )

dnl for external use of linphone libs
LINPHONE_CFLAGS="-I${includedir} -I${includedir}/linphone"
LINPHONE_LIBS="-L${libdir} -llinphone"

if test x$mingw_found = xyes ; then
	LINPHONE_LIBS="$LINPHONE_LIBS $OSIP_LIBS"
fi
AC_SUBST(LINPHONE_CFLAGS)
AC_SUBST(LINPHONE_LIBS)

AC_DEFINE_UNQUOTED(LINPHONE_VERSION,"$PACKAGE_VERSION",[Linphone's version number])

AC_DEFINE_UNQUOTED(LINPHONE_PLUGINS_DIR, "${package_prefix}/lib/liblinphone/plugins" ,[path of liblinphone plugins, not mediastreamer2 plugins])
LINPHONE_PLUGINS_DIR="${package_prefix}/lib/liblinphone/plugins"
AC_SUBST(LINPHONE_PLUGINS_DIR)

AC_ARG_ENABLE(external-ortp,
      [  --enable-external-ortp    Use external oRTP library],
      [case "${enableval}" in
        yes) external_ortp=true ;;
        no)  external_ortp=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-external-ortp) ;;
      esac],[external_ortp=false])

if test "$external_ortp" = 'true'; then
	PKG_CHECK_MODULES([ORTP], [ortp])
	ORTP_VERSION=`$PKG_CONFIG --modversion ortp`
else
	AC_CONFIG_SUBDIRS( oRTP )
	ORTP_CFLAGS="-I\$(top_srcdir)/oRTP/include"
	ORTP_LIBS="\$(top_builddir)/oRTP/src/libortp.la"
	if test x$ac_cv_c_bigendian = xyes ; then
		ORTP_CFLAGS="$ORTP_CFLAGS -DORTP_BIGENDIAN"
	fi
	changequote(<<, >>)
	ORTP_VERSION=`grep -E ^[AC]+_INIT ${top_srcdir}/oRTP/configure.ac | sed -e 's:^.*_INIT(.*,\[\(.*\)\]):\1:g'`
	changequote([, ])
fi
AC_SUBST(ORTP_CFLAGS)
AC_SUBST(ORTP_LIBS)
AC_SUBST([ORTP_VERSION])

AM_CONDITIONAL(EXTERNAL_ORTP, [test "$external_ortp" = 'true'])

dnl ##################################################
dnl # Check for doxygen
dnl ##################################################

AC_PATH_PROG(DOXYGEN,doxygen,false)
AM_CONDITIONAL(HAVE_DOXYGEN, test $DOXYGEN != false)


AC_OUTPUT([ 
Makefile 
m4/Makefile
po/Makefile.in
pixmaps/Makefile
coreapi/Makefile
coreapi/help/Makefile
coreapi/help/Doxyfile
gtk/Makefile
console/Makefile
share/Makefile
share/C/Makefile
share/fr/Makefile
share/it/Makefile
share/ja/Makefile
share/cs/Makefile
share/linphone.pc
share/linphone.desktop
scripts/Makefile
linphone.spec
linphone.iss
])

echo "Linphone build configuration ended."

if test x$gtk_ui = xtrue ; then
	echo "* GTK interface will be compiled."
fi
if test x$console_ui = xtrue ; then
	echo "* Console interface will be compiled."
fi

echo "Now type 'make' to compile, and then 'make install' as root to install it."
