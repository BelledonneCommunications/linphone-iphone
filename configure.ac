dnl Process this file with autoconf to produce a configure script.

AC_INIT([linphone],[3.10.2],[linphone-developers@nongnu.org])
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

LIBLINPHONE_SO_CURRENT=9 dnl increment this number when you add/change/remove an interface
LIBLINPHONE_SO_REVISION=0 dnl increment this number when you change source code, without changing interfaces; set to 0 when incrementing CURRENT
LIBLINPHONE_SO_AGE=0 dnl increment this number when you add an interface, set to 0 if you remove an interface

LIBLINPHONE_SO_VERSION=$LIBLINPHONE_SO_CURRENT:$LIBLINPHONE_SO_REVISION:$LIBLINPHONE_SO_AGE

AC_SUBST(LIBLINPHONE_SO_CURRENT, $LIBLINPHONE_SO_CURRENT)
AC_SUBST(LIBLINPHONE_SO_VERSION, $LIBLINPHONE_SO_VERSION)
AC_SUBST(LINPHONE_VERSION)

AC_MSG_NOTICE([$PACKAGE_NAME-$PACKAGE_VERSION		A full featured audio/video sip phone.])
AC_MSG_NOTICE([licensed under the terms of the General Public License (GPL)])

AM_INIT_AUTOMAKE([1.9 tar-pax subdir-objects])
AC_SUBST([LIBTOOL_DEPS])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],)
AC_SUBST([docdir], [${datadir}/doc])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4])
dnl do not put anythingelse before AC_PROG_CC unless checking if macro still work for clang
AC_PROG_CXX(["xcrun clang++" g++])
AC_PROG_CC(["xcrun clang" gcc])
AC_PROG_OBJC(["xcrun clang" gcc])

gl_LD_OUTPUT_DEF

AC_ISC_POSIX
AC_C_INLINE
AC_HEADER_STDC
AM_PROG_CC_C_O
AC_CHECK_PROGS(MD5SUM,[md5sum md5])
AM_CONDITIONAL(HAVE_MD5SUM,test -n $MD5SUM)

ios_found=no

case $target in
	*mingw32ce)
		CFLAGS="$CFLAGS -D_WIN32_WCE -DORTP_STATIC -D_WIN32_WINNT=0x0501"
		CXXFLAGS="$CXXFLAGS -DORTP_STATIC -D_WIN32_WINNT=0x0501"
		LIBS="$LIBS -lws2 -liphlpapi"
		mingw_found=yes
		mingwce_found=yes
	;;
	*mingw*)
		dnl Workaround for mingw, whose compiler does not check in /usr/include ...
		CPPFLAGS="$CPPFLAGS -I/usr/include"
		LDFLAGS="$LDFLAGS -L/usr/lib"
		CFLAGS="$CFLAGS -DORTP_STATIC -D_WIN32_WINNT=0x0501 "
		CXXFLAGS="$CXXFLAGS -DORTP_STATIC -D_WIN32_WINNT=0x0501"
		LIBS="$LIBS -lws2_32"
		GUI_FLAGS="-mwindows"
		CONSOLE_FLAGS="-mconsole"
		mingw_found=yes
		AC_CHECK_TOOL(WINDRES, windres)
	;;
	armv6-apple-darwin|armv7-apple-darwin|i386-apple-darwin|armv7s-apple-darwin|aarch64-apple-darwin|*-apple-darwin.ios)
		CFLAGS="$CFLAGS -DTARGET_OS_IPHONE=1 "
		LIBS="$LIBS -framework CoreFoundation -framework AudioToolbox -framework CoreAudio -framework Foundation -framework QuartzCore -framework OpenGLES -framework UIKit -framework AVFoundation"
		ios_found=yes
	;;
	x86_64-apple-darwin*|i686-apple-darwin*)
		MSPLUGINS_CFLAGS=""
		dnl use macport installation
		AS_IF([test -d "/opt/local/share/aclocal"], [ACLOCAL_MACOS_FLAGS="-I /opt/local/share/aclocal"])
		build_macos=yes
	;;

esac

AM_CONDITIONAL(BUILD_IOS, test x$ios_found = xyes)

AC_SUBST(ACLOCAL_MACOS_FLAGS)
AC_SUBST(CONSOLE_FLAGS)
AC_SUBST(GUI_FLAGS)

case "$build_os" in
	*darwin*)
		HTTPS_CA_DIR=`openssl version -d | sed  "s/OPENSSLDIR: \"\(.*\)\"/\1/"`
	;;
esac

AC_SUBST(HTTPS_CA_DIR)

dnl localization tools
IT_PROG_INTLTOOL([0.40], [no-xml])

dnl Initialize libtool
LT_INIT([win32-dll shared disable-static])

dnl Enable library dependencies linking
AC_ARG_ENABLE(deplibs-link,
	[AS_HELP_STRING([--disable-deplibs-link ], [Disable library dependencies linking (might break builds)])],
	[enable_deplibs_linking="$enableval"],
	[enable_deplibs_linking="yes"]
)
AC_MSG_NOTICE([Enable library dependencies linking: $enable_interlib_deps])
if test "${enable_deplibs_linking}" == "yes"; then
	link_all_deplibs=yes
	link_all_deplibs_CXX=yes
else
	link_all_deplibs=no
	link_all_deplibs_CXX=no
fi

