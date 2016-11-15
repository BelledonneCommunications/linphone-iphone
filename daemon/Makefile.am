

bin_PROGRAMS=linphone-daemon linphone-daemon-pipetest

linphone_daemon_SOURCES=daemon.cc \
			commands/adaptive-jitter-compensation.cc \
			commands/answer.cc \
			commands/audio-codec-get.cc \
			commands/audio-codec-move.cc \
			commands/audio-codec-set.cc \
			commands/audio-codec-toggle.cc \
			commands/audio-stream-start.cc \
			commands/audio-stream-stop.cc \
			commands/audio-stream-stats.cc \
			commands/auth-infos-clear.cc \
			commands/call.cc \
			commands/call-stats.cc \
			commands/call-status.cc \
			commands/call-pause.cc \
			commands/call-resume.cc \
			commands/call-mute.cc \
			commands/video.cc \
			commands/call-transfer.cc \
			commands/conference.cc \
			commands/dtmf.cc \
			commands/firewall-policy.cc \
			commands/help.cc \
			commands/ipv6.cc \
			commands/jitterbuffer.cc \
			commands/media-encryption.cc \
			commands/msfilter-add-fmtp.cc \
			commands/play-wav.cc \
			commands/pop-event.cc \
			commands/port.cc \
			commands/ptime.cc \
			commands/register.cc \
			commands/register-status.cc \
			commands/terminate.cc \
			commands/unregister.cc \
			commands/quit.cc \
			commands/version.cc \
			commands/contact.cc \
			commands/config.cc commands/configcommand.h \
			daemon.h \
			commands/adaptive-jitter-compensation.h \
			commands/answer.h \
			commands/audio-codec-get.h \
			commands/audio-codec-move.h \
			commands/audio-codec-set.h \
			commands/audio-codec-toggle.h \
			commands/audio-stream-start.h \
			commands/audio-stream-stop.h \
			commands/audio-stream-stats.h \
			commands/auth-infos-clear.h \
			commands/call.h \
			commands/call-stats.h \
			commands/call-status.h \
			commands/call-pause.h \
			commands/call-resume.h \
			commands/call-mute.h \
			commands/video.h \
			commands/call-transfer.h \
			commands/conference.h \
			commands/dtmf.h \
			commands/firewall-policy.h \
			commands/help.h \
			commands/ipv6.h \
			commands/jitterbuffer.h \
			commands/media-encryption.h \
			commands/msfilter-add-fmtp.h \
			commands/play-wav.h \
			commands/pop-event.h \
			commands/port.h \
			commands/ptime.h \
			commands/register.h \
			commands/register-status.h \
			commands/terminate.h \
			commands/unregister.h \
			commands/quit.h \
			commands/contact.h \
			commands/netsim.cc commands/netsim.h\
			commands/cn.cc commands/cn.h \
			commands/version.h

linphone_daemon_pipetest_SOURCES=daemon-pipetest.c

linphone_daemon_pipetest_LDADD=$(ORTP_LIBS)

linphone_daemon_LDADD=$(top_builddir)/coreapi/liblinphone.la $(READLINE_LIBS)  \
		$(MEDIASTREAMER_LIBS) \
		$(ORTP_LIBS) \
		$(SPEEX_LIBS) \
		$(LIBXML2_LIBS) \
		$(BCTOOLBOX_LIBS)

AM_CFLAGS=$(READLINE_CFLAGS) $(STRICT_OPTIONS)
AM_CXXFLAGS=$(READLINE_CXXFLAGS) $(STRICT_OPTIONS)

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/coreapi \
	-I$(top_srcdir)/include \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	$(BELLESIP_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(BCTOOLBOX_CFLAGS)

