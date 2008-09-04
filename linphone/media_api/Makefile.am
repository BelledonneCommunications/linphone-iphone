## Process this file with automake to produce Makefile.in
if BUILD_MEDIA_API

#the media_api library is the only one we have to build here
lib_LTLIBRARIES=libmedia_api.la

#definition of the sources of libmedia_api
libmedia_api_la_SOURCES=	basiccall.c callmember.c mediaflow.c

# libmedia_api  needs libmediastreamer
libmedia_api_la_LIBADD=$(top_srcdir)/mediastreamer/libmediastreamer.la

#the media_api test program
bin_PROGRAMS=apitest

apitest_SOURCES= apitest.c
# the test program links to libmedia_api
apitest_LDADD=libmedia_api.la

endif

DEFS=@DEFS@ @SOUNDDEFS@ -DDEBUG -DG_LOG_DOMAIN=\"MediaApi\" 

INCLUDES=-I$(top_srcdir)/mediastreamer \
		-I$(top_srcdir)/speex \
		-I$(top_srcdir)/gsmlib \
		$(ORTP_CFLAGS) \
		-I$(top_srcdir)/lpc10-1.5 \
		-I$(top_srcdir)/ffmpeg 