AC_CONFIG_COMMANDS([libtool-hacking],
	[if test "$mingw_found" = "yes" ; then
		echo "Hacking libtool to work with mingw..."
		sed -e 's/\*\" \$a_deplib \"\*/\*/' < ./libtool > libtool.tmp
		cp -f ./libtool.tmp ./libtool
		rm -f ./libtool.tmp
	fi],
	[mingw_found=$mingw_found]
)

dnl Add the languages which your application supports here.
PKG_PROG_PKG_CONFIG
ALL_LINGUAS=$(cd $srcdir/po && echo *.po | sed 's/\.po//g')
AC_SUBST(ALL_LINGUAS)
AC_DEFINE_UNQUOTED(LINPHONE_ALL_LANGS, "$ALL_LINGUAS", [All supported languages])

if test "$mingw_found" != "yes" ; then
	dnl gettext macro does not work properly under mingw. And we want to use the one provided by GTK.

	dnl AM_GNU_GETTEXT pollutes CPPFLAGS: workaround this.
	CPPFLAGS_save=$CPPFLAGS
	AM_GNU_GETTEXT([external])
	AC_SUBST(INTLLIBS)
	CPPFLAGS=$CPPFLAGS_save
	LIBS="$LIBS $LIBINTL"
else
	if test "$USE_NLS" = "yes" ; then
		AC_DEFINE(HAVE_INTL,1,[Tells wheter localisation is possible])
		LIBS="$LIBS -lintl"
	fi
fi

GETTEXT_PACKAGE=linphone
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The name of the gettext package name])
dnl AC_CHECK_LIB(intl,libintl_gettext)

PKG_CHECK_MODULES(BCTOOLBOX, bctoolbox, [found_bctoolbox=yes],[found_bctoolbox=no])
if test "x$found_bctoolbox" != "xyes" ; then
	AC_MSG_ERROR(["Could not find bctoolbox (required dependency)"])
fi

AC_CHECK_FUNCS([get_current_dir_name strndup stpcpy] )

AC_ARG_ENABLE(x11,
	[AS_HELP_STRING([--disable-x11], [Disable X11 support (default=yes for MacOSX, no otherwise)])],
	[case "${enableval}" in
		yes)	enable_x11=true ;;
		no)	enable_x11=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --disable-x11) ;;
	esac],
	[case "$target_os" in
		*darwin*) enable_x11=false ;; #disable x11 on MacOS by default
		*) enable_x11=true ;;
	esac]
)

dnl conditional build of LDAP support
AC_ARG_ENABLE(ldap,
	[AS_HELP_STRING([--enable-ldap], [Enables LDAP support (default=no)])],
	[case "${enableval}" in
		yes)	enable_ldap=true ;;
		no)		enable_ldap=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-ldap) ;;
	esac],
	[enable_ldap=false]
)

if test "$enable_ldap" = "true"; then
	PKG_CHECK_MODULES(LDAP, [openldap],[found_ldap=yes], [found_ldap=no])
	if test  "$found_ldap" = "no"; then
		AC_CHECK_LIB(ldap,ldap_initialize, [LDAP_LIBS="-lldap -llber"],
			[AC_MSG_ERROR([You need libldap for LDAP support])]
		)
		AC_CHECK_HEADERS(ldap.h, [foo=bar], [AC_MSG_ERROR( [ldap.h not found] ) ] )
		found_ldap=yes
	fi

	PKG_CHECK_MODULES(SASL, [libsasl2],[found_sasl=yes],[found_sasl=no] )

	if test "$found_sasl" = "no"; then
		AC_CHECK_LIB(sasl2, sasl_client_init , [SASL_LIBS="-lsasl2"],
			[AC_MSG_ERROR([You need SASL for LDAP support] ) ]
			)
		AC_CHECK_HEADERS(sasl/sasl.h,foo=bar, [AC_MSG_ERROR([sasl/sasl.h not found])])
		found_sasl=yes
	fi

	AC_SUBST(LDAP_CFLAGS)
	AC_SUBST(LDAP_LIBS)

	AC_SUBST(SASL_CFLAGS)
	AC_SUBST(SASL_LIBS)

	if test "$found_ldap$found_sasl" = "yesyes"; then
		AC_DEFINE(BUILD_LDAP,1,[Defined if LDAP build option enabled])
	else
		AC_MSG_ERROR([Cannot use LDAP due to previous errors])
	fi

fi

AM_CONDITIONAL(BUILD_LDAP, test x$enable_ldap != xfalse)

dnl conditionnal build of console interface.
AC_ARG_ENABLE(console_ui,
	[AS_HELP_STRING([--enable-console_ui=[yes/no]], [Turn on or off compilation of console interface (default=yes)])],
	[case "${enableval}" in
		yes)	console_ui=true ;;
		no)	console_ui=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-console_ui) ;;
	esac],
	[console_ui=true]
)

dnl conditionnal build of tools.
AC_ARG_ENABLE(tools,
	[AS_HELP_STRING([--enable-tools=[yes/no]], [Turn on or off compilation of console interface (default=yes)])],
	[case "${enableval}" in
		yes)	build_tools=true ;;
		no)	build_tools=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-tools) ;;
	esac],
	[build_tools=check]
)

dnl check for installed version of libupnp
AC_ARG_ENABLE(upnp,
	[AS_HELP_STRING([--disable-upnp], [Disable uPnP support])],
	[case "${enableval}" in
		yes)	build_upnp=true ;;
		no)	build_upnp=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --disable-upnp) ;;
	esac],
	[build_upnp=auto]
)

