
SUBDIRS=C fr it ja cs xml

LINPHONE_ONHOLD_MUSIC= 	toy-mono.wav \
			dont_wait_too_long.mkv

LINPHONE_SOUNDS=ringback.wav hello8000.wav hello16000.wav incoming_chat.wav $(LINPHONE_ONHOLD_MUSIC)

LINPHONE_RINGS=		\
			rings/oldphone-mono.wav \
			rings/oldphone-mono-30s.caf \
			rings/four_hands_together.mkv \
			rings/house_keeping.mkv \
			rings/its_a_game.mkv \
			rings/leaving_dreams.mkv \
			rings/notes_of_the_optimistic.mkv \
			rings/soft_as_snow.mkv

sounddir=$(datadir)/sounds/linphone

sound_DATA=$(LINPHONE_SOUNDS)

ringdir=$(datadir)/sounds/linphone/rings

ring_DATA=$(LINPHONE_RINGS)

#to be compliant with freedesktop.org:
linphone_fddir= $(datadir)/applications
linphone_fd_DATA= linphone.desktop audio-assistant.desktop


pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=linphone.pc

linphonedir=$(datadir)/linphone
linphone_DATA=rootca.pem

appdatadir=$(datadir)/appdata
appdata_DATA=linphone.appdata.xml

#download root ca from mozilla using script from curl (mk-ca-bundle.pl).
#if that fails (no connection, no perl SSL...) , then a rootca bundle archived in the source tree is taken instead.
rootca.pem:
	rm -f $(builddir)/fresh-rootca.pem
	- HTTPS_CA_DIR=$(HTTPS_CA_DIR) $(top_srcdir)/scripts/mk-ca-bundle.pl $(builddir)/fresh-rootca.pem
	if test -f $(builddir)/fresh-rootca.pem ; then \
		cp -f $(builddir)/fresh-rootca.pem $(builddir)/rootca.pem ; \
	else \
		cp -f $(srcdir)/archived-rootca.pem $(builddir)/rootca.pem ; \
	fi

EXTRA_DIST =					$(LINPHONE_SOUNDS) \
						$(LINPHONE_RINGS) \
						linphone.desktop.in   \
						audio-assistant.desktop.in   \
						linphone.pc.in \
						Makefile.inc \
						archived-rootca.pem \
						$(appdata_DATA)

CLEANFILES=rootca.pem certdata.txt fresh-rootca.pem


