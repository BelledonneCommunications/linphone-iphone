
EXTRA_DIST = Doxyfile.in doxygen.dox.in

#html doc
if HAVE_DOXYGEN

SOURCES=$(top_srcdir)/include/mediastreamer2/*.h $(top_srcdir)/src/*.c $(top_srcdir)/src/*.h

# doxdir & pkgdocdir are not always defined by automake
docdir=$(datadir)/doc
pkgdocdir=$(docdir)/$(PACKAGE)-$(VERSION)
doc_htmldir=$(pkgdocdir)/html

doc_html_DATA = $(top_builddir)/help/doc/html/html.tar

$(doc_html_DATA): $(top_builddir)/help/doc/html/index.html
	cd $(<D) && tar cf html.tar *

$(top_builddir)/help/doc/html/index.html: $(SOURCES) Doxyfile Makefile.am
	rm -rf doc
	$(DOXYGEN) Doxyfile

install-data-hook:
	cd $(DESTDIR)$(doc_htmldir) && tar xf html.tar && rm -f html.tar

uninstall-hook:
	cd $(DESTDIR)$(doc_htmldir) && rm -f *

endif

clean-local:
	rm -rf doc