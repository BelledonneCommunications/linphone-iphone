dnl Process this file with autoconf to produce a configure script.
AC_INIT([ortp],[0.16.1])
AC_CANONICAL_SYSTEM

dnl Source packaging numbers
ORTP_MAJOR_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f1)
ORTP_MINOR_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f2)
ORTP_MICRO_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f3)
ORTP_EXTRA_VERSION=$(echo $PACKAGE_VERSION | cut -d. -f4)

LIBORTP_SO_CURRENT=8 dnl increment this number when you add/change/remove an interface
LIBORTP_SO_REVISION=0 dnl increment this number when you change source code, without changing interfaces; set to 0 when incrementing CURRENT
LIBORTP_SO_AGE=0 dnl increment this number when you add an interface, set to 0 if you remove an interface

LIBORTP_SO_VERSION=$LIBORTP_SO_CURRENT:$LIBORTP_SO_REVISION:$LIBORTP_SO_AGE
ORTP_VERSION=${ORTP_MAJOR_VERSION}.${ORTP_MINOR_VERSION}.${ORTP_MICRO_VERSION}

if test -n "$ORTP_EXTRA_VERSION" ; then
	ORTP_VERSION="${ORTP_VERSION}.${ORTP_EXTRA_VERSION}"
fi

ORTP_PKGCONFIG_VERSION=${ORTP_VERSION}

AC_SUBST(LIBORTP_SO_VERSION)
AC_SUBST(ORTP_VERSION)
AC_SUBST(ORTP_PKGCONFIG_VERSION)


PACKAGE=ortp

AM_INIT_AUTOMAKE([tar-ustar])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],)
AC_CONFIG_HEADERS(ortp-config.h)
AC_DEFINE_UNQUOTED(ORTP_MAJOR_VERSION,$ORTP_MAJOR_VERSION, [major version])
AC_DEFINE_UNQUOTED(ORTP_MINOR_VERSION,$ORTP_MINOR_VERSION, [minor version])
AC_DEFINE_UNQUOTED(ORTP_MICRO_VERSION,$ORTP_MICRO_VERSION, [micro version])
AC_DEFINE_UNQUOTED(ORTP_VERSION,"$ORTP_VERSION",[ortp version number])

AC_SUBST([mkdir_p])

AC_MSG_CHECKING([warning make an error on compilation])
AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=yes]],
[wall_werror=$enableval],
[wall_werror=yes]
)

AC_ARG_ENABLE(perf,
[  --enable-perf       Disable costly features to reduce cpu consumtion [default=no]],
[perf=$enableval],
[perf=no]
)

ORTP_DEFS=

dnl enable ipv6 support
AC_ARG_ENABLE(ipv6,
      [  --enable-ipv6    Turn on ipv6 support],
      [case "${enableval}" in
        yes)  ipv6=true;;
        no)   ipv6=false;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-ipv6) ;;
      esac],[ipv6=true])
if test x$ipv6 = xtrue ; then
	ORTP_DEFS="$ORTP_DEFS -DORTP_INET6"
fi

AC_ARG_ENABLE(mode64bit,
			[  --enable-mode64bit=[yes/no]   produce a 64-bit library. [default=no]],
			[case "${enableval}" in
				yes) mode64bit_enabled=yes;;
				no) mode64bit_enabled=no;;
				*) AC_MSG_ERROR("Bad value for --enable-mode64bit");;
			esac],
			[mode64bit_enabled=no])

AC_ARG_ENABLE(debug,
			[  --enable-debug=[yes/no]   enables the display of traces showing the execution of the library. [default=yes]],
			[case "${enableval}" in
				yes) debug_enabled=yes;;
				no) debug_enabled=no;;
				*) AC_MSG_ERROR("Bad value for --enable-debug");;
			esac],
			[debug_enabled=no])


hpux_host=no
posixtimer_interval=10000
PTHREAD_LDFLAGS=

case "$target_os" in
	*hpux*) 
		hpux_host=yes
		AC_DEFINE(NOCONNECT,1,[Defined if we should not use connect() on udp sockets])
		CFLAGS="$CFLAGS -D_HPUX_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506L"
		LIBS="$LIBS -lxnet"
		;;
	*freebsd*)
		AC_DEFINE(NOCONNECT,1,[Defined if we should not use connect() on udp sockets])
		PTHREAD_LDFLAGS="-pthread"
		;;
	*mingw*)
		CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0501 -DORTP_STATIC"
		LIBS="$LIBS -lws2_32 -liphlpapi -lwinmm"
		build_tests=no
		;;
