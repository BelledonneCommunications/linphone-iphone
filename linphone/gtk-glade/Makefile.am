GLADE_FILES=	about.glade \
		main.glade \
		password.glade \
		contact.glade \
		incoming_call.glade \
		parameters.glade \
		sip_account.glade \
		chatroom.glade \
		call_logs.glade \
		log.glade \
		buddylookup.glade \
		waiting.glade

PIXMAPS=	linphone2.png \
		linphone.png \
		stock_people.png 

EXTRA_DIST=	$(PIXMAPS) \
		$(GLADE_FILES) \
		linphone.iss \
		linphone.dev

if BUILD_GLADE_UI

bin_PROGRAMS=linphone-3

linphone_3_SOURCES=	main.c \
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
			linphone.h

linphone_3_LDADD=$(top_builddir)/oRTP/src/libortp.la \
		$(top_builddir)/mediastreamer2/src/libmediastreamer.la \
		$(top_builddir)/coreapi/liblinphone.la \
			$(LIBGTK_LIBS) $(INTLLIBS) \
			$(LIBGLADE_LIBS)


if BUILD_WIN32

linphone.res:	linphone.rc linphone.ico
	windres linphone.rc -O coff -o linphone.res

linphone_3_LDADD+=linphone.res -lwininet
linphone_3_LDFLAGS=-Wl,--export-all-symbols -mwindows
else
linphone_3_LDFLAGS=-export-dynamic
endif

gladedir=$(datadir)/linphone
glade_DATA=$(GLADE_FILES) $(PIXMAPS)

#all-local:	gtk-linphone.ui	

#gtk-linphone.ui:	gtk-linphone.glade
#	gtk-builder-convert gtk-linphone.glade $@

endif


AM_CFLAGS= -DIN_LINPHONE -I$(top_srcdir)/coreapi/ \
		-I$(top_srcdir)/mediastreamer2/include/ \
		$(LIBGLADE_CFLAGS) $(STRICT_OPTIONS) $(LIBGTK_CFLAGS) $(IPV6_CFLAGS) \
		$(ORTP_CFLAGS) $(OSIP_CFLAGS) 


