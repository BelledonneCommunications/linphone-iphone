AC_DEFUN([LP_CHECK_ORTP],[

ortp_pkgconfig=true

PKG_CHECK_MODULES([ORTP], [ortp], , [ortp_pkgconfig=false])

if test $ortp_pkgconfig = false; then
	AC_CHECK_HEADER([ortp/ortp.h], ,AC_MSG_ERROR([Could not find oRTP headers !]))
	LIBS_save=$LIBS
	AC_CHECK_LIB([ortp], [ortp_init], , AC_MSG_ERROR([Could not find oRTP library]))
	ORTP_LIBS='-lortp'
	LIBS=$LIBS_save
fi
AC_SUBST([ORTP_LIBS])
AC_SUBST([ORTP_CFLAGS])
])