if test "$build_upnp" != "false" ; then
	PKG_CHECK_MODULES([LIBUPNP], [libupnp],
		[if  pkg-config --atleast-version=1.6 "libupnp < 1.7"; then
			build_upnp=true
		else
			AC_MSG_ERROR([libupnp >= 1.6 < 1.5 required.])
		fi],
		[if test "$build_upnp" == "true" ; then
			AC_MSG_ERROR([libupnp not found.])
		else
			build_upnp=false
		fi]
	)

fi

AM_CONDITIONAL(BUILD_UPNP, test x$build_upnp != xfalse)
if test "$build_upnp" != "false" ; then
	AC_DEFINE(BUILD_UPNP, 1, [Define if upnp enabled])
fi

dnl check zlib
AC_ARG_ENABLE(zlib,
	[AS_HELP_STRING([--disable-zlib], [Disable ZLib support])],
	[case "${enableval}" in
		yes)	build_zlib=true ;;
		no)	build_zlib=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --disable-zlib) ;;
	esac],
	[build_zlib=auto]
)
if test "$build_zlib" != "false" ; then
	PKG_CHECK_MODULES(ZLIB, [zlib], [found_zlib=yes], [found_zlib=no])
	if test "x$found_zlib" = "xno" ; then
		AC_CHECK_LIB(z, inflate,
			[AC_CHECK_HEADER([zlib.h],
			[AC_COMPILE_IFELSE(
				[AC_LANG_PROGRAM([[
				 #include <zlib.h>
				 #if !defined(ZLIB_VERNUM) || (ZLIB_VERNUM < 0x1230)
				 // compile error
				 #endif
			]],[])],
			[found_zlib=yes])])])
		if test "x$found_zlib" = "xno" ; then
			AC_MSG_NOTICE([zlib library and headers not found])
		else
			AC_DEFINE( HAVE_ZLIB, 1, [ZLIB support] )
			ZLIB_LIBS='-lz'
			AC_SUBST(ZLIB_LIBS)
		fi
	else
		AC_MSG_NOTICE([ZLIB found])
		AC_DEFINE( HAVE_ZLIB, 1, [ZLIB support] )
	fi
fi


dnl check libxml2
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0],[libxml2_found=yes],foo=bar)
if test "$libxml2_found" != "yes" ; then
	AC_MSG_ERROR([libxml2 not found. Install it and try again (the package is usually named libxml2-dev in the Linux distributions)])
fi

AM_CONDITIONAL(BUILD_TOOLS, test x$build_tools != xfalse)
if test "$build_tools" != "false" ; then
	build_tools=true
	AC_DEFINE(BUILD_TOOLS, 1, [Define if tools enabled] )
fi

dnl conditionnal build of gtk interface.
AC_ARG_ENABLE(gtk_ui,
	[AS_HELP_STRING([--enable-gtk_ui=[yes/no]], [Turn on or off compilation of gtk interface (default=yes)])],
	[case "${enableval}" in
		yes)	gtk_ui=true ;;
		no)	gtk_ui=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-gtk_ui) ;;
	esac],
	[gtk_ui=true]
)

if test "$gtk_ui" = "true" ; then
	PKG_CHECK_MODULES(LIBGTK, gtk+-2.0 >= 2.18.0 gthread-2.0)
	if test "$enable_x11" = "false" ; then
		PKG_CHECK_MODULES(LIBGTKMAC,[gtk-mac-integration >= 2.0.1], [found_gtkmac=true], [found_gtkmac=false])
		if test "$found_gtkmac" != "true" ; then
			dnl for newest macports, the name changed.
			PKG_CHECK_MODULES(LIBGTKMAC,[gtk-mac-integration-gtk2 >= 2.0.1], [found_gtkmac=true], [found_gtkmac=false])
		fi
		if test "$found_gtkmac" != "true" ; then
			AC_MSG_ERROR([gtk-mac-integration not found. Please install gtk-osx-application package.])
		fi
		AC_DEFINE([HAVE_GTK_OSX],[1],[Defined when gtk osx is used])
	fi

	PKG_CHECK_MODULES(LIBGLIB, [glib-2.0 >= 2.26.0], [build_status_notifier=yes], [build_status_notifier=no])
else
	echo "GTK interface compilation is disabled."
fi

AM_CONDITIONAL([BUILD_STATUS_NOTIFIER], [test "$build_status_notifier" = "yes"])

AC_ARG_ENABLE(notify,
	[AS_HELP_STRING([--enable-notify=[yes/no]], [Enable libnotify support (default=yes)])],
	[case "${enableval}" in
		yes)	notify=true ;;
		no)	notify=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-notify) ;;
	esac],
	[notify=true]
)

dnl conditionnal build of the notify library
if test "$gtk_ui" = "true" ; then
	if test "$notify" = "true"; then
		PKG_CHECK_MODULES([NOTIFY4], [libnotify >= 0.7.0 ], [found_notify4=yes], foo=bar)
		case "$found_notify4" in
			yes)
				AC_SUBST(NOTIFY4_CFLAGS)
				AC_SUBST(NOTIFY4_LIBS)
				AC_DEFINE([HAVE_NOTIFY4],[1],[NOTIFY4 support])
		esac

		PKG_CHECK_MODULES([NOTIFY1], [libnotify < 0.7.0], [found_notify1=yes], foo=bar)
		case "$found_notify1" in
			yes)
				AC_SUBST(NOTIFY1_CFLAGS)
				AC_SUBST(NOTIFY1_LIBS)
				AC_DEFINE([HAVE_NOTIFY1],[1],[NOTIFY1 support])
		esac
	else
		echo "Libnotify support is disabled."
	fi
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

