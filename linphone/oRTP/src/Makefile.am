
EXTRA_DIST=ortp-config-win32.h dll_entry.c

AM_CFLAGS=  $(PTHREAD_CFLAGS)  -I$(top_srcdir) $(TRUESPEECH_CFLAGS) $(SRTP_CFLAGS)
AM_LDFLAGS= $(PTHREAD_LDFLAGS)

INCLUDES=-I$(top_srcdir)/include/

lib_LTLIBRARIES = libortp.la

libortp_la_SOURCES=	str_utils.c 	\
			port.c \
			rtpparse.c  \
			rtpsession.c \
			rtpsession_inet.c \
			rtpsession_priv.h \
			jitterctl.c jitterctl.h \
			rtpsignaltable.c  \
			rtptimer.c	rtptimer.h \
			posixtimer.c \
			ortp.c \
			scheduler.c scheduler.h \
			avprofile.c  \
			sessionset.c  \
			telephonyevents.c  \
			payloadtype.c \
			rtcp.c  \
			utils.c utils.h \
			rtcpparse.c \
			event.c \
			stun.c stun_udp.c \
			srtp.c \
			b64.c


libortp_la_LIBADD= $(PTHREAD_LIBS) -lm $(SRTP_LIBS) $(SSL_LIBS)

libortp_la_LDFLAGS= -version-info $(LIBORTP_SO_VERSION) -no-undefined


SUBDIRS= . tests
