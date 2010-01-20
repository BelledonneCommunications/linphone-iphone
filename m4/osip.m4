dnl -*- autoconf -*-
AC_DEFUN([LP_CHECK_OSIP2],[

AC_ARG_WITH( osip,
      [  --with-osip      Set prefix where osip can be found (ex:/usr or /usr/local)[default=/usr] ],
      [ osip_prefix=${withval}],[ osip_prefix=/usr ])

dnl FC6's compat-osip2 headers live into "/usr/include/osip2-2.2.2",
dnl while  libraries are "/usr/lib/libosipparser2-2.2.2.so*".
AC_ARG_WITH( osip-version,dnl
AC_HELP_STRING([--with-osip-version=VERSION],
	[Set osip legacy version @<:@empty@:>@]),
	[osip_legacy_version="-${withval}"],
	[osip_legacy_version=""])

osip_pkgconfig=true
osip_pkgconfig_file=libosip2
if test -n "$osip_legacy_version"; then
	osip_pkgconfig_file=libosip2${osip_legacy_version}
fi

PKG_CHECK_MODULES(OSIP, $osip_pkgconfig_file >= 3.0.0, ,osip_pkgconfig=false)

if test $osip_pkgconfig = false; then

	if test -z "$osip_legacy_version"; then
		OSIP_CFLAGS="-I${osip_prefix}/include"
	else
		OSIP_CFLAGS="-I${osip_prefix}/include/osip2${osip_legacy_version}"
	fi

dnl check osip2 headers
	CPPFLAGS_save=$CPPFLAGS
	CPPFLAGS=$OSIP_CFLAGS
	AC_CHECK_HEADER([osip2/osip.h], ,AC_MSG_ERROR([Could not find osip2 headers !]))
	CPPFLAGS=$CPPFLAGS_save

dnl check for osip2 libs
	OSIP_LIBS="-L$osip_prefix/lib -losipparser2${osip_legacy_version}"
	LDFLAGS_save=$LDFLAGS
	LDFLAGS=$OSIP_LIBS
	LIBS_save=$LIBS
	case "$target_os" in
		*mingw*)
			osip_aux_libs=
			;;
		*)
			osip_aux_libs=-lpthread
			;;
	esac
	OSIP_LIBS="$OSIP_LIBS $osip_aux_libs"
	AC_CHECK_LIB(osip2${osip_legacy_version},osip_init, , AC_MSG_ERROR([Could not find osip2 library !]),[-losipparser2${osip_legacy_version} $osip_aux_libs ])
	AC_CHECK_LIB(osipparser2${osip_legacy_version},osip_message_init, , AC_MSG_ERROR([Could not find osipparser2 library !]),[$osip_aux_libs])
	LDFLAGS=$LDFLAGS_save
	LIBS=$LIBS_save
fi

AC_SUBST(OSIP_CFLAGS)
AC_SUBST(OSIP_LIBS)

])