AC_ARG_WITH(configdir,
	[AS_HELP_STRING([--with-configdir], [Set a APPDATA subdir where linphone is supposed to find its config (windows only)])],
	[ configdir=${withval}],[ configdir="Linphone" ])

AC_DEFINE_UNQUOTED(LINPHONE_CONFIG_DIR,"$configdir",[Windows appdata subdir where linphonerc can be found])

AC_ARG_ENABLE(relativeprefix,
	[AS_HELP_STRING([--enable-relativeprefix], [Build a linphone that finds its resources relatively to the directory where it is installed])],
	[case "${enableval}" in
		yes)	relativeprefix=yes ;;
		no)	relativeprefix=no ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-relativeprefix) ;;
	esac],
	[relativeprefix=guess]
)

AC_ARG_ENABLE(date,
	[AS_HELP_STRING([--enable-date], [Use build date in internal version number])],
	[case "${enableval}" in
		yes)	use_date=yes ;;
		no)	use_date=no ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-date) ;;
	esac],
	[use_date=no]
)

if test x$use_date =  xyes ; then
	AC_DEFINE(USE_BUILDDATE_VERSION,1,[Tell whether date_version.h must be used])
fi


dnl enable ipv6 support
AC_ARG_ENABLE(ipv6,
	[AS_HELP_STRING([--enable-ipv6], [Turn on ipv6 support])],
	[case "${enableval}" in
		yes)	ipv6=true;;
		no)	ipv6=false;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-ipv6) ;;
	esac],
	[ipv6=true]
)
IPV6_CFLAGS=
if test x$ipv6 = xtrue ; then
	IPV6_CFLAGS=-DINET6
fi
AC_SUBST(IPV6_CFLAGS)

dnl enable timestamp support
AC_ARG_ENABLE(ntp-timestamp,
	[AS_HELP_STRING([--enable-ntp-timestamp], [Turn on NTP timestamping on received packet])],
	[case "${enableval}" in
		yes)	ntptimestamp=true;;
		no)	ntptimestamp=false;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-ntp-timestamp) ;;
	esac],
	[ntptimestamp=false]
)

AC_ARG_ENABLE(debug,
	[AS_HELP_STRING([--enable-debug=[yes/no]], [Enables the display of traces showing the execution of the library. (default=yes)])],
	[case "${enableval}" in
		yes)	debug_enabled=yes;;
		no)	debug_enabled=no;;
		*)	AC_MSG_ERROR("Bad value for --enable-debug");;
	esac],
	[debug_enabled=no]
)
AS_CASE([$debug_enabled],
		[yes],[
			CFLAGS="$CFLAGS -g -O0 -DDEBUG"
			CXXFLAGS="$CXXFLAGS -g -O0 -DDEBUG"
			OBJCFLAGS="$OBJCFLAGS -g -O0 -DDEBUG"
		],
		[no],
		[
			case "$CFLAGS" in
				*-O*)
				;;
				*)
					CFLAGS="$CFLAGS -O2 -g"
					CXXFLAGS="$CXXFLAGS -O2 -g"
					OBJCFLAGS="$OBJCFLAGS -O2 -g"
				;;
			esac
		],
		[AC_MSG_ERROR([Bad value ($debug_enabled) for --enable-debug. Valid values are yes or no.])])

dnl enable truespeech codec support
AC_ARG_ENABLE(truespeech,
	[AS_HELP_STRING([--enable-truespeech], [Turn on TrueSpeech support (x86 only)])],
	[case "${enableval}" in
		yes)	truespeech=true;;
		no)	truespeech=false;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-truespeech) ;;
	esac],
	[truespeech=false]
)
TRUESPEECH_CFLAGS=
if test x$truespeech = xtrue ; then
	TRUESPEECH_CFLAGS=-DTRUESPEECH
fi
AC_SUBST(TRUESPEECH_CFLAGS)
AM_CONDITIONAL([BUILD_TRUESPEECH], [test x$truespeech = xtrue])

AC_ARG_ENABLE(nonstandard-gsm,
	[AS_HELP_STRING([--enable-nonstandard-gsm], [Enable GSM codec at nonstandard rates (11025hz, 16000hz)])],
	[case "${enableval}" in
		yes)
			exotic_gsm=yes
	 		AC_DEFINE(ENABLE_NONSTANDARD_GSM,1,[Defined when using gsm at nonstandard rates])
	 	;;
		no)	exotic_gsm=no ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-nonstandard-gsm) ;;
	esac],
	[exotic_gsm=no]
)

if test "x${prefix}" = "xNONE"; then
	package_prefix=${ac_default_prefix}
else
	package_prefix=${prefix}
fi

if test "$relativeprefix" = "guess" ; then
	if test "$mingw_found" = "yes" ; then
		relativeprefix="yes"
	fi
fi

if test "$relativeprefix" = "yes" ; then
	dnl allow binaries to install everywhere
	package_prefix="."
fi

dnl Set PACKAGE_LOCALE_DIR in config.h.
case "$target_os" in
	*qnx*)
		DATADIRNAME=app/native/assets
	;;
	*)
		DATADIRNAME=share
	;;
esac
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${package_prefix}/${DATADIRNAME}/locale",[Defines the place where locales can be found])

AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${package_prefix}/${DATADIRNAME}",[Defines the place where data are found])

