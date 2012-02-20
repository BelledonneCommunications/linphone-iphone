
SUBDIRS=. help

EXTRA_DIST=linphonecore_jni.cc

## Process this file with automake to produce Makefile.in
linphone_includedir=$(includedir)/linphone

linphone_include_HEADERS=linphonecore.h linphonefriend.h linphonecore_utils.h ../config.h lpconfig.h sipsetup.h 

if BUILD_TUNNEL
linphone_include_HEADERS+=linphone_tunnel.h
endif

INCLUDES = \
	-I$(top_srcdir)


lib_LTLIBRARIES=liblinphone.la

liblinphone_la_SOURCES=\
	linphonecore.c linphonecore.h private.h\
	offeranswer.c offeranswer.h\
	sal.c sal.h \
	sal_eXosip2.c sal_eXosip2.h\
	sal_eXosip2_sdp.c \
	sal_eXosip2_presence.c \
	callbacks.c \
	misc.c  \
	address.c \
	enum.c enum.h \
	presence.c \
	proxy.c \
	friend.c \
	authentication.c \
	lpconfig.c lpconfig.h \
	chat.c \
	linphonecall.c \
	sipsetup.c sipsetup.h \
	siplogin.c \
	lsd.c linphonecore_utils.h \
	ec-calibrator.c \
	conference.c \
	linphone_tunnel.cc
	
if BUILD_WIZARD
liblinphone_la_SOURCES+=sipwizard.c 
endif

if BUILD_TUNNEL
liblinphone_la_SOURCES+=TunnelManager.cc TunnelManager.hh 
endif



liblinphone_la_LDFLAGS= -version-info $(LIBLINPHONE_SO_VERSION) -no-undefined

liblinphone_la_LIBADD= \
		$(EXOSIP_LIBS) \
		$(MEDIASTREAMER_LIBS) \
		$(ORTP_LIBS) $(OPENSSL_LIBS) \
		$(TUNNEL_LIBS) \
		$(LIBSOUP_LIBS)

if BUILD_TESTS
noinst_PROGRAMS=test_lsd test_ecc

test_lsd_SOURCES=test_lsd.c

test_lsd_LDADD=liblinphone.la $(liblinphone_la_LIBADD)

test_ecc_SOURCES=test_ecc.c

test_ecc_LDADD=liblinphone.la $(liblinphone_la_LIBADD)
endif


AM_CFLAGS=$(STRICT_OPTIONS)  -DIN_LINPHONE \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(OSIP_CFLAGS) \
	$(EXOSIP_CFLAGS) \
	$(LIBSOUP_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	$(IPV6_CFLAGS) \
	-DORTP_INET6 \
	$(VIDEO_CFLAGS) \
	$(TUNNEL_CFLAGS)

if BUILD_WIZARD
AM_CFLAGS+= -DBUILD_WIZARD 
endif

AM_CXXFLAGS=$(AM_CFLAGS)
