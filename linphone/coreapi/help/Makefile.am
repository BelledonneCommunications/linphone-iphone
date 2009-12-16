
EXTRA_DIST = Doxyfile.in doxygen.dox.in

SOURCES=$(top_srcdir)/coreapi/*.h $(top_srcdir)/coreapi/*.c


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