dnl Set PACKAGE_SOUND_DIR in config.h.
AC_DEFINE_UNQUOTED(PACKAGE_SOUND_DIR, "${package_prefix}/${DATADIRNAME}/sounds/linphone",[Defines the place where linphone sounds are found])


dnl check if we have the getifaddrs() sytem call
AC_CHECK_FUNCS(getifaddrs)

if test "$console_ui" = "true" ; then
	dnl check gnu readline
	LP_CHECK_READLINE
else
	echo "Console interface compilation is disabled."
fi

AC_WORDS_BIGENDIAN

AC_ARG_ENABLE([speex],
	AS_HELP_STRING([--disable-speex], [Disable speex support]),
	[],
	[enable_speex=yes]
)
if test "x$enable_speex" = "xyes"; then
	dnl normaly this should only by done by mediastreamer2/configure.ac
	dnl but to workaround bugs when cross-compiling for arm-linux,
	dnl we need to have SPEEX_LIBS defined
	dnl Furthermore it is good to repeat here all mediastreamer2 toggles
	dnl since top-level configure --help will not print them.
	PKG_CHECK_MODULES(SPEEX, speex >= 1.1.6, build_speex=yes)
fi

dnl conditionnal build of video support
AC_ARG_ENABLE(video,
	[AS_HELP_STRING([--enable-video], [Turn on video support compiling (default=yes)])],
	[case "${enableval}" in
		yes)	video=true ;;
		no)	video=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-video) ;;
	esac],
	[video=true]
)

AC_ARG_WITH(ffmpeg,
	[AS_HELP_STRING([--with-ffmpeg], [Sets the installation prefix of ffmpeg, needed for video support. (default=/usr)])],
	[ ffmpegdir=${withval}],
	[ ffmpegdir=/usr ]
)

AM_CONDITIONAL([BUILD_MACOS], [test "x$build_macos" = "xyes"])

if test "$video" = "true"; then

	if test "$enable_x11" = "true"; then
	  	AC_CHECK_HEADERS(X11/Xlib.h)
		if test "$build_macos" = "yes"; then
			X11_LIBS="-L/usr/X11/lib -lX11"
		else
			AC_CHECK_LIB(X11,XUnmapWindow, X11_LIBS="-lX11")
		fi
		AC_SUBST(X11_LIBS)
	fi
	AC_DEFINE(VIDEO_ENABLED,1,[defined if video support is available])
fi

AC_ARG_ENABLE(alsa,
	[AS_HELP_STRING([--enable-alsa], [Turn on alsa native support compiling])],
	[case "${enableval}" in
		yes)	alsa=true ;;
		no)	alsa=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-alsa) ;;
	esac],
	[alsa=true]
)

dnl this options are just for passing to mediastreamer2 subproject
AC_ARG_ENABLE(zrtp,
	[AS_HELP_STRING([--enable-zrtp], [Turn on zrtp support])],
	[case "${enableval}" in
		yes)	zrtp=true ;;
		no)	zrtp=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-zrtp) ;;
	esac],
	[zrtp=false]
)

dnl this options are just for passing to mediastreamer2 subproject
AC_ARG_ENABLE(dtls,
	[AS_HELP_STRING([--enable-dtls], [Turn on srtp-dtls support - requires polarssl > 1.4])],
	[case "${enableval}" in
		yes)	dtls=true ;;
		no)	dtls=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-dtls) ;;
	esac],
	[dtls=false]
)

AC_ARG_ENABLE(g729bCN,
	[AS_HELP_STRING([--enable-g729bCN], [Turn on or off usage of G729AnnexB in RFC3389 implementation of Comfort Noise Payload (default=no)])],
	[case "${enableval}" in
		yes)	g729bCN=true ;;
		no)	g729bCN=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-g729bCN) ;;
	esac],
	[g729bCN=false]
)

dnl Polarssl lib is requested for Lime
AC_ARG_WITH( polarssl,
      [  --with-polarssl      Set prefix where polarssl can be found (ex:/usr, /usr/local)[[default=PREFIX or /usr if NONE]] ],
      [ polarssl_prefix=${withval}],[ if test "$prefix" != "NONE"; then
					polarssl_prefix=${prefix}
				      else
					polarssl_prefix="/usr"
				      fi ])

found_polarssl=no

if test "$polarssl_prefix" != "none" ; then
	if test "$polarssl_prefix" != "/usr" ; then
		POLARSSL_CFLAGS="-I${polarssl_prefix}/include"
		POLARSSL_LIBS="-L${polarssl_prefix}/lib"
	fi
	POLARSSL_LIBS="$POLARSSL_LIBS -lpolarssl"

	CPPFLAGS_save=$CPPFLAGS
	LIBS_save=$LIBS

	CPPFLAGS="$CPPFLAGS $POLARSSL_CFLAGS"
	LIBS="$LIBS $POLARSSL_LIBS"
	AC_CHECK_HEADERS(polarssl/gcm.h,
				[found_polarssl=yes; AC_MSG_NOTICE([polarssl usable])],
				[POLARSSL_CFLAGS=""
					POLARSSL_LIBS=""])
	CPPFLAGS=$CPPFLAGS_save
	LIBS=$LIBS_save
fi

dnl check for Lime support, need polarssl version >= 1.3 (with gcm.h)
AC_ARG_ENABLE(lime,
	[AS_HELP_STRING([--enable-lime], [Turn on or off compilation of Instant Messaging Encryption (default=auto)])],
	[case "${enableval}" in
		yes)	lime=true ;;
		no)	lime=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-lime) ;;
	esac],
	[lime=false]
)

