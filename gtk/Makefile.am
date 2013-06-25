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
		call_statistics.ui

PIXMAPS=	\
		stock_people.png 

LINPHONE_ICO_RC_FILE=linphone.rc
LINPHONE_ICO_FILE=linphone.ico

EXTRA_DIST=	$(PIXMAPS) \
		$(UI_FILES) \
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
			linphone.h 
if BUILD_WIZARD
linphone_SOURCES+=	\
			setupwizard.c
endif

linphone_LDADD=	$(top_builddir)/coreapi/liblinphone.la \
		$(LIBGTK_LIBS) $(NOTIFY1_LIBS) $(NOTIFY4_LIBS) $(LIBGTKMAC_LIBS) $(INTLLIBS) $(SQLITE3_LIBS)


if BUILD_WIN32

linphone.res:	$(LINPHONE_ICO_RC_FILE) $(LINPHONE_ICO_FILE)
	windres $(srcdir)/$(LINPHONE_ICO_RC_FILE) -O coff -o linphone.res

linphone_LDADD+=linphone.res -lwininet
linphone_LDFLAGS=-Wl,--export-all-symbols -mwindows
else
linphone_LDFLAGS=-export-dynamic
endif

uidir=$(datadir)/linphone
ui_DATA=$(UI_FILES) $(PIXMAPS) $(top_srcdir)/COPYING

endif


AM_CFLAGS= -DIN_LINPHONE -I$(top_srcdir)/coreapi/ \
		$(MEDIASTREAMER_CFLAGS) \
		$(ORTP_CFLAGS) \
		$(STRICT_OPTIONS) $(LIBGTK_CFLAGS) $(LIBGTKMAC_CFLAGS) $(IPV6_CFLAGS) \
		$(TUNNEL_CFLAGS) \
		$(SQLITE3_CFLAGS)


version_date.h: $(top_srcdir)/configure.ac
	echo "#define LINPHONE_VERSION_DATE \"$(VERSION)-`date +%y%m%d`\"" > $@

newdate:
	rm -f version_date.h

CLEANFILES=version_date.h linphone.res

