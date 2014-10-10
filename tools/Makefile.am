## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=\
	-I$(top_srcdir) \
	-I$(top_srcdir)/coreapi

COMMON_CFLAGS=\
	-DIN_LINPHONE \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(STRICT_OPTIONS) \
	$(STRICT_OPTIONS_CC) \
	$(LIBXML2_CFLAGS)

#-fpermissive to workaround a g++ bug on macos 32bit SDK.
AM_CXXFLAGS=$(LIBXML2_CFLAGS) -fpermissive $(STRICT_OPTIONS) $(STRICT_OPTIONS_CXX)

EXTRA_DIST=xml2lpc_jni.cc lpc2xml_jni.cc

if BUILD_TOOLS

bin_PROGRAMS=xml2lpc_test lpc2xml_test lp-gen-wrappers

xml2lpc_test_SOURCES=\
	xml2lpc_test.c

lpc2xml_test_SOURCES=\
	lpc2xml_test.c

xml2lpc_test_CFLAGS=$(COMMON_CFLAGS)
xml2lpc_test_LDADD=\
	$(top_builddir)/coreapi/liblinphone.la

lpc2xml_test_CFLAGS=$(COMMON_CFLAGS)
lpc2xml_test_LDADD=\
	$(top_builddir)/coreapi/liblinphone.la

lp_gen_wrappers_SOURCES=genwrappers.cc \
			software-desc.cc software-desc.hh \
			generator.cc generator.hh

lp_gen_wrappers_LDADD= \
		$(LIBXML2_LIBS)

endif


