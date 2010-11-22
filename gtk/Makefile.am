UI_FILES=	about.ui \
		main.ui \
		password.ui \
		contact.ui \
		parameters.ui \
		sip_account.ui \
		chatroom.ui \
		call_logs.ui \
		log.ui \
		buddylookup.ui \
		waiting.ui

PIXMAPS=	\
		stock_people.png 

LINPHONE_ICO_RC_FILE=linphone.rc
LINPHONE_ICO_FILE=linphone.ico

EXTRA_DIST=	$(PIXMAPS) \
		$(UI_FILES) \
		linphone.iss \
		$(LINPHONE_ICO_RC_FILE) \
		$(LINPHONE_ICO_FILE)


if BUILD_GTK_UI

BUILT_SOURCES=version_date.h

bin_PROGRAMS=linphone-3

linphone_3_SOURCES=	\
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
			setupwizard.c\
			incall_view.c \
			loginframe.c \
			linphone.h 

linphone_3_LDADD=$(ORTP_LIBS) \
		$(MEDIASTREAMER_LIBS) \
		$(top_builddir)/coreapi/liblinphone.la \
			$(LIBGTK_LIBS) $(INTLLIBS) 


if BUILD_WIN32

linphone.res:	$(LINPHONE_ICO_RC_FILE) $(LINPHONE_ICO_FILE)
	windres $(LINPHONE_ICO_RC_FILE) -O coff -o linphone.res

linphone_3_LDADD+=linphone.res -lwininet
linphone_3_LDFLAGS=-Wl,--export-all-symbols -mwindows
else
linphone_3_LDFLAGS=-export-dynamic
endif

uidir=$(datadir)/linphone
ui_DATA=$(UI_FILES) $(PIXMAPS) $(top_srcdir)/COPYING

endif


AM_CFLAGS= -DIN_LINPHONE -I$(top_srcdir)/coreapi/ \
		$(MEDIASTREAMER_CFLAGS) \
		$(ORTP_CFLAGS) \
		$(STRICT_OPTIONS) $(LIBGTK_CFLAGS) $(IPV6_CFLAGS) \
		$(OSIP_CFLAGS) 


version_date.h: $(top_srcdir)/configure.ac
	echo "#define LINPHONE_VERSION_DATE \"$(VERSION)-`date +%y%m%d`\"" > $@

newdate:
	rm -f version_date.h

CLEANFILES=version_date.h linphone.res