if test "$lime" != "false" ; then
	if test "x$found_polarssl" != "xyes" ; then
		if test "$lime" = "true" ; then
			AC_MSG_ERROR("LIME requires POLARSSL in version >= 1.3")
		fi
		lime=false
	else
		AC_DEFINE(HAVE_LIME, 1, [Defined when LIME support is compiled])
		lime=true
	fi
fi

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

AC_ARG_ENABLE(assistant,
	[AS_HELP_STRING([--enable-assistant], [Turn on assistant compiling])],
	[case "${enableval}" in
		yes)	build_wizard=true ;;
		no)	build_wizard=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-assistant) ;;
	esac],
	[build_wizard=check]
)

if test "$build_wizard" != "false" ; then
	PKG_CHECK_MODULES(LIBGTKWIZARD, [gtk+-2.0 >= 2.22.0],[],
		[if test "$build_wizard" = "true" ; then
			AC_MSG_ERROR([gtk+-2.0 < 2.22.0, assistant cannot be compiled.])
		else
			build_wizard=false
		fi]
	)
fi
AM_CONDITIONAL(BUILD_WIZARD, test x$build_wizard != xfalse)
if test "$build_wizard" != "false" ; then
	build_wizard=true
	 AC_DEFINE(BUILD_WIZARD, 1, [Define if wizard enabled] )
fi

AC_CHECK_HEADERS(libudev.h)
AC_CHECK_LIB(udev,udev_new)

##################################################
# Stricter build options (after external packages)
##################################################


AC_ARG_ENABLE(strict,
	AC_HELP_STRING([--enable-strict], [Build with stricter options @<:@yes@:>@]),
	[strictness="${enableval}"],
	[strictness=yes]
)

STRICT_OPTIONS="-Wall -Wuninitialized"
STRICT_OPTIONS_CC="-Wstrict-prototypes"
STRICT_OPTIONS_CXX=""

#for clang

case $CC in
	*gcc*)
		STRICT_OPTIONS="$STRICT_OPTIONS -fno-inline-small-functions"
	;;
	*clang*)
		STRICT_OPTIONS="$STRICT_OPTIONS -Qunused-arguments "
		#disabled due to wrong optimization false positive with small string
		#(cf. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=35903)
		STRICT_OPTIONS="$STRICT_OPTIONS -Wno-array-bounds "
	;;
esac
# because Darwin's gcc is actually clang, we need to check it...
case "$target_os" in
	*darwin*)
	STRICT_OPTIONS="$STRICT_OPTIONS -Wno-error=unknown-warning-option -Qunused-arguments -Wno-tautological-compare -Wno-unused-function "
	#disabled due to wrong optimization false positive with small string
	#(cf. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=35903)
	STRICT_OPTIONS="$STRICT_OPTIONS -Wno-array-bounds "
	;;
esac
if test "$strictness" = "yes" ; then
	STRICT_OPTIONS="$STRICT_OPTIONS -Werror -Wextra -Wno-unused-parameter -Wno-error=deprecated-declarations -Wno-missing-field-initializers"
	CFLAGS="$CFLAGS -fno-strict-aliasing"
fi

AC_SUBST(STRICT_OPTIONS)
AC_SUBST(STRICT_OPTIONS_CC)
AC_SUBST(STRICT_OPTIONS_CXX)

top_srcdir=`dirname $0`

AC_ARG_ENABLE(external-ortp,
	[AS_HELP_STRING([--enable-external-ortp], [Use external oRTP library])],
	[case "${enableval}" in
		yes)	external_ortp=true ;;
		no)	external_ortp=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-external-ortp) ;;
	esac],
	[external_ortp=false]
)

if test "$external_ortp" = 'true'; then
	PKG_CHECK_MODULES([ORTP], [ortp >= 0.24.0])
	ORTP_VERSION=`$PKG_CONFIG --modversion ortp`
else
	AC_CONFIG_SUBDIRS( oRTP )
	ORTP_CFLAGS="-I\$(top_srcdir)/oRTP/include"
	ORTP_LIBS="\$(top_builddir)/oRTP/src/libortp.la"
	if test x$ac_cv_c_bigendian = xyes ; then
		ORTP_CFLAGS="$ORTP_CFLAGS -DORTP_BIGENDIAN"
	fi
	if test x$ntptimestamp = xtrue ; then
		ORTP_CFLAGS="$ORTP_CFLAGS -DORTP_TIMESTAMP"
	fi
	ORTP_DIR=oRTP
	changequote(<<, >>)
	ORTP_VERSION=`grep -E ^[AC]+_INIT ${top_srcdir}/oRTP/configure.ac | sed -e 's:^.*_INIT(.*,\[\(.*\)\]):\1:g'`
	changequote([, ])
fi
AC_SUBST(ORTP_CFLAGS)
AC_SUBST(ORTP_LIBS)
AC_SUBST([ORTP_VERSION])
AC_SUBST([ORTP_DIR])

AC_ARG_ENABLE([external-mediastreamer],
	[AS_HELP_STRING([--enable-external-mediastreamer],[Use external mediastreamer library])],,
	[enable_external_mediastreamer=no]
)

