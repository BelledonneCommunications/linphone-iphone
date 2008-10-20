
EXTRA_DIST= winsnd2.c winsnd.c winvideo.c  \
	winvideods.c wincevideods.c dxfilter.h dxfilter.cpp \
	msfileplayer_win.c msfilerec_win.c winsndds.cpp nowebcamCIF.jpg winsnd3.c vfw-missing.h \
	winvideo2.c

BUILT_SOURCES=alldescs.h

CLEANFILES=alldescs.h filterdescs.txt

INCLUDES=-I$(top_srcdir)/include/

lib_LTLIBRARIES=libmediastreamer.la

libmediastreamer_la_SOURCES=	mscommon.c     \
				msfilter.c     \
				msqueue.c      \
				msticker.c     \
				alaw.c 	       \
				ulaw.c         \
				mssndcard.c    \
				msfileplayer.c \
				msrtp.c	       \
				dtmfgen.c      \
				msfilerec.c    \
				ice.c          \
				tee.c          \
				msconf.c       \
				msjoin.c       \
				g711common.h \
				msvolume.c \
				mswebcam.c \
				mtu.c


libmediastreamer_la_SOURCES+=audiostream.c

if BUILD_RESAMPLE
libmediastreamer_la_SOURCES+=msresample.c
endif

if BUILD_ALSA
libmediastreamer_la_SOURCES+=alsa.c
endif

if BUILD_OSS
libmediastreamer_la_SOURCES+=oss.c
endif

if BUILD_ARTS
libmediastreamer_la_SOURCES+=arts.c
endif

if BUILD_PORTAUDIO
libmediastreamer_la_SOURCES+=pasnd.c
endif

if BUILD_MACSND
libmediastreamer_la_SOURCES+=macsnd.c
endif

if BUILD_MACAQSND
libmediastreamer_la_SOURCES+=aqsnd.c
endif


if BUILD_VIDEO

if BUILD_MACOSX
libmediastreamer_la_SOURCES+=msv4m.c
else
libmediastreamer_la_SOURCES+=msv4l.c msv4l2.c
endif

libmediastreamer_la_SOURCES+=	sdlout.c   \
				videoenc.c \
				videodec.c \
				pixconv.c  \
				sizeconv.c \
				rfc2429.h \
				nowebcam.c nowebcam.h \
				videoout.c \
				msvideo.c \
				rfc3984.c \
				mire.c \
				swscale.h ffmpeg-priv.h

libmediastreamer_la_SOURCES+=videostream.c

endif

if BUILD_THEORA
libmediastreamer_la_SOURCES+=theora.c
endif

if BUILD_SPEEX
libmediastreamer_la_SOURCES+=msspeex.c speexec.c
endif

if BUILD_GSM
libmediastreamer_la_SOURCES+=gsm.c
endif

alldescs.h:	Makefile.am $(libmediastreamer_la_SOURCES)
	builddir=`pwd` && cd $(srcdir) && \
	awk 'BEGIN { FS="[()]" ; }; /^\t*MS_FILTER_DESC_EXPORT/{ printf("%s\n", $$2) } '  > $$builddir/filterdescs.txt $(libmediastreamer_la_SOURCES) && \
	awk 'BEGIN { print("#include \"mediastreamer2/msfilter.h\"\n") } { printf("extern MSFilterDesc %s;\n",$$1) } ' $$builddir/filterdescs.txt > $$builddir/$@ && \
	awk 'BEGIN { print("MSFilterDesc * ms_filter_descs[]={") } { printf("&%s,\n",$$1) } END{ print("NULL\n};\n") } ' $$builddir/filterdescs.txt >> $$builddir/$@


libmediastreamer_la_LIBADD=	$(ORTP_LIBS) \
				$(PORTAUDIO_LIBS) \
				$(ALSA_LIBS) \
				$(ARTS_LIBS) \
				$(SPEEX_LIBS) \
				$(GSM_LIBS) \
				$(THEORA_LIBS)

libmediastreamer_la_LDFLAGS=-rdynamic

if BUILD_VIDEO
libmediastreamer_la_LIBADD+=$(VIDEO_LIBS)
endif


AM_CFLAGS=	-I$(top_srcdir)  \
                $(ORTP_CFLAGS)   \
		$(SPEEX_CFLAGS)  \
		$(GSM_CFLAGS)    \
		$(STRICT_OPTIONS)

if BUILD_VIDEO
AM_CFLAGS+=$(VIDEO_CFLAGS)
endif

imgdir=$(datadir)/images/

img_DATA=nowebcamCIF.jpg

