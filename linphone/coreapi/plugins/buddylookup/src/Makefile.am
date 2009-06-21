pluginsdir=$(LINPHONE_PLUGINS_DIR)

plugins_LTLIBRARIES=libbuddylookup.la

libbuddylookup_la_SOURCES=lookup.c

libbuddylookup_la_LIBADD=$(LIBSOUP_LIBS) $(LIBLINPHONE_LIBS) 

libbuddylookup_la_LDFLAGS=-no-undefined

AM_CFLAGS= $(LIBLINPHONE_CFLAGS) $(LIBSOUP_CFLAGS)

if BUILD_WIN32
#hack for mingw to force plugins dll to be where we want them to be.
install-data-hook:
	mv $(DESTDIR)$(pluginsdir)/../bin/libbuddylookup*.dll \
	$(DESTDIR)$(pluginsdir)/.
	rm -f $(DESTDIR)$(pluginsdir)/*.la
	rm -f $(DESTDIR)$(pluginsdir)/*.dll.a
endif
