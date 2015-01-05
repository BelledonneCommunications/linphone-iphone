AUTOMAKE_OPTIONS = subdir-objects
GITVERSION_FILE=liblinphone_gitversion.h
GITVERSION_FILE_TMP=liblinphone_gitversion.h.tmp
GITDESCRIBE=`cd $(top_srcdir) && git describe --always`
GIT_TAG=`cd $(top_srcdir) && git describe --abbrev=0`
GITREVISION=`cd $(top_srcdir) && git rev-parse HEAD`

## This command is used to check if the sources are cloned in a git repo.
## We can't only depend on the presence of the .git/ directory anymore,
## because of gits submodule handling.
## We now simply issue a git log on configure.ac and if the output is empty (error or file not tracked), then we are not in git.
GITLOG=$(shell git log -1 --pretty=format:%H $(top_srcdir)/configure.ac)

ECHO=/bin/echo

SUBDIRS=. help

EXTRA_DIST=linphonecore_jni.cc $(GITVERSION_FILE)

BUILT_SOURCES=$(GITVERSION_FILE)

CLEANFILES=$(GITVERSION_FILE)

## Process this file with automake to produce Makefile.in
linphone_includedir=$(includedir)/linphone

linphone_include_HEADERS=\
	buffer.h \
	call_log.h \
	call_params.h \
	content.h \
	event.h \
	linphonecore.h \
	linphonecore_utils.h \
	linphonefriend.h \
	linphonepresence.h \
	linphone_tunnel.h \
	lpc2xml.h \
	lpconfig.h \
	sipsetup.h \
	xml2lpc.h

lib_LTLIBRARIES=liblinphone.la

liblinphone_la_SOURCES=\
	address.c \
	authentication.c \
	buffer.c \
	callbacks.c \
	call_log.c \
	call_params.c \
	chat.c \
	conference.c \
	contactprovider.c contactprovider.h contact_providers_priv.h \
	content.c \
	dict.c \
	ec-calibrator.c \
	enum.c enum.h \
	event.c event.h \
	friend.c \
	info.c \
	ldap/ldapprovider.c ldap/ldapprovider.h \
	linphonecall.c \
	linphonecore.c linphonecore.h \
	linphonecore_utils.h \
	localplayer.c \
	lpc2xml.c \
	lpconfig.c lpconfig.h \
	lsd.c \
	message_storage.c \
	misc.c  \
	offeranswer.c offeranswer.h\
	player.c \
	presence.c \
	private.h \
	proxy.c \
	quality_reporting.c quality_reporting.h\
	remote_provisioning.c \
	sal.c \
	siplogin.c \
	sipsetup.c sipsetup.h \
	xml2lpc.c \
	xml.c \
	$(GITVERSION_FILE)

if BUILD_UPNP
liblinphone_la_SOURCES+=upnp.c upnp.h
endif

liblinphone_la_SOURCES+=	bellesip_sal/sal_address_impl.c \
				bellesip_sal/sal_impl.c bellesip_sal/sal_impl.h \
				bellesip_sal/sal_op_impl.c \
				bellesip_sal/sal_op_call.c \
				bellesip_sal/sal_op_registration.c \
				bellesip_sal/sal_sdp.c  \
				bellesip_sal/sal_op_message.c \
				bellesip_sal/sal_op_presence.c \
				bellesip_sal/sal_op_publish.c \
				bellesip_sal/sal_op_call_transfer.c \
				bellesip_sal/sal_op_info.c \
				bellesip_sal/sal_op_events.c

if BUILD_WIZARD
liblinphone_la_SOURCES+=sipwizard.c
endif

liblinphone_la_SOURCES+=linphone_tunnel_config.c
if BUILD_TUNNEL
liblinphone_la_SOURCES+=linphone_tunnel.cc TunnelManager.cc TunnelManager.hh linphone_tunnel.h
else
liblinphone_la_SOURCES+=linphone_tunnel_stubs.c linphone_tunnel.h
endif



liblinphone_la_LDFLAGS= -version-info $(LIBLINPHONE_SO_VERSION) -no-undefined

if HAVE_LD_OUTPUT_DEF
liblinphone_la_LDFLAGS += -Wl,--output-def,liblinphone-$(LIBLINPHONE_SO_CURRENT).def
defexecdir = $(libdir)
defexec_DATA = liblinphone-$(LIBLINPHONE_SO_CURRENT).def
CLEANFILES += $(defexec_DATA)

