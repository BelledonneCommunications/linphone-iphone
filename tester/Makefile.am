EXTRA_DIST=	empty_rc laure_rc marie_early_rc marie_no_sdp_rc marie_rc multi_account_lrc pauline_alt_rc \
		marie_remote_rc pauline_remote_rc marie_remote_404_rc marie_remote_invalid_rc \
		pauline_rc pauline_wild_rc pauline_tcp_rc tester_hosts sounds images certificates


if BUILD_CUNIT_TESTS

noinst_PROGRAMS=liblinphone_tester  

liblinphone_tester_SOURCES=	liblinphone_tester.c liblinphone_tester.h\
				setup_tester.c \
				register_tester.c \
				message_tester.c \
				call_tester.c \
				presence_tester.c \
				upnp_tester.c \
				eventapi_tester.c \
				flexisip_tester.c \
				stun_tester.c \
				remote_provisioning_tester.c


#liblinphone_tester_CFLAGS=$(CUNIT_CFLAGS)

#liblinphone_tester_LDFLAGS=$(CUNIT_LIBS)


AM_CPPFLAGS=-I$(top_srcdir)/include -I$(top_srcdir)/coreapi

LDADD=$(top_builddir)/coreapi/liblinphone.la $(BELLESIP_LIBS) $(LIBXML2_LIBS)

AM_LDFLAGS=$(CUNIT_LIBS)

AM_CFLAGS=$(STRICT_OPTIONS)  -DIN_LINPHONE $(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS) $(CUNIT_CFLAGS) $(BELLESIP_CFLAGS) $(LIBXML2_CFLAGS)

test: liblinphone_tester
	./liblinphone_tester  --config $(abs_srcdir) 

else

test:
	@echo "CUnit must be installed to be able to run the tests!"

endif
