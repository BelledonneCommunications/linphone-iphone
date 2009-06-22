pluginsdir=$(PACKAGE_PLUGINS_DIR)

plugins_LTLIBRARIES=libmsx264.la

libmsx264_la_SOURCES=msx264.c

libmsx264_la_LIBADD=\
	$(LIBAVCODEC_LIBS) \
	$(LIBSWSCALE_LIBS) \
	$(MEDIASTREAMER_LIBS) \
	$(X264_LIBS)

libmsx264_la_LDFLAGS=-no-undefined

AM_CFLAGS=	$(LIBAVCODEC_CFLAGS) \
		$(LIBSWSCALE_CFLAGS) \
		$(MEDIASTREAMER_CFLAGS)\
		$(X264_CFLAGS)

if BUILD_WIN32
#hack for mingw to force plugins dll to be where we want them to be.
install-data-hook:
	mv $(DESTDIR)$(pluginsdir)/../bin/*.dll \
	$(DESTDIR)$(pluginsdir)/.
	rm -f $(DESTDIR)$(pluginsdir)/*.la
	rm -f $(DESTDIR)$(pluginsdir)/*.dll.a
endif
