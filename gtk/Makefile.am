UI_FILES=	about.ui \
		main.ui \
		password.ui \
		contact.ui \
		parameters.ui \
		sip_account.ui \
		call_logs.ui \
		keypad.ui \
		log.ui \
		buddylookup.ui \
		tunnel_config.ui \
		waiting.ui \
		dscp_settings.ui \
		call_statistics.ui \
		ldap.ui \
		config-uri.ui \
		provisioning-fetch.ui \
		audio_assistant.ui \
		chatroom_frame.ui \
		in_call_frame.ui \
		conf_frame.ui \
		callee_frame.ui \
		login_frame.ui \
		setup_wizard.ui

PIXMAPS=	\
		stock_people.png

LINPHONE_ICO_RC_FILE=linphone.rc
LINPHONE_ICO_FILE=linphone.ico

EXTRA_DIST= \
	linphone.iss \
	$(LINPHONE_ICO_RC_FILE) \
	$(LINPHONE_ICO_FILE)
	gtkrc \
	gtkrc.mac

if BUILD_GTK_UI

BUILT_SOURCES=version_date.h

bin_PROGRAMS=linphone

linphone_SOURCES=	\
			main.c \
			propertybox.c \
			friendlist.c \
			support.c \
			chat.c \
			calllogs.c \
			logging.c \
			update.c \
			buddylookup.c \
			utils.c \
			incall_view.c \
			loginframe.c \
			singleinstance.c \
			conference.c \
			config-fetching.c \
			audio_assistant.c \
			videowindow.c \
			status_icon.c status_icon.h \
			linphone.h regex.h

if BUILD_WIZARD
linphone_SOURCES+= \
	setupwizard.c setupwizard.h
endif

if BUILD_STATUS_NOTIFIER
linphone_SOURCES+= \
	status_notifier.c \
	status_notifier.h
endif


linphone_LDADD=	$(top_builddir)/coreapi/liblinphone.la \
		$(LIBGTK_LIBS) $(NOTIFY1_LIBS) $(NOTIFY4_LIBS) $(LIBGTKMAC_LIBS) $(INTLLIBS) $(SQLITE3_LIBS) $(BELLESIP_LIBS) $(BCTOOLBOX_LIBS)


if BUILD_WIN32

linphone.res:	$(LINPHONE_ICO_RC_FILE) $(LINPHONE_ICO_FILE)
	$(WINDRES) $(srcdir)/$(LINPHONE_ICO_RC_FILE) -O coff -o linphone.res

linphone_LDADD+=linphone.res -lwininet
linphone_LDFLAGS=-Wl,--export-all-symbols -mwindows
else
linphone_LDFLAGS=-export-dynamic
endif

uidir=$(datadir)/linphone
dist_ui_DATA=$(UI_FILES) $(PIXMAPS) $(top_srcdir)/COPYING

if BUILD_MACOS
linphone_SOURCES+=mac.m
linphone_LDFLAGS+=-framework Cocoa
endif
endif


AM_CPPFLAGS= -I$(top_srcdir)/include/ -I$(top_builddir)/coreapi/ -I$(top_srcdir)/coreapi/ \
		$(MEDIASTREAMER_CFLAGS) \
		$(ORTP_CFLAGS) $(BELLESIP_CFLAGS) \
		$(STRICT_OPTIONS) $(STRICT_OPTIONS_CC) $(LIBGTK_CFLAGS) $(LIBGTKMAC_CFLAGS) $(IPV6_CFLAGS) \
		$(TUNNEL_CFLAGS) \
		$(SQLITE3_CFLAGS) \
		$(BCTOOLBOX_CFLAGS)
		
version_date.h: $(top_srcdir)/configure.ac
	echo "#define LINPHONE_VERSION_DATE \"$(VERSION)-`date +%y%m%d`\"" > $@

newdate:
	rm -f version_date.h

CLEANFILES=version_date.h linphone.res

