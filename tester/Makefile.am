EXTRA_DIST=	tester_hosts sounds images certificates rcfiles


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


AM_CPPFLAGS=-I$(top_srcdir)/include -I$(top_srcdir)/coreapi

LDADD=$(top_builddir)/coreapi/liblinphone.la $(BELLESIP_LIBS) $(LIBXML2_LIBS)

AM_LDFLAGS   = $(CUNIT_LIBS)

AM_CFLAGS    = $(STRICT_OPTIONS)  -DIN_LINPHONE $(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS) $(CUNIT_CFLAGS) $(BELLESIP_CFLAGS) $(LIBXML2_CFLAGS)
AM_OBJCFLAGS = $(AM_CFLAGS) -DNO_XCODE

liblinphone_tester_LDFLAGS= 

if BUILD_IOS
liblinphone_tester_SOURCES += liblinphonetester_ios.m liblinphonetester_ios.h
liblinphone_tester_LDFLAGS += -framework CoreFoundation -framework AudioToolbox -framework CoreAudio -framework Foundation -framework QuartzCore -framework OpenGLES -framework UIKit -framework AVFoundation
# we have to force the linker to use c-style link when mixing Obj-C and C sources, otherwise the Obj-C linker is used, which gives strange errors (UTF8?)
liblinphone_tester_LINK    = $(LINK)
endif


test: liblinphone_tester
	./liblinphone_tester  --config $(abs_srcdir) 

else

test:
	@echo "CUnit must be installed to be able to run the tests!"

endif
