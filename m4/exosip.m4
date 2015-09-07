dnl -*- autoconf -*-
AC_DEFUN([LP_SETUP_EXOSIP],[
AC_REQUIRE([AC_CANONICAL_HOST])
AC_REQUIRE([LP_CHECK_OSIP2])


case $host_alias in
	i386-apple*|armv6-apple*|armv7-apple*|armv7s-apple*)
		OSIP_LIBS="$OSIP_LIBS  -framework CoreFoundation -framework CFNetwork -lresolv"
	;;
	x86_64-apple*)
		OSIP_LIBS="$OSIP_LIBS  -framework CoreFoundation"
	;;
esac

dnl eXosip embeded stuff
EXOSIP_CFLAGS="$OSIP_CFLAGS -DOSIP_MT "
EXOSIP_LIBS="$OSIP_LIBS -leXosip2  "

CPPFLAGS_save=$CPPFLAGS
CPPFLAGS="$OSIP_CFLAGS $CPPFLAGS"
AC_CHECK_HEADER([eXosip2/eXosip.h], ,AC_MSG_ERROR([Could not find eXosip2 headers !]))

dnl check exosip support of DSCP in exosip
AC_MSG_CHECKING([for DSCP support in exosip])
AC_TRY_COMPILE([#include <eXosip2/eXosip.h>],
	[int dscp=0;eXosip_set_option(EXOSIP_OPT_SET_DSCP,&dscp);],
	has_exosip_dscp=yes,
	has_exosip_dscp=no
)
AC_MSG_RESULT($has_exosip_dscp)
if test "$has_exosip_dscp" = "yes" ; then
	AC_DEFINE( HAVE_EXOSIP_DSCP, 1, [Define if exosip dscp available] )
fi

CPPFLAGS=$CPPFLAGS_save



dnl check for eXosip2 libs
LDFLAGS_save=$LDFLAGS
LDFLAGS="$OSIP_LIBS $LDFLAGS $OPENSSL_LIBS"
LIBS_save=$LIBS
AC_CHECK_LIB([eXosip2],[eXosip_set_tls_ctx],
	[],
	[AC_MSG_ERROR([Could not find eXosip2 library with version >= 3.5.0 !])],
	[-losipparser2 -losip2 ])
AC_CHECK_LIB([eXosip2],[eXosip_get_version],
	[AC_DEFINE([HAVE_EXOSIP_GET_VERSION],[1],[Defined when eXosip_get_version is available])],
	[],
	[-losipparser2 -losip2 ])
AC_CHECK_LIB([eXosip2],[eXosip_tls_verify_certificate],
	[AC_DEFINE([HAVE_EXOSIP_TLS_VERIFY_CERTIFICATE],[1],[Defined when eXosip_tls_verify_certificate is available])],
	[AC_MSG_WARN([Could not find eXosip_tls_verify_certificate in eXosip2 !])],
	[-losipparser2 -losip2 ])
AC_CHECK_LIB([eXosip2],[eXosip_tls_verify_cn],
	[AC_DEFINE([HAVE_EXOSIP_TLS_VERIFY_CN],[1],[Defined when eXosip_tls_verify_certificate is available])],
	[AC_MSG_WARN([Could not find eXosip_tls_verify_cn in eXosip2 !])],
	[-losipparser2 -losip2 ])
AC_CHECK_LIB([eXosip2],[eXosip_trylock],
	[AC_DEFINE([HAVE_EXOSIP_TRYLOCK],[1],[Defined when eXosip_get_socket is available])],
	[],
	[-losipparser2 -losip2 ])
AC_CHECK_LIB([eXosip2],[eXosip_reset_transports],
	[AC_DEFINE([HAVE_EXOSIP_RESET_TRANSPORTS],[1],[Defined when eXosip_reset_transports is available])],
	[],
	[-losipparser2 -losip2 ])
dnl AC_CHECK_LIB([eXosip2],[eXosip_get_naptr],
dnl	[AC_DEFINE([HAVE_EXOSIP_NAPTR_SUPPORT],[1],[Defined when eXosip_get_naptr is available])],
dnl	[],
dnl	[-losipparser2 -losip2 ])
LIBS=$LIBS_save
LDFLAGS=$LDFLAGS_save

AC_SUBST(EXOSIP_CFLAGS)
AC_SUBST(EXOSIP_LIBS)
])
