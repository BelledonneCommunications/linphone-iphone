## Process this file with automake to produce Makefile.in

if BUILD_CONSOLE

INCLUDES = \
	-I$(top_srcdir)\
	-I$(top_srcdir)/coreapi\
	$(ORTP_CFLAGS) \
	-I$(top_srcdir)/exosip \
	-I$(top_srcdir)/mediastreamer2/include



bin_PROGRAMS =  linphonec sipomatic

linphonec_SOURCES = linphonec.c linphonec.h commands.c

linphonec_LDADD = $(top_builddir)/coreapi/liblinphone.la $(READLINE_LIBS) $(OSIP_LIBS)

sipomatic_SOURCES=\
	sipomatic.c sipomatic.h

sipomatic_LDADD= 	$(INTLLIBS)  \
					$(top_builddir)/coreapi/liblinphone.la \
			$(OSIP_LIBS)

endif

AM_CFLAGS=$(STRICT_OPTIONS)  -DENABLE_TRACE -D_ORTP_SOURCE $(VIDEO_CFLAGS) $(READLINE_CFLAGS) $(OSIP_CFLAGS)



AM_LDFLAGS= 	$(top_builddir)/mediastreamer2/src/libmediastreamer.la \
		$(ORTP_LIBS) \
		$(SPEEX_LIBS) \
		$(OSIP_LIBS)

