## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=\
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/coreapi

COMMON_CFLAGS=\
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(STRICT_OPTIONS) \
	$(STRICT_OPTIONS_CC) \
	$(MEDIASTREAMER_CFLAGS) \
	$(BCTOOLBOX_CFLAGS)

#-fpermissive to workaround a g++ bug on macos 32bit SDK.
AM_CXXFLAGS=$(LIBXML2_CFLAGS) -fpermissive $(STRICT_OPTIONS) $(STRICT_OPTIONS_CXX)

EXTRA_DIST=xml2lpc_jni.cc lpc2xml_jni.cc

if BUILD_TOOLS

bin_PROGRAMS=xml2lpc_test lpc2xml_test lp-gen-wrappers lp-autoanswer lp-test-ecc lp-sendmsg

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

lp_autoanswer_SOURCES=auto_answer.c
lp_autoanswer_CFLAGS=$(COMMON_CFLAGS)
lp_autoanswer_LDADD=\
	$(top_builddir)/coreapi/liblinphone.la \
	$(MEDIASTREAMER_LIBS)

lp_test_ecc_SOURCES=test_ecc.c
lp_test_ecc_CFLAGS=$(COMMON_CFLAGS)
lp_test_ecc_LDADD=$(top_builddir)/coreapi/liblinphone.la

lp_sendmsg_SOURCES=lpsendmsg.c
lp_sendmsg_CFLAGS=$(COMMON_CFLAGS)
lp_sendmsg_LDADD=$(top_builddir)/coreapi/liblinphone.la

endif



if ENABLE_TESTS

noinst_PROGRAMS=test_lsd  test_numbers

test_lsd_SOURCES=test_lsd.c
test_lsd_CFLAGS=$(COMMON_CFLAGS)
test_lsd_LDADD=$(top_builddir)/coreapi/liblinphone.la



test_numbers_SOURCES=test_numbers.c
test_numbers_CFLAGS=$(COMMON_CFLAGS)
test_numbers_LDADD=$(top_builddir)/coreapi/liblinphone.la

endif
