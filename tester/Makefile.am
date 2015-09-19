EXTRA_DIST=	tester_hosts sounds images certificates rcfiles


if BUILD_CUNIT_TESTS

# there are 2 targets: liblinphonetester.la and the executable liblinphone_tester

liblinphonedir = $(includedir)/linphone
liblinphone_HEADERS = liblinphone_tester.h

lib_LTLIBRARIES = liblinphonetester.la

liblinphonetester_la_SOURCES = \
				accountmanager.c \
				call_tester.c \
				dtmf_tester.c \
				eventapi_tester.c \
				flexisip_tester.c \
				log_collection_tester.c \
				message_tester.c \
				multi_call_tester.c \
				multicast_call_tester.c \
				offeranswer_tester.c \
				player_tester.c \
				presence_tester.c \
				proxy_config_tester.c \
				quality_reporting_tester.c \
				register_tester.c \
				remote_provisioning_tester.c \
				setup_tester.c \
				stun_tester.c \
				tunnel_tester.c \
				tester.c \
				upnp_tester.c \
				video_tester.c \
				common/bc_tester_utils.c \ 
				complex_sip_call.c

liblinphonetester_ladir = $(includedir)/linphone
liblinphonetester_la_HEADERS = common/bc_tester_utils.h

liblinphonetester_la_LDFLAGS= -no-undefined
liblinphonetester_la_LIBADD= ../coreapi/liblinphone.la $(CUNIT_LIBS)

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/coreapi -I$(top_srcdir)/tester/common
AM_CFLAGS    = -DBC_CONFIG_FILE=\"config.h\" $(STRICT_OPTIONS) $(STRICT_OPTIONS_CC) \
		-DIN_LINPHONE $(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS) $(CUNIT_CFLAGS) \
		$(BELLESIP_CFLAGS) $(LIBXML2_CFLAGS) $(SQLITE3_CFLAGS)

if BUILD_GTK_UI

liblinphonetester_la_LIBADD += $(LIBGTK_LIBS) $(LIBGTKMAC_LIBS)
AM_CFLAGS += $(LIBGTK_CFLAGS) $(LIBGTKMAC_CFLAGS) -DHAVE_GTK

endif

if !BUILD_IOS

noinst_PROGRAMS = liblinphone_tester

liblinphone_tester_SOURCES = liblinphone_tester.c
liblinphone_tester_LDADD   = $(top_builddir)/coreapi/liblinphone.la liblinphonetester.la -lm

endif


test: liblinphone_tester
	./liblinphone_tester  --config $(abs_srcdir) $(TEST_OPTIONS)

else !BUILD_CUNIT_TESTS

test:
	@echo "CUnit must be installed to be able to run the tests!"

endif !BUILD_CUNIT_TESTS
