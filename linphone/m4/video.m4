AC_DEFUN([LP_CHECK_DEP],[
	dnl $1=dependency description
	dnl $2=dependency short name, will be suffixed with _CFLAGS and _LIBS
	dnl $3=headers's place
	dnl $4=lib's place
	dnl $5=header to check
	dnl $6=lib to check
	dnl $7=function to check in library
	
	NAME=$2
	dep_headersdir=$3
	dep_libsdir=$4
	dep_header=$5
	dep_lib=$6
	dep_funclib=$7
	other_libs=$8	
	
	if test "$dep_headersdir" != "/usr/include" ; then
		eval ${NAME}_CFLAGS=\"-I$dep_headersdir \"
	fi
	eval ${NAME}_LIBS=\"-L$dep_libsdir -l$dep_lib\"
	
	CPPFLAGS_save=$CPPFLAGS
	LDFLAGS_save=$LDFLAGS
	CPPFLAGS="-I$dep_headersdir "
	LDFLAGS="-L$dep_libsdir "
	
	AC_CHECK_HEADERS([$dep_header],[AC_CHECK_LIB([$dep_lib],[$dep_funclib],found=yes,found=no, [$other_libs])
	],found=no)
	
	if test "$found" = "yes" ; then
		eval ${NAME}_found=yes
		AC_DEFINE([HAVE_${NAME}],1,[Defined when we have found $1])
		AC_SUBST(${NAME}_CFLAGS)
		AC_SUBST(${NAME}_LIBS)
	else
		eval ${NAME}_found=no
		eval ${NAME}_CFLAGS=
		eval ${NAME}_LIBS=
	fi
	CPPFLAGS=$CPPFLAGS_save
	LDFLAGS=$LDFLAGS_save
	
])


AC_DEFUN([LP_CHECK_VIDEO],[

	dnl conditionnal build of video support
	AC_ARG_ENABLE(video,
		  [  --enable-video    Turn on video support compiling: not functionnal for the moment],
		  [case "${enableval}" in
			yes) video=true ;;
			no)  video=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-video) ;;
		  esac],[video=false])
		  
	AC_ARG_WITH( ffmpeg,
		  [  --with-ffmpeg		Sets the installation prefix of ffmpeg, needed for video support. [default=/usr] ],
		  [ ffmpegdir=${withval}],[ ffmpegdir=/usr ])
	
	AC_ARG_WITH( sdl,
		  [  --with-sdl		Sets the installation prefix of libSDL, needed for video support. [default=/usr] ],
		  [ libsdldir=${withval}],[ libsdldir=/usr ])
	
	if test "$video" = "true"; then
		
		dnl test for ffmpeg presence
		dnl LP_CHECK_DEP([ffmpeg],[FFMPEG],[${ffmpegdir}/include/ffmpeg],[${ffmpegdir}/lib],[avcodec.h],[avcodec],[avcodec_init], [-lavutils -lm])
		dnl if test "$FFMPEG_found" = "no" ; then
		dnl	AC_MSG_ERROR([Could not find ffmpeg headers and library. This is mandatory for video support])
		dnl fi
		PKG_CHECK_MODULES(FFMPEG, [libavcodec >= 50.0.0 ], , [ AC_MSG_ERROR([Could not find ffmpeg headers and library. This is mandatory for video support]) ])
		
		LP_CHECK_DEP([SDL],[SDL],[${libsdldir}/include],[${libsdldir}/lib],[SDL/SDL.h],[SDL],[SDL_Init])
		if test "$SDL_found" = "no" ; then
			AC_MSG_ERROR([Could not find libsdl headers and library. This is mandatory for video support])
		fi
	
		VIDEO_CFLAGS=" $FFMPEG_CFLAGS $SDL_CFLAGS"
		VIDEO_LIBS=" $FFMPEG_LIBS $SDL_LIBS"
		
		AC_DEFINE(VIDEO_ENABLED,1,[Set when video support is enabled])
		
	fi
	
	AC_SUBST(VIDEO_CFLAGS)
	AC_SUBST(VIDEO_LIBS)
])
