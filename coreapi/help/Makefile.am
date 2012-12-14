
EXTRA_DIST=Doxyfile.in doxygen.dox

SOURCES=doxygen.dox $(top_srcdir)/coreapi/help/*.c $(top_srcdir)/coreapi/*.c $(top_srcdir)/coreapi/*.h  


# html doc
if HAVE_DOXYGEN

# docdir & pkgdocdir are not always defined by automake
pkgdocdir=$(docdir)/$(PACKAGE)-$(VERSION)
doc_htmldir=$(pkgdocdir)/html

doc_html_DATA = $(top_builddir)/coreapi/help/doc/html/html.tar

$(doc_html_DATA): $(top_builddir)/coreapi/help/doc/html/index.html
	cd $(top_builddir)/coreapi/help/doc/html/ && tar cf html.tar *

$(top_builddir)/coreapi/help/doc/html/index.html: $(SOURCES) Doxyfile Makefile.am
	rm -rf doc
	$(DOXYGEN) Doxyfile

install-data-hook:
	cd $(DESTDIR)$(doc_htmldir) && tar xf html.tar && rm -f html.tar

uninstall-hook:
	cd $(DESTDIR)$(doc_htmldir) && rm -f *

endif

clean-local:
	rm -rf doc

if ENABLE_TESTS
#tutorials
if BUILD_TESTS
noinst_PROGRAMS=helloworld registration buddy_status chatroom

helloworld_SOURCES=helloworld.c
LINPHONE_TUTOS=$(helloworld_SOURCES)

helloworld_LDADD=$(top_builddir)/coreapi/liblinphone.la \
				$(MEDIASTREAMER_LIBS) \
				$(ORTP_LIBS)

registration_SOURCES=registration.c
LINPHONE_TUTOS+=$(registration_SOURCES)

registration_LDADD=$(helloworld_LDADD)

buddy_status_SOURCES=buddy_status.c
LINPHONE_TUTOS+=$(buddy_status_SOURCES)

buddy_status_LDADD=$(helloworld_LDADD)

chatroom_SOURCES=chatroom.c
LINPHONE_TUTOS+=$(chatroom_SOURCES)

chatroom_LDADD=$(helloworld_LDADD)
endif
												
endif

AM_CFLAGS=\
	$(STRICT_OPTIONS) \
	-DIN_LINPHONE \
	$(ORTP_CFLAGS) \
	$(OSIP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(EXOSIP_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	$(IPV6_CFLAGS) \
	-DORTP_INET6 \
	$(VIDEO_CFLAGS)


tutodir=$(datadir)/tutorials/linphone

tuto_DATA=$(LINPHONE_TUTOS)