AS_CASE($enable_external_mediastreamer,
	[yes],
	[PKG_CHECK_MODULES([MEDIASTREAMER], [mediastreamer >= 2.11.0])
	MS2_VERSION=`$PKG_CONFIG --modversion mediastreamer`],
	[no],
	[AC_CONFIG_SUBDIRS( mediastreamer2 )
	MEDIASTREAMER_DIR=${top_srcdir}/mediastreamer2
	MEDIASTREAMER_CFLAGS="-I\$(top_srcdir)/mediastreamer2/include"
	MEDIASTREAMER_LIBS="\$(top_builddir)/mediastreamer2/src/libmediastreamer_base.la \$(top_builddir)/mediastreamer2/src/libmediastreamer_voip.la"
dnl need to temporary change quotes to allow square brackets
	changequote(<<, >>)
	MS2_VERSION=`grep -e '^.C_INIT(' $MEDIASTREAMER_DIR/configure.ac | sed -e 's:\([^(]\+\)(\[mediastreamer\],\[\(.*\)\]):\2:g'`
	changequote([, ])
	MS2_DIR=mediastreamer2],
	[AC_MSG_ERROR([bad value '${enable_external_mediastreamer}' for --enable-external-mediastreamer])]
)

AC_SUBST(MEDIASTREAMER_CFLAGS)
AC_SUBST(MEDIASTREAMER_LIBS)
AC_SUBST([MS2_VERSION])
AC_SUBST([MS2_DIR])


AC_ARG_ENABLE(tunnel,
	[AS_HELP_STRING([--enable-tunnel=[yes/no]], [Turn on compilation of tunnel support (default=no)])],
	[case "${enableval}" in
		yes)	enable_tunnel=true ;;
		no)	enable_tunnel=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-tunnel) ;;
	esac],
	[enable_tunnel=false]
)
AM_CONDITIONAL(BUILD_TUNNEL, test x$enable_tunnel = xtrue)
if test x$enable_tunnel = xtrue; then
	PKG_CHECK_MODULES(TUNNEL, tunnel >= 0.6.0)
	AC_DEFINE(TUNNEL_ENABLED,1,[Tells tunnel extension is built-in])
fi

AC_ARG_ENABLE(vcard,
	[AS_HELP_STRING([--enable-vcard=[yes/no]], [Turn on compilation of vcard (default=auto)])],
	[case "${enableval}" in
		yes)	enable_vcard=true ;;
		no)		enable_vcard=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-vcard) ;;
	esac],
	[enable_vcard=auto]
)

if test x$enable_vcard != xfalse; then
	PKG_CHECK_MODULES(BELCARD, belcard, [found_vcard=yes],[found_vcard=no])
	if test "$found_vcard" = "no"; then
		dnl Check the lib presence in case the PKG-CONFIG version is not found
		AC_LANG_CPLUSPLUS
		AC_CHECK_LIB(belcard, main, [BELCARD_LIBS+=" -lbelr -lbelcard"; found_vcard=yes], [foo=bar])
		AC_LANG_C
	fi
	if test "$found_vcard" = "yes"; then
		BELCARD_CFLAGS+=" -DVCARD_ENABLED"
		enable_vcard=true
	else
		if test x$enable_vcard = xtrue; then
			AC_MSG_ERROR([belcard, required for vcard support, not found])
		fi
		enable_vcard=false
	fi

	AC_SUBST(BELCARD_CFLAGS)
	AC_SUBST(BELCARD_LIBS)
fi

AM_CONDITIONAL(BUILD_VCARD, test x$enable_vcard = xtrue)

AC_ARG_ENABLE(sqlite-storage,
	[AS_HELP_STRING([--enable-sqlite-storage=[yes/no]], [Turn on compilation of sqlite storage for call history, messages, friends (default=auto)])],
	[case "${enableval}" in
		yes)	enable_sqlite_storage=true ;;
		no)	enable_sqlite_storage=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --enable-sqlite-storage) ;;
	esac],
	[enable_sqlite_storage=auto]
)

if test x$enable_sqlite_storage != xfalse; then
	PKG_CHECK_MODULES(SQLITE3,[sqlite3 >= 3.6.0],[found_sqlite=yes],[found_sqlite=no])
	if test "$found_sqlite" = "no"; then
		dnl Check the lib presence in case the PKG-CONFIG version is not found
		AC_CHECK_LIB(sqlite3, sqlite3_open, [SQLITE3_LIBS+=" -lsqlite3 "; found_sqlite=yes], [foo=bar])
	fi
	if test "$found_sqlite" = "yes"; then
		SQLITE3_CFLAGS+=" -DSQLITE_STORAGE_ENABLED"
		if test "$build_macos" = "yes" -o "$ios_found" = "yes"; then
			SQLITE3_LIBS+=" -liconv"
		fi
		enable_sqlite_storage=true
	else
		if test x$enable_sqlite_storage = xtrue; then
			AC_MSG_ERROR([sqlite3, required for storage, not found])
		fi
		enable_sqlite_storage=false
	fi

	AC_SUBST(SQLITE3_CFLAGS)
	AC_SUBST(SQLITE3_LIBS)
fi

AM_CONDITIONAL(BUILD_SQLITE_STORAGE, test x$enable_sqlite_storage = xtrue)


PKG_CHECK_MODULES(BELLESIP, [belle-sip >= 1.4.2])

SIPSTACK_CFLAGS="$BELLESIP_CFLAGS"
SIPSTACK_LIBS="$BELLESIP_LIBS"


AC_SUBST(SIPSTACK_CFLAGS)
AC_SUBST(SIPSTACK_LIBS)

