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
GITLOG=`cd $(top_srcdir) && git log -1 --pretty=format:%H configure.ac`

ECHO=/bin/echo

SUBDIRS=. help

EXTRA_DIST=linphonecore_jni.cc $(GITVERSION_FILE)

BUILT_SOURCES=$(GITVERSION_FILE)

CLEANFILES=$(GITVERSION_FILE)

## Process this file with automake to produce Makefile.in
lib_LTLIBRARIES=liblinphone.la

liblinphone_la_SOURCES=\
	account_creator.c \
	address.c \
	authentication.c \
	buffer.c \
	callbacks.c \
	carddav.h \
	call_log.c \
	call_params.c \
	chat.c \
	chat_file_transfer.c \
	conference.cc conference_private.h \
	contactprovider.c contact_providers_priv.h \
	content.c \
	dial_plan.c \
	dict.c \
	ec-calibrator.c \
	echo-tester.c \
	enum.c enum.h \
	error_info.c \
	event.c \
	friend.c \
	friendlist.c \
	im_notif_policy.c \
	info.c \
	ldapprovider.c \
	linphonecall.c \
	linphonecore.c \
	localplayer.c \
	lpc2xml.c lpc2xml.h \
	lime.c lime.h\
	im_encryption_engine.c \
	lpconfig.c \
	lsd.c \
	message_storage.c \
	misc.c  \
	nat_policy.c \
	offeranswer.c offeranswer.h\
	player.c \
	presence.c \
	private.h \
	proxy.c \
	quality_reporting.c quality_reporting.h\
	remote_provisioning.c \
	sal.c \
	siplogin.c \
	sipsetup.c \
	vcard_private.h \
	xml2lpc.c xml2lpc.h \
	xml.c \
	xmlrpc.c \
	vtables.c \
	carddav.c \
	ringtoneplayer.c \
	sqlite3_bctbx_vfs.c sqlite3_bctbx_vfs.h\
	factory.c \
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

liblinphone_la_SOURCES+=linphone_tunnel_config.c
if BUILD_TUNNEL
liblinphone_la_SOURCES+=linphone_tunnel.cc TunnelManager.cc TunnelManager.hh
else
liblinphone_la_SOURCES+=linphone_tunnel_stubs.c
endif

if BUILD_VCARD
liblinphone_la_SOURCES+=vcard.cc
else
liblinphone_la_SOURCES+=vcard_stubs.c
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
		$(MEDIASTREAMER_LIBS) \
		$(ORTP_LIBS) \
		$(SIPSTACK_LIBS) \
		$(TUNNEL_LIBS) \
		$(SQLITE3_LIBS) \
		$(LIBXML2_LIBS) \
		$(LDAP_LIBS) \
		$(SASL_LIBS) \
		$(BELCARD_LIBS) \
		$(ZLIB_LIBS) \
		$(BCTOOLBOX_LIBS)


AM_CPPFLAGS=\
	-I$(top_srcdir) -I$(top_srcdir)/include -I$(builddir) \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(LIBXML2_CFLAGS)

COMMON_CFLAGS=\
	$(STRICT_OPTIONS) \
	$(SIPSTACK_CFLAGS) \
	-DENABLE_TRACE \
	-DLOG_DOMAIN=\"LinphoneCore\" \
	$(IPV6_CFLAGS) \
	-DORTP_INET6 \
	$(VIDEO_CFLAGS) \
	$(TUNNEL_CFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(LDAP_CFLAGS) \
	$(SASL_CFLAGS) \
	$(BELCARD_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(BCTOOLBOX_CFLAGS)

if BUILD_WIZARD
COMMON_CFLAGS+= -DBUILD_WIZARD
endif

COMMON_CFLAGS+= -DUSE_BELLESIP

AM_CFLAGS=$(COMMON_CFLAGS) $(STRICT_OPTIONS_CC)
AM_CXXFLAGS=$(COMMON_CFLAGS) $(STRICT_OPTIONS_CXX)
if BUILD_VCARD
AM_CXXFLAGS+=-std=c++11
endif

#Make sure that we are in linphone's git tree by doing git log $(top_srcdir)/configure.ac.
#if it is something known to git, then that will be ok to check the git describe number and make sure it is consistent with
#the PACKAGE_VERSION given in configure.ac

make_gitversion_h:
	if test -n "$(GITLOG)" ; then  \
		if test "$(GITDESCRIBE)" != "" ; then \
			if [ "$(GIT_TAG)" != "$(PACKAGE_VERSION)" ]; then \
				$(ECHO) "*** PACKAGE_VERSION and git tag differ. Please put them identical."; \
				exit 1; \
			fi ; \
			printf "#define LIBLINPHONE_GIT_VERSION \"$(GITDESCRIBE)\"\n" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
		elif test "$(GITREVISION)" != "" ; then \
			printf "#define LIBLINPHONE_GIT_VERSION \"$(LINPHONE_VERSION)_$(GITREVISION)\"\n" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
		else \
			printf "" > $(builddir)/$(GITVERSION_FILE_TMP) ; \
		fi ; \
		if ! test -f $(builddir)/$(GITVERSION_FILE) ; then \
			cp -f $(builddir)/$(GITVERSION_FILE_TMP) $(builddir)/$(GITVERSION_FILE) ; \
		fi ; \
		if test "`cat $(builddir)/$(GITVERSION_FILE_TMP)`" != "`cat $(builddir)/$(GITVERSION_FILE)`" ; then \
			cp -f $(builddir)/$(GITVERSION_FILE_TMP) $(builddir)/$(GITVERSION_FILE) ; \
		fi ; \
		rm -f $(builddir)/$(GITVERSION_FILE_TMP) ; \
	fi

$(GITVERSION_FILE): make_gitversion_h
