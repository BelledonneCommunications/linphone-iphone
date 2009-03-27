noinst_PROGRAMS=echo ring mtudiscover bench

if BUILD_VIDEO
noinst_PROGRAMS+=videodisplay
endif

echo_SOURCES=echo.c
ring_SOURCES=ring.c
videodisplay_SOURCES=videodisplay.c
mtudiscover_SOURCES=mtudiscover.c
bench_SOURCES=bench.c

libexec_PROGRAMS=mediastream

mediastream_SOURCES=mediastream.c

#libquickstream.la would be enough, but to workaround a bug of libtool when 
#cross compiling we need to add all the dependencies.
LDADD=	$(top_builddir)/src/libmediastreamer.la \
	$(ORTP_LIBS) \
	$(ALSA_LIBS) \
	$(ARTS_LIBS) \
	$(SPEEX_LIBS) \
	$(GSM_LIBS) \
	$(THEORA_LIBS) \
	$(VIDEO_LIBS)



INCLUDES=-I$(top_srcdir)/include/

AM_CFLAGS=-I$(top_srcdir) $(ORTP_CFLAGS) $(STRICT_OPTIONS) $(VIDEO_CFLAGS)
AM_LDFLAGS=-rdynamic