esac

dnl Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)

if test "$GCC" != "yes" ; then
	if test "$hpux_host" = "yes" ; then
		dnl we are probably using HPUX cc compiler, so add a +O2 to CFLAGS
			CFLAGS="$CFLAGS +O2 -g "
		if test x$mode64bit_enabled = xyes ; then
			CFLAGS="$CFLAGS +DA2.0W +DS2.0"
		fi
	fi
else
	CFLAGS="$CFLAGS -Wall"	
fi

build_scheduler=yes

dnl Check if we have seteuid system call
AC_CHECK_FUNCS(seteuid)


dnl check if we can use the pthread_library
AC_CHECK_LIB(pthread, pthread_mutex_init, [pthread_enabled=yes], [pthread_enabled=no])
if test $pthread_enabled = "no" ; then
	build_scheduler=no
else
	PTHREAD_LIBS="-lpthread"
	PTHREAD_CFLAGS="-D_REENTRANT"
	AC_SUBST(PTHREAD_CFLAGS)
	AC_SUBST(PTHREAD_LIBS)
	AC_SUBST(PTHREAD_LDFLAGS)
fi
AC_ARG_WITH(thread-stack-size,
	AC_HELP_STRING([--with-thread-stack-size=SIZE-IN-BYTES],[Set thread stack size [[default=os-default]]]),
	[thread_stack_size=$withval], [thread_stack_size=0])
AC_DEFINE_UNQUOTED(ORTP_DEFAULT_THREAD_STACK_SIZE, $thread_stack_size, [Default thread stack size (0 = let operating system decide)])


dnl check for libsrtp support (secure rtp)
AC_ARG_WITH( srtp,
      [  --with-srtp      Set prefix where libsrtp can be found (ex:/usr or /usr/local)[default=/usr] ],
      [ srtp_prefix=${withval}],[ srtp_prefix=/usr ])


if test "${srtp_prefix}" != "/usr" ; then
	SRTP_CFLAGS="-I${srtp_prefix}/include"
	SRTP_LIBS="-L${srtp_prefix}/lib"
fi
SRTP_LIBS="$SRTP_LIBS -lsrtp"

dnl check srtp headers
have_srtp=no
CPPFLAGS_save=$CPPFLAGS
CPPFLAGS=$SRTP_CFLAGS
AC_CHECK_HEADER([srtp/srtp.h],have_srtp_headers=yes)
CPPFLAGS=$CPPFLAGS_save

dnl check for srtp lib
LDFLAGS_save=$LDFLAGS
LDFLAGS=$SRTP_LIBS
LIBS_save=$LIBS
AC_CHECK_LIB(srtp,srtp_init,have_srtp_lib=yes)
LDFLAGS=$LDFLAGS_save
LIBS=$LIBS_save

if test "$have_srtp_headers$have_srtp_lib" = "yesyes" ; then
	have_srtp=yes
	AC_DEFINE(HAVE_SRTP, 1, [Defined when srtp support is compiled])
else
	AC_MSG_NOTICE([Could not find libsrtp headers or lib, cryto transport disabled.])
	SRTP_CFLAGS=
	SRTP_LIBS=
fi

AC_SUBST(SRTP_CFLAGS)
AC_SUBST(SRTP_LIBS)

if test $debug_enabled = "yes"; then
	ORTP_DEFS="$ORTP_DEFS -DORTP_DEBUG_MODE -g"
	CFLAGS=`echo $CFLAGS | sed 's/-O.//'`
fi

AC_ARG_ENABLE(memcheck,
				[  --enable-memcheck=[yes/no]   enables memory leak detection (HPUX only).],
				[case "${enableval}" in
				yes) memcheck_enabled=yes;;
				no) memcheck_enabled=no;;
				*) AC_MSG_ERROR("Bad value for --enable-memcheck");;
			esac],
				[memcheck_enabled=no])
				
if test "$memcheck_enabled" = "yes" ; then
	if test "$hpux_host" = "yes" ; then
		AC_DEFINE(ENABLE_MEMCHECK,1,[Defined when memory leak checking if enabled])
	else
		echo "WARNING ************ : the memory check option is only available for HPUX."
	fi
fi

