#plugin to enable fonis into linphone

JAVA_LDFLAGS=\
	-Wl,-rpath,$(javadir)/jre/lib/amd64 -Wl,-rpath,$(javadir)/jre/lib/i386 \
	-Wl,-rpath,$(javadir)/jre/lib/amd64/server -Wl,-rpath,$(javadir)/jre/lib/i386/server

pluginsdir=$(LINPHONE_PLUGINS_DIR)

plugins_LTLIBRARIES=libfonisprovider.la

libfonisprovider_la_SOURCES=fonis.c

libfonisprovider_la_LIBADD=$(top_builddir)/launcher/src/libfonisc.la

libfonisprovider_la_LDFLAGS=$(JAVA_LDFLAGS)

AM_CFLAGS= $(LIBLINPHONE_CFLAGS)

INCLUDES=-I$(top_srcdir)/launcher/src