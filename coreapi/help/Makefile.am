
EXTRA_DIST=Doxyfile.in doxygen.dox

SOURCES=doxygen.dox $(top_srcdir)/coreapi/help/*.c $(top_srcdir)/coreapi/*.c $(top_srcdir)/coreapi/*.h


# html doc
if HAVE_DOXYGEN

# docdir & pkgdocdir are not always defined by automake
pkgdocdir=$(docdir)/$(PACKAGE)-$(VERSION)
doc_htmldir=$(pkgdocdir)/html
doc_xmldir=$(pkgdocdir)/xml

doc_html_DATA = $(top_builddir)/coreapi/help/doc/html/html.tar

$(doc_html_DATA): $(top_builddir)/coreapi/help/doc/html/index.html
	cd $(top_builddir)/coreapi/help/doc/html/ && tar cf html.tar *

$(top_builddir)/coreapi/help/doc/html/index.html: $(SOURCES) Doxyfile Makefile.am
	rm -rf doc
	$(DOXYGEN) Doxyfile

doc_xml_DATA = $(top_builddir)/coreapi/help/doc/xml/xml.tar

$(doc_xml_DATA): $(top_builddir)/coreapi/help/doc/xml/index.xml
	cd $(top_builddir)/coreapi/help/doc/xml/ && tar cf xml.tar *

$(top_builddir)/coreapi/help/doc/xml/index.xml: $(top_builddir)/coreapi/help/doc/html/index.html


install-data-hook:
	cd $(DESTDIR)$(doc_htmldir) && tar xf html.tar && rm -f html.tar
	cd $(DESTDIR)$(doc_xmldir) && tar xf xml.tar && rm -f xml.tar

uninstall-hook:
	cd $(DESTDIR)$(doc_htmldir) && rm -f *
	cd $(DESTDIR)$(doc_xmldir) && rm -f *

endif

clean-local:
	rm -rf doc

#tutorials

if ENABLE_TUTORIALS

noinst_PROGRAMS=helloworld registration buddy_status chatroom notify filetransfer realtimetext_sender realtimetext_receiver

helloworld_SOURCES=helloworld.c
LINPHONE_TUTOS=$(helloworld_SOURCES)

helloworld_LDADD=$(top_builddir)/coreapi/liblinphone.la \
				$(MEDIASTREAMER_LIBS) \
				$(ORTP_LIBS) \
				$(BELLESIP_LIBS)

registration_SOURCES=registration.c
LINPHONE_TUTOS+=$(registration_SOURCES)

registration_LDADD=$(helloworld_LDADD)

buddy_status_SOURCES=buddy_status.c
LINPHONE_TUTOS+=$(buddy_status_SOURCES)

buddy_status_LDADD=$(helloworld_LDADD)

chatroom_SOURCES=chatroom.c
LINPHONE_TUTOS+=$(chatroom_SOURCES)

chatroom_LDADD=$(helloworld_LDADD)

notify_SOURCES=notify.c
LINPHONE_TUTOS+=$(notify_SOURCES)

notify_LDADD=$(helloworld_LDADD)

filetransfer_SOURCES=filetransfer.c
LINPHONE_TUTOS+=$(filetransfer_SOURCES)

filetransfer_LDADD=$(helloworld_LDADD)

realtimetext_sender_SOURCES=realtimetext_sender.c
LINPHONE_TUTOS+=$(realtimetext_sender_SOURCES)

realtimetext_sender_LDADD=$(helloworld_LDADD)

realtimetext_receiver_SOURCES=realtimetext_receiver.c
LINPHONE_TUTOS+=$(realtimetext_receiver_SOURCES)

realtimetext_receiver_LDADD=$(helloworld_LDADD)

AM_CFLAGS=\
	-I$(top_srcdir)/include \
	$(STRICT_OPTIONS) \
	$(STRICT_OPTIONS_CC) \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	$(IPV6_CFLAGS) \
	-DORTP_INET6 \
	$(VIDEO_CFLAGS) \
	$(BELLESIP_CFLAGS)


tutodir=$(datadir)/tutorials/linphone

tuto_DATA=$(LINPHONE_TUTOS)
endif
