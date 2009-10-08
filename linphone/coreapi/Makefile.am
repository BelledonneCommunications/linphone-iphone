

## Process this file with automake to produce Makefile.in
linphone_includedir=$(includedir)/linphone

linphone_include_HEADERS=linphonecore.h ../config.h lpconfig.h sipsetup.h

INCLUDES = \
	-I$(top_srcdir)\
	-I$(top_srcdir)/mediastreamer2/include


lib_LTLIBRARIES=liblinphone.la

liblinphone_la_SOURCES=\
	linphonecore.c linphonecore.h private.h\
	exevents.c exevents.h \
	misc.c  \
	enum.c enum.h \
	sdphandler.c sdphandler.h \
	presence.c \
	proxy.c \
	friend.c \
	authentication.c \
	lpconfig.c lpconfig.h \
	chat.c \
        general_state.c \
	sipsetup.c sipsetup.h \
	siplogin.c


liblinphone_la_LDFLAGS= -version-info $(LIBLINPHONE_SO_VERSION) -no-undefined

liblinphone_la_LIBADD= \
		$(EXOSIP_LIBS) \
		$(top_builddir)/mediastreamer2/src/libmediastreamer.la \
		$(ORTP_LIBS)

if BUILD_WIN32
liblinphone_la_LIBADD+=$(top_builddir)/oRTP/src/libortp.la
endif


AM_CFLAGS=$(STRICT_OPTIONS)  -DIN_LINPHONE \
	$(OSIP_CFLAGS) \
	$(EXOSIP_CFLAGS) \
	$(ORTP_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	 $(IPV6_CFLAGS) \
	 -DORTP_INET6 \
	 $(VIDEO_CFLAGS) 
