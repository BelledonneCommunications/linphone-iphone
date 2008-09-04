## Process this file with automake to produce Makefile.in

linphone_common_sources=\
	linphone.c linphone.h \
	gui_utils.c gui_utils.h \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	presence.c presence.h \
	propertybox.c propertybox.h \
	addressbook.c addressbook.h \
	friends.c friends.h


INCLUDES = \
	-I$(top_srcdir)\
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/coreapi \
	-I$(top_srcdir)/mediastreamer2/include

if BUILD_GTK

bin_PROGRAMS = linphone

linphone_SOURCES = \
	main.c \
	$(linphone_common_sources)

linphone_LDADD = $(LIBGTK_LIBS) $(INTLLIBS) \
				$(ORTP_LIBS) \
				$(top_builddir)/mediastreamer2/src/libmediastreamer.la \
				$(top_builddir)/coreapi/liblinphone.la 

endif

AM_CFLAGS=$(STRICT_OPTIONS) $(LIBGTK_CFLAGS) $(IPV6_CFLAGS) \
		$(ORTP_CFLAGS) $(OSIP_CFLAGS)

@INTLTOOL_SERVER_RULE@

if BUILD_GNOME_APPLET

gnome_appletdir=$(libexecdir)

gnome_applet_PROGRAMS = linphone_applet

linphone_applet_SOURCES = \
	$(linphone_common_sources)\
	applet.c

linphone_applet_CFLAGS=$(AM_CFLAGS) $(GNOME_APPLETS_CFLAGS) -DLINPHONE_APPLET

linphone_applet_LDADD = \
				$(GNOME_APPLETS_LIBS) \
				$(top_builddir)/coreapi/liblinphone.la \
				$(OSIP_LIBS)


serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_LinphoneApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@


uidir   = $(datadir)/gnome-2.0/ui
ui_DATA = GNOME_LinphoneApplet.xml

endif			

EXTRA_DIST = 						\
	GNOME_LinphoneApplet.server.in.in	\
	applet.c \
	$(ui_DATA)

DISTCLEANFILES=GNOME_LinphoneApplet.server.in GNOME_LinphoneApplet.server
