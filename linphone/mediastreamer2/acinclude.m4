dnl -*- autoconf -*-
AC_DEFUN([MS_CHECK_DEP],[
	dnl $1=dependency description
	dnl $2=dependency short name, will be suffixed with _CFLAGS and _LIBS
	dnl $3=headers's place
	dnl $4=lib's place
	dnl $5=header to check
	dnl $6=lib to check
	dnl $7=function to check in library
	
	dep_name=$2
	dep_headersdir=$3
	dep_libsdir=$4
	dep_header=$5
	dep_lib=$6
	dep_funclib=$7
	other_libs=$8	
	
	CPPFLAGS_save=$CPPFLAGS
	LDFLAGS_save=$LDFLAGS
	LIBS_save=$LIBS
	CPPFLAGS=`echo "-I$dep_headersdir"|sed -e "s:-I/usr/include[\ ]*$::"`
	LIBS="-l$dep_lib"
	LDFLAGS=`echo "-L$dep_libsdir"|sed -e "s:-L/usr/lib\(64\)*[\ ]*::"`
	
	$2_CFLAGS="$CPPFLAGS"
	$2_LIBS="$LDFLAGS $LIBS"

	AC_CHECK_HEADERS([$dep_header],[AC_CHECK_LIB([$dep_lib],[$dep_funclib],found=yes,found=no, [$other_libs])
	],found=no)
	
	if test "$found" = "yes" ; then
		eval $2_found=yes
	else
		eval $2_found=no
		eval $2_CFLAGS=
		eval $2_LIBS=
	fi
	AC_SUBST($2_CFLAGS)
	AC_SUBST($2_LIBS)
	CPPFLAGS=$CPPFLAGS_save
	LDFLAGS=$LDFLAGS_save
	LIBS=$LIBS_save
])


AC_DEFUN([MS_CHECK_VIDEO],[

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
	
	AC_ARG_WITH( sdl,
		  [  --with-sdl		Sets the installation prefix of libSDL, needed for video support. [default=/usr] ],
		  [ libsdldir=${withval}],[ libsdldir=/usr ])
	
	if test "$video" = "true"; then
		
		dnl test for ffmpeg presence
		PKG_CHECK_MODULES(FFMPEG, [libavcodec >= 51.0.0 ],ffmpeg_found=yes , ffmpeg_found=no)
		if test x$ffmpeg_found = xno ; then
			AC_MSG_ERROR([Could not find libavcodec (from ffmpeg) headers and library. This is mandatory for video support])
		fi
		PKG_CHECK_MODULES(SWSCALE, [libswscale >= 0.7.0 ],swscale_found=yes , swscale_found=no)
		if test x$swscale_found = xno ; then
			AC_MSG_ERROR([Could not find libswscale (from ffmpeg) headers and library. This is mandatory for video support])
		fi

		dnl check for new/old ffmpeg header file layout
		CPPFLAGS_save=$CPPFLAGS
		CPPFLAGS="$FFMPEG_CFLAGS $CPPFLAGS"
		AC_CHECK_HEADERS(libavcodec/avcodec.h)
		CPPFLAGS=$CPPFLAGS_save

		dnl to workaround a bug on debian and ubuntu, check if libavcodec needs -lvorbisenc to compile	
		AC_CHECK_LIB(avcodec,avcodec_register_all, novorbis=yes , [
			LIBS="$LIBS -lvorbisenc"
		], $FFMPEG_LIBS )

		dnl when swscale feature is not provided by
		dnl libswscale, its features are swallowed by
		dnl libavcodec, but without swscale.h and without any
		dnl declaration into avcodec.h (this is to be
		dnl considered as an ffmpeg bug).
		dnl 
		dnl #if defined(HAVE_LIBAVCODEC_AVCODEC_H) && !defined(HAVE_LIBSWSCALE_SWSCALE_H)
		dnl # include "swscale.h" // private linhone swscale.h
		dnl #endif
		CPPFLAGS_save=$CPPFLAGS
		CPPFLAGS="$SWSCALE_CFLAGS $CPPFLAGS"
		AC_CHECK_HEADERS(libswscale/swscale.h)
		CPPFLAGS=$CPPFLAGS_save

		if test "$libsdldir" != "none" ; then
			MS_CHECK_DEP([SDL],[SDL],[${libsdldir}/include],[${libsdldir}/lib],[SDL/SDL.h],[SDL],[SDL_Init])
			if test "$SDL_found" = "no" ; then
				AC_MSG_ERROR([Could not find libsdl headers and library. This is mandatory for video support])
			fi
		fi

		AC_ARG_ENABLE(theora,
		  [  --disable-theora    Disable theora support],
		  [case "${enableval}" in
			yes) theora=true ;;
			no)  theora=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --disable-theora) ;;
		  esac],[theora=true])

		if test x$theora = xtrue; then
		PKG_CHECK_MODULES(THEORA, [theora >= 1.0alpha7 ], [have_theora=yes],
					[have_theora=no])
		fi

		AC_ARG_ENABLE(x11,
		  [  --disable-x11    Disable X11 support],
		  [case "${enableval}" in
			yes) enable_x11=true ;;
			no)  enable_x11=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --disable-x11) ;;
		  esac],[enable_x11=true])

		if test "$enable_x11" = "true"; then
		AC_CHECK_HEADERS(X11/Xlib.h)
		fi
		
		VIDEO_CFLAGS=" $FFMPEG_CFLAGS -DVIDEO_ENABLED"
		VIDEO_LIBS=" $FFMPEG_LIBS $SWSCALE_LIBS"

		if test "$SDL_found" = "yes" ; then
			VIDEO_CFLAGS="$VIDEO_CFLAGS $SDL_CFLAGS -DHAVE_SDL"
			VIDEO_LIBS="$VIDEO_LIBS $SDL_LIBS"
		fi

		if test "${ac_cv_header_X11_Xlib_h}" = "yes" ; then
			VIDEO_LIBS="$VIDEO_LIBS -lX11"
		fi
	fi
	
	AC_SUBST(VIDEO_CFLAGS)
	AC_SUBST(VIDEO_LIBS)
])
