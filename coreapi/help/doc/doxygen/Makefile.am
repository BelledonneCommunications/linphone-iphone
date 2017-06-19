EXTRA_DIST=Doxyfile.in doxygen.dox

SOURCES=doxygen.dox $(top_srcdir)/coreapi/help/*.c $(top_srcdir)/coreapi/*.c $(top_srcdir)/coreapi/*.h

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
