## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=\
	-I$(top_srcdir) \
	-I$(top_srcdir)/coreapi \
	-I$(top_srcdir)/exosip

COMMON_CFLAGS=\
	-DIN_LINPHONE \
	$(LIBXML2_CFLAGS) \
	$(ORTP_CFLAGS) \
	$(STRICT_OPTIONS)

if BUILD_TOOLS

lib_LTLIBRARIES=libxml2lpc.la

libxml2lpc_la_SOURCES=\
	xml2lpc.c \
	xml2lpc.h

libxml2lpc_la_CFLAGS=$(COMMON_CFLAGS)
libxml2lpc_la_LIBADD=\
	$(LIBXML2_LIBS) \
	$(top_builddir)/coreapi/liblinphone.la

libxml2lpc_la_LDFLAGS=-no-undefined

bin_PROGRAMS=xml2lpc_test

xml2lpc_test_SOURCES=\
	xml2lpc_test.c

xml2lpc_test_CFLAGS=$(COMMON_CFLAGS)
xml2lpc_test_LDADD=\
	libxml2lpc.la

endif


