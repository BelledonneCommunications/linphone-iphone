##################################################
# Check for readline support.
##################################################

AC_DEFUN([LP_CHECK_READLINE],[

AC_ARG_WITH( readline,
      [  --with-readline      Set prefix where gnu readline headers and libs can be found (ex:/usr, /usr/local, none) [default=/usr] ],
      [ readline_prefix=${withval}],[ readline_prefix="/usr" ])

if test "$readline_prefix" != "none"; then

	if test "$readline_prefix" != "/usr"; then
		READLINE_CFLAGS="-I$readline_prefix/include"
		READLINE_LIBS="-L$readline_prefix/lib"
	fi
	
	CPPFLAGS_save=$CPPFLAGS
	LIBS_save=$LIBS
	CPPFLAGS="$CPPFLAGS $READLINE_CFLAGS"
	LIBS="$LIBS $READLINE_LIBS"
	AC_CHECK_HEADERS(readline.h readline/readline.h, readline_h_found=yes)
	AC_CHECK_HEADERS(history.h readline/history.h)
	
	for termcap_lib in "" -ltermcap -lcurses -lncurses; do
		unset ac_cv_lib_readline_readline
		AC_CHECK_LIB(readline, readline, [readline_libs_found=yes],[],[$termcap_lib])
		if test "x$readline_libs_found" = "xyes" ; then
			READLINE_LIBS="$READLINE_LIBS -lreadline $termcap_lib"
			break
		fi
	done

	LIBS=$LIBS_save
	CPPFLAGS=$CPPFLAGS_save
	
	if test "$readline_libs_found$readline_h_found" != "yesyes" ; then
		AC_MSG_WARN([Could not find libreadline headers or library, linphonec will have limited prompt features])
	else
		AC_DEFINE([HAVE_READLINE],1,[defined when compiling with readline support])
	fi
	
	
	AC_SUBST(READLINE_CFLAGS)
	AC_SUBST(READLINE_LIBS)

else
	AC_MSG_NOTICE([Readline support disabled.])
fi

])
