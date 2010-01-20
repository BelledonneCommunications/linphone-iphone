linphone_manualdir = $(datadir)/gnome/help/linphone/$(HELPLANG)/

linphone_help = \
        $(top_builddir)/share/$(HELPLANG)

if ENABLE_MANUAL
all-local:	$(linphone_help)/manual.html
else
all-local:
endif

$(linphone_help)/manual.html:
	rm -f $(linphone_help)/manual.html
	sgmltools $(srcdir)/manual.sgml

install-data-local:
			$(mkdir_p) $(DESTDIR)$(linphone_manualdir)
			-cp -f $(linphone_help)/*.html $(DESTDIR)/$(linphone_manualdir)/.
			-cp -f $(linphone_help)/*.css $(DESTDIR)/$(linphone_manualdir)/.

uninstall-local:
			rm -rf $(linphone_manualdir)


EXTRA_DIST=  manual.lyx manual.sgml
