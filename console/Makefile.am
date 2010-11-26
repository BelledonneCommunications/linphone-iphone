## Process this file with automake to produce Makefile.in

COMMON_CFLAGS=$(STRICT_OPTIONS) -DIN_LINPHONE -DENABLE_TRACE -D_ORTP_SOURCE $(VIDEO_CFLAGS) $(READLINE_CFLAGS) $(OSIP_CFLAGS)

if BUILD_CONSOLE

INCLUDES = \
	-I$(top_srcdir)\
	-I$(top_srcdir)/coreapi\
	$(ORTP_CFLAGS) \
	-I$(top_srcdir)/exosip \
	$(MEDIASTREAMER_CFLAGS)

bin_PROGRAMS =  linphonec linphonecsh

if BUILD_WIN32
bin_PROGRAMS += linphoned
endif

linphonec_SOURCES = linphonec.c linphonec.h commands.c
linphonec_CFLAGS=$(COMMON_CFLAGS) $(CONSOLE_FLAGS)
linphonec_LDADD = $(top_builddir)/coreapi/liblinphone.la $(READLINE_LIBS)  \
		$(MEDIASTREAMER_LIBS) \
		$(ORTP_LIBS) \
		$(SPEEX_LIBS) \
		$(OSIP_LIBS)

if BUILD_WIN32
#special build of linphonec to detach from the windows console
linphoned_SOURCES = $(linphonec_SOURCES)
linphoned_CFLAGS=$(COMMON_CFLAGS) $(GUI_FLAGS)
linphoned_LDADD=$(linphonec_LDADD)
endif


sipomatic_SOURCES=\
	sipomatic.c sipomatic.h
sipomatic_CFLAGS= $(COMMON_CFLAGS) $(CONSOLE_FLAGS)

sipomatic_LDADD= 	$(INTLLIBS)  \
			$(top_builddir)/coreapi/liblinphone.la \
			$(MEDIASTREAMER_LIBS) \
			$(ORTP_LIBS) \
			$(SPEEX_LIBS) \
			$(OSIP_LIBS)

linphonecsh_SOURCES = shell.c
linphonecsh_CFLAGS = $(CONSOLE_FLAGS)
linphonecsh_LDADD = $(ORTP_LIBS)

endif