liblinphone-$(LIBLINPHONE_SO_CURRENT).def: liblinphone.la

if BUILD_WIN32
defexec_DATA += liblinphone-$(LIBLINPHONE_SO_CURRENT).lib
liblinphone-$(LIBLINPHONE_SO_CURRENT).lib: liblinphone-$(LIBLINPHONE_SO_CURRENT).def liblinphone.la
	$(DLLTOOL) --dllname liblinphone-$(LIBLINPHONE_SO_CURRENT).dll --input-def liblinphone-$(LIBLINPHONE_SO_CURRENT).def --output-lib $@ liblinphone.la
endif
endif

liblinphone_la_LIBADD= \
		$(SIPSTACK_LIBS) \
		$(MEDIASTREAMER_LIBS) \
		$(ORTP_LIBS) $(OPENSSL_LIBS) \
		$(TUNNEL_LIBS) \
		$(LIBSOUP_LIBS) \
		$(SQLITE3_LIBS) \
		$(LIBXML2_LIBS) \
		$(LDAP_LIBS) $(SASL_LIBS)


if ENABLE_TESTS
noinst_PROGRAMS=test_lsd test_ecc test_numbers

test_lsd_SOURCES=test_lsd.c

test_lsd_LDADD=liblinphone.la $(liblinphone_la_LIBADD)

test_ecc_SOURCES=test_ecc.c

test_ecc_LDADD=liblinphone.la $(liblinphone_la_LIBADD)

test_numbers_SOURCES=test_numbers.c

test_numbers_LDADD=liblinphone.la $(liblinphone_la_LIBADD)
endif

AM_CPPFLAGS=\
	-I$(top_srcdir) -I$(top_srcdir)/include -I$(builddir)

COMMON_CFLAGS=\
	$(STRICT_OPTIONS) \
	-DIN_LINPHONE \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(SIPSTACK_CFLAGS) \
	$(LIBSOUP_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	$(IPV6_CFLAGS) \
	-DORTP_INET6 \
	$(VIDEO_CFLAGS) \
	$(TUNNEL_CFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(LDAP_CFLAGS) $(SASL_CFLAGS)

if BUILD_WIZARD
COMMON_CFLAGS+= -DBUILD_WIZARD
endif

COMMON_CFLAGS+= -DUSE_BELLESIP

AM_CFLAGS=$(COMMON_CFLAGS) $(STRICT_OPTIONS_CC)
AM_CXXFLAGS=$(COMMON_CFLAGS) $(STRICT_OPTIONS_CXX)

#Make sure that we are in linphone's git tree by doing git log $(top_srcdir)/configure.ac.
#if it is something known to git, then that will be ok to check the git describe number and make sure it is consistent with
#the PACKAGE_VERSION given in configure.ac

make_gitversion_h:
	if test -n "$(GITLOG)" ; then  \
		if test "$(GITDESCRIBE)" != "" ; then \
			if test "$(GIT_TAG)" != "$(PACKAGE_VERSION)" ; then \
				echo "*** PACKAGE_VERSION and git tag differ. Please put them identical."; \
				exit 1; \
			fi ; \
			$(ECHO) -n "#define LIBLINPHONE_GIT_VERSION \"$(GITDESCRIBE)\"" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
		elif test "$(GITREVISION)" != "" ; then \
			$(ECHO) -n "#define LIBLINPHONE_GIT_VERSION \"$(LINPHONE_VERSION)_$(GITREVISION)\"" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
		else \
			$(ECHO) -n "" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
		fi ; \
	else \
		$(ECHO) -n "" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
	fi
	if ! test -f $(builddir)/$(GITVERSION_FILE) ; then \
		cp -f $(builddir)/$(GITVERSION_FILE_TMP) $(builddir)/$(GITVERSION_FILE) ; \
	fi
	if test "`cat $(builddir)/$(GITVERSION_FILE_TMP)`" != "`cat $(builddir)/$(GITVERSION_FILE)`" ; then \
		cp -f $(builddir)/$(GITVERSION_FILE_TMP) $(builddir)/$(GITVERSION_FILE) ; \
	fi
	rm -f $(builddir)/$(GITVERSION_FILE_TMP) ;

$(GITVERSION_FILE): make_gitversion_h
