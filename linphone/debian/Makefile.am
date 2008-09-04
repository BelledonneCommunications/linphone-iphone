EXTRA_DIST=  copyright dirs manpage.sgml.ex rules \
					control docs manpage.1.ex menu watch.ex changelog


changelog:	changelog.stamp
	echo -n -e "linphone (">changelog
	echo -n -e $(VERSION)>>changelog
	echo -n -e "-1) unstable; urgency=low\n\n">>changelog
	echo -n -e "  * New Release.\n\n">>changelog
	echo -n -e " -- Simon Morlat <simon.morlat@linphone.org>  ">>changelog
	date -R >>changelog
	touch changelog.stamp
	
changelog.stamp:
	touch changelog.stamp