dnl check for db2html (docbook) to generate html user manual
AC_CHECK_PROG(have_sgmltools, sgmltools, yes, no)
AM_CONDITIONAL(ENABLE_MANUAL, test x$have_sgmltools$build_manual = xyesyes )

dnl for external use of linphone libs
LINPHONE_CFLAGS="-I${includedir} -I${includedir}/linphone"
LINPHONE_LIBS="-L${libdir} -llinphone"

AC_SUBST(LINPHONE_CFLAGS)
AC_SUBST(LINPHONE_LIBS)

AC_DEFINE_UNQUOTED(LINPHONE_VERSION, "$PACKAGE_VERSION", [Linphone\'s version number])

AC_DEFINE_UNQUOTED(LINPHONE_PLUGINS_DIR, "${package_prefix}/lib/liblinphone/plugins" ,[path of liblinphone plugins, not mediastreamer2 plugins])
LINPHONE_PLUGINS_DIR="${package_prefix}/lib/liblinphone/plugins"
AC_SUBST(LINPHONE_PLUGINS_DIR)



AC_ARG_ENABLE(tutorials,
	[AS_HELP_STRING([--disable-tutorials], [Disable compilation of tutorials])],
	[case "${enableval}" in
		yes)	tutorials_enabled=true ;;
		no)	tutorials_enabled=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --disable-tutorials) ;;
	esac],
	[tutorials_enabled=yes]
)
AM_CONDITIONAL(ENABLE_TUTORIALS, test x$tutorials_enabled = xyes)

AC_ARG_ENABLE(tests,
	[AS_HELP_STRING([--disable-tests], [Disable compilation of tests])],
	[case "${enableval}" in
		yes)	tests_enabled=true ;;
		no)	tests_enabled=false ;;
		*)	AC_MSG_ERROR(bad value ${enableval} for --disable-tests) ;;
	esac],
	[tests_enabled=yes]
)
AM_CONDITIONAL(ENABLE_TESTS, test x$tests_enabled = xyes)

PKG_CHECK_MODULES(BCTOOLBOXTESTER, bctoolbox-tester, [found_pkg_config_bctoolboxtester=yes],[found_pkg_config_bctoolboxtester=no])

AM_CONDITIONAL([ENABLE_TESTS], [test x$found_pkg_config_bctoolboxtester = xyes && test x$tests_enabled != xfalse])
if test "$found_pkg_config_bctoolboxtester" = "no" ; then
        AC_MSG_WARN([Could not find bctoolbox-tester wrapper, tests are not compiled.])
fi



dnl ##################################################
dnl # Check for doxygen
dnl ##################################################

AC_ARG_ENABLE(documentation,
	[AS_HELP_STRING([--enable-documentation], [Documentation generation using doxygen (default=yes)])],
	[case "${enableval}" in
		yes)	documentation_enabled=yes;;
		no)		documentation_enabled=no;;
		*)		AC_MSG_ERROR("Bad value for --enable-documentation");;
	esac],
	[documentation_enabled=yes]
)
if test "$documentation_enabled" = "yes" ; then
	AC_CHECK_PROG(DOXYGEN,doxygen,doxygen,false)
else
	DOXYGEN=false
fi
AM_CONDITIONAL(HAVE_DOXYGEN, test "$DOXYGEN" != "false")

AC_PATH_PROG([SIPP],[sipp],[false])
if test "x$SIPP" != "xfalse" ; then
	AC_DEFINE(HAVE_SIPP,1,[defined when SIPP is available])
	AC_DEFINE_UNQUOTED(SIPP_COMMAND,"$SIPP",[defined when SIPP is available])
fi

AC_CONFIG_FILES([
	Makefile
	build/Makefile
	build/macos/Makefile
	build/macos/Info-linphone.plist
	build/macos/pkg-distribution.xml
	m4/Makefile
	po/Makefile.in
	pixmaps/Makefile
	include/Makefile
	include/linphone/Makefile
	coreapi/Makefile
	coreapi/help/Makefile
	coreapi/help/Doxyfile
	tester/Makefile
	gtk/Makefile
	console/Makefile
	daemon/Makefile
	share/Makefile
	share/C/Makefile
	share/fr/Makefile
	share/it/Makefile
	share/ja/Makefile
	share/cs/Makefile
	share/xml/Makefile
	share/linphone.pc
	share/linphone.desktop
	share/audio-assistant.desktop
	scripts/Makefile
	tools/Makefile
	linphone.spec
	linphone.iss
])

AC_OUTPUT

echo "Linphone build configuration ended."
echo "Summary of build options:"
printf "* %-30s %s\n" "Video support"		$video
printf "* %-30s %s\n" "GTK interface"		$gtk_ui
printf "* %-30s %s\n" "Account assistant"	$build_wizard
printf "* %-30s %s\n" "Console interface"	$console_ui
printf "* %-30s %s\n" "Tools"				$build_tools
printf "* %-30s %s\n" "Sqlite storage"		$enable_sqlite_storage
printf "* %-30s %s\n" "VCard support"		$enable_vcard
printf "* %-30s %s\n" "IM encryption"		$lime
printf "* %-30s %s\n" "uPnP support" 		$build_upnp
printf "* %-30s %s\n" "LDAP support" 		$enable_ldap
printf "* %-30s %s\n" "ZLIB support" 		$found_zlib
printf "* %-30s %s\n" "Documentation" 		$documentation_enabled

if test "$enable_tunnel" = "true" ; then
	printf "* %-30s %s\n" "Tunnel support" "true"
fi


echo "Now type 'make' to compile, and then 'make install' as root to install it."
