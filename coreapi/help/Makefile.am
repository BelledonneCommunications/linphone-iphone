
EXTRA_DIST = Doxyfile.in doxygen.dox

SOURCES= doxygen.dox *.c $(top_srcdir)/coreapi/*.c $(top_srcdir)/coreapi/*.h  


#html doc
if HAVE_DOXYGEN

# doxdir & pkgdocdir are not always defined by automake
docdir=$(datadir)/doc
pkgdocdir=$(docdir)/$(PACKAGE)-$(VERSION)
doc_htmldir=$(pkgdocdir)/html

doc_html_DATA = $(top_builddir)/coreapi/help/doc/html/html.tar

$(doc_html_DATA): $(top_builddir)/coreapi/help/doc/html/index.html
	cd $(<D) && tar cf html.tar *

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

noinst_PROGRAMS=helloworld registration

helloworld_SOURCES=helloworld.c

helloworld_LDADD=$(top_builddir)/coreapi/liblinphone.la \
				$(MEDIASTREAMER_LIBS) \
				$(ORTP_LIBS)

registration_SOURCES=registration.c

registration_LDADD=$(top_builddir)/coreapi/liblinphone.la \
				$(MEDIASTREAMER_LIBS) \
				$(ORTP_LIBS)
				

INCLUDES=-I$(top_srcdir)/coreapi \
		$(MEDIASTREAMER_CFLAGS)

AM_CFLAGS=$(STRICT_OPTIONS)  -DIN_LINPHONE \
	$(ORTP_CFLAGS) \
	$(OSIP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(EXOSIP_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	 $(IPV6_CFLAGS) \
	 -DORTP_INET6 \
	 $(VIDEO_CFLAGS) 

