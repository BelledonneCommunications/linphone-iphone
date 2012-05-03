

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
			commands/quit.cc \
			daemon.h \
                        commands/answer.h \
                        commands/audio-codec-disable.h \
                        commands/audio-codec-enable.h \
                        commands/audio-codec-get.h \
                        commands/audio-codec-move.h \
                        commands/audio-codec-set.h \
                        commands/audio-stream-start.h \
                        commands/audio-stream-stop.h \
                        commands/call.h \
                        commands/call-stats.h \
                        commands/call-status.h \
                        commands/help.h \
                        commands/pop-event.h \
                        commands/ptime.h \
                        commands/register.h \
                        commands/register-status.h \
                        commands/terminate.h \
                        commands/unregister.h \
                        commands/quit.h

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