AC_ARG_ENABLE(so-reuseaddr,
				[  --enable-so-reuseaddr=[yes/no]   enables SO_REUSEADDR socket option in the rtp_session_set_local_addr() function.],
				[case "${enableval}" in
					yes) so_reuseaddr_enabled=yes;;
					no) so_reuseaddr_enabled=no;;
					*) AC_MSG_ERROR("Bad value for --enable-so-reuseaddr");;
				esac],
				[so_reuseaddr_enabled=yes])
				
if test "$so_reuseaddr_enabled" = "yes" ; then
	AC_DEFINE(SO_REUSE_ADDR,1,[Defined when SO_REUSEADDR socket option in the rtp_session_set_local_addr() function is enabled])
fi


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(poll.h sys/poll.h sys/uio.h fcntl.h sys/time.h unistd.h sys/audio.h linux/soundcard.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_WORDS_BIGENDIAN
if test x$ac_cv_c_bigendian = xyes ; then
	ORTP_DEFS="$ORTP_DEFS -DORTP_BIGENDIAN"
fi

dnl Checks for library functions.
AC_CHECK_FUNCS(select socket strerror)

if test $hpux_host = "yes" ; then
dnl it seems 10 ms is too fast on hpux and it causes trouble 
		posixtimer_interval=20000
fi

AC_DEFINE_UNQUOTED(POSIXTIMER_INTERVAL,$posixtimer_interval,[Defines the periodicity of the rtp scheduler in microseconds])

if test "$perf" = "yes" ; then
	CFLAGS="$CFLAGS -DPERF"
fi

if test $GCC = yes && test $wall_werror = yes;  then
	CFLAGS="$CFLAGS -Werror "
fi

AC_CHECK_HEADERS(openssl/hmac.h openssl/md5.h)

AC_CHECK_LIB(ssl,SSL_CTX_new,[SSL_LIBS="-lssl"])
AC_CHECK_LIB(crypto,MD5,[SSL_LIBS="$SSL_LIBS -lcrypto"])

AC_SUBST(SSL_LIBS)

ORTPDEPS_LIBS="$ORTPDEPS_LIBS $PTHREAD_LIBS $PTHREAD_LDFLAGS $SRTP_LIBS $SSL_LIBS"
ORTPDEPS_CFLAGS="$ORTPDEPS_CFLAGS $PTHREAD_CFLAGS $ORTP_DEFS $SRTP_CFLAGS"
CFLAGS="$CFLAGS $ORTP_DEFS"
echo "$ORTPDEPS_CFLAGS" > ortp.defs


dnl ##################################################
dnl # Check for doxygen
dnl ##################################################

AC_PATH_PROG(DOXYGEN,doxygen,false)
AM_CONDITIONAL(HAVE_DOXYGEN, test $DOXYGEN != false)


dnl ##################################################
dnl # Check for ESP Packager
dnl ##################################################

AC_PATH_PROG(EPM,epm,false)
AC_PATH_PROG(MKEPMLIST,mkepmlist,false)
AC_PATH_PROG(EPMINSTALL,epminstall,false)
AM_CONDITIONAL(WITH_EPM,test $EPM != false && test $MKEPMLIST != false && test $EPMINSTALL != false)
AM_CONDITIONAL(BUILD_TESTS,test x$build_tests != xno)

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
SUMMARY="An LGPL implementation of RTP - RFC3550"
AC_SUBST(SUMMARY)
PACKAGER=anonymous
AC_SUBST(PACKAGER)
LICENSE=LGPL
AC_SUBST(LICENSE)
VENDOR=Linphone
AC_SUBST(VENDOR)
RELEASE=1
AC_SUBST(RELEASE)

AC_SUBST(ORTPDEPS_CFLAGS)
AC_SUBST(ORTPDEPS_LIBS)
AC_SUBST(ORTPDEPS_LDFLAGS)

AC_OUTPUT(
Makefile
include/Makefile
include/ortp/Makefile
src/Makefile
src/tests/Makefile
src/tests/win_receiver/Makefile
src/tests/win_sender/Makefile
build/Makefile
build/win32native/Makefile
build/wince/Makefile
ortp.pc
ortp.spec
ortp.doxygen
)

AC_MSG_WARN([ortp-0.15.0 breaks compatibility with older releases, because value returned by rtp_session_recvm_with_ts() has
	changed. See the API documentation in doc/ for further readings.])
