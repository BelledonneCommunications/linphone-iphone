AC_DEFUN([LP_CHECK_ILBC],[

AC_ARG_WITH( ilbc,
      [  --with-ilbc      Set prefix where ilbc headers and libs can be found (ex:/usr, /usr/local, none to disable ilbc support) [default=/usr] ],
      [ ilbc_prefix=${withval}],[ ilbc_prefix="/usr" ])

if test "$ilbc_prefix" = "none" ; then
	AC_MSG_NOTICE([iLBC codec support disabled. ])
else
	ILBC_CFLAGS=" -I${ilbc_prefix}/include/ilbc"
	ILBC_LIBS="-L${ilbc_prefix}/lib -lilbc -lm"
	CPPFLAGS_save=$CPPFLAGS
	CPPFLAGS=$ILBC_CFLAGS
	LDFLAGS_save=$LDFLAGS
	LDFLAGS=$ILBC_LIBS
	AC_CHECK_HEADERS(iLBC_decode.h,[AC_CHECK_LIB(ilbc,iLBC_decode,ilbc_found=yes,ilbc_found=no)
	],ilbc_found=no)
	
	CPPFLAGS=$CPPFLAGS_save
	LDFLAGS=$LDFLAGS_save
	
	if test "$ilbc_found" = "no" ; then
		AC_MSG_WARN([Could not find ilbc headers or libs. Please install ilbc package from http://www.linphone.org if you want iLBC codec support in linphone.])
		ILBC_CFLAGS=
		ILBC_LIBS=
	else
		AC_DEFINE(HAVE_ILBC,1,[Defined when we have ilbc codec lib])
		AC_SUBST(ILBC_CFLAGS)
		AC_SUBST(ILBC_LIBS)
	fi	
fi

])
