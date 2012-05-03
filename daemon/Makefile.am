

bin_PROGRAMS=linphone-daemon linphone-daemon-pipetest

linphone_daemon_SOURCES=daemon.cc \
			commands/answer.cc \
			commands/audio-codec-disable.cc \
			commands/audio-codec-enable.cc \
			commands/audio-codec-get.cc \
			commands/audio-codec-move.cc \
			commands/audio-codec-set.cc \
			commands/audio-stream-start.cc \
			commands/audio-stream-stop.cc \
			commands/call.cc \
			commands/call-stats.cc \
			commands/call-status.cc \
			commands/help.cc \
			commands/pop-event.cc \
			commands/ptime.cc \
			commands/register.cc \
			commands/register-status.cc \
			commands/terminate.cc \
			commands/unregister.cc \
			commands/quit.cc


linphone_daemon_pipetest_SOURCES=daemon-pipetest.c

linphone_daemon_pipetest_LDADD=$(ORTP_LIBS)

linphone_daemon_LDADD=$(top_builddir)/coreapi/liblinphone.la $(READLINE_LIBS)  \
		$(MEDIASTREAMER_LIBS) \
		$(ORTP_LIBS) \
		$(SPEEX_LIBS) \
		$(OSIP_LIBS) 

AM_CFLAGS=$(READLINE_CFLAGS) -DIN_LINPHONE $(STRICT_OPTIONS)
AM_CXXFLAGS=$(READLINE_CXXFLAGS) -DIN_LINPHONE $(STRICT_OPTIONS)

INCLUDES = \
	-I$(top_srcdir)\
	-I$(top_srcdir)/coreapi\
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS)

