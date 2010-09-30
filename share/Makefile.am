
SUBDIRS=C fr it ja cs

LINPHONE_SOUNDS=ringback.wav hello8000.wav hello16000.wav
LINPHONE_RINGS=rings/orig.wav \
				rings/oldphone.wav \
				rings/oldphone-mono.wav \
				rings/oldphone-mono-30s.caf \
				rings/rock.wav \
				rings/bigben.wav \
				rings/toy.wav \
				rings/sweet.wav \
				rings/synth.wav \
				rings/tapping.wav

sounddir=$(datadir)/sounds/linphone

sound_DATA=$(LINPHONE_SOUNDS)

ringdir=$(datadir)/sounds/linphone/rings

ring_DATA=$(LINPHONE_RINGS)

#to be compliant with freedesktop.org:
linphone_fddir= $(datadir)/applications
linphone_fd_DATA= linphone.desktop


pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=linphone.pc

EXTRA_DIST = 					$(LINPHONE_SOUNDS) \
								$(LINPHONE_RINGS) \
						        linphone.desktop.in   \
								linphone.pc.in \
								Makefile.inc
