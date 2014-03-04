EXTRA_DIST=	tester_hosts sounds images certificates rcfiles


if BUILD_CUNIT_TESTS

# there are 2 targets: liblinphonetester.la and the executable liblinphone_tester

liblinphonedir = $(includedir)/linphone
liblinphone_HEADERS = liblinphone_tester.h

lib_LTLIBRARIES = liblinphonetester.la

liblinphonetester_la_SOURCES = \
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


AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/coreapi
AM_CFLAGS    = $(STRICT_OPTIONS) -DIN_LINPHONE $(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS) $(CUNIT_CFLAGS) $(BELLESIP_CFLAGS) $(LIBXML2_CFLAGS)
AM_OBJCFLAGS = $(AM_CFLAGS)


if BUILD_IOS

#we build the liblinphonetester as a library, not as an executable, so that it is usable in XCode
liblinphonetester_la_SOURCES += liblinphonetester_ios.m liblinphonetester_ios.h
liblinphonetester_la_LDFLAGS  = -framework CoreFoundation -framework AudioToolbox -framework CoreAudio -framework Foundation -framework QuartzCore -framework OpenGLES -framework UIKit -framework AVFoundation $(CUNIT_LIBS)

else

noinst_PROGRAMS = liblinphone_tester

liblinphone_tester_LDFLAGS = $(CUNIT_LIBS)
liblinphone_tester_SOURCES = liblinphone_tester.c 
liblinphone_tester_LDADD   = $(top_builddir)/coreapi/liblinphone.la $(BELLESIP_LIBS) $(LIBXML2_LIBS) liblinphonetester.la

endif


test: liblinphone_tester
	./liblinphone_tester  --config $(abs_srcdir) 

else !BUILD_CUNIT_TESTS

test:
	@echo "CUnit must be installed to be able to run the tests!"

endif !BUILD_CUNIT_TESTS
