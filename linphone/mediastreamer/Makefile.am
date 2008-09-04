EXTRA_DIST=Makefile.ms 


if BUILD_UGLIB
SUPPORTLIB=$(top_builddir)/support/libuglib.la
endif


#gdk video output
if BUILD_VIDEO
VIDEO_TESTPROGS=test_v4l test_videostream 
#videoserver videoclient
endif

if BUILD_TRUESPEECH
  TRUESPEECH_SOURCES = mstruespeechencoder.c mstruespeechencoder.h \
				mstruespeechdecoder.c mstruespeechdecoder.h
  TRUESPEECH_LIBADD = ../win32acm/libwin32acm.a
  TRUESPEECH_TEST = test_truespeech
  TRUESPEECH_INCLUDES = -I$(top_srcdir)/win32acm
endif

if BUILD_MEDIASTREAMER

noinst_LTLIBRARIES = libmediastreamer.la 
endif

useless_files=mstcpserv.c mstcpserv.h mstcpclient.c mstcpclient.h 

libmediastreamer_la_SOURCES=msfilter.c msfilter.h msutils.h waveheader.h\
							mscodec.c mscodec.h \
							mssoundread.c mssoundread.h \
							mssoundwrite.c mssoundwrite.h \
							msbuffer.c msbuffer.h  \
							msqueue.c msqueue.h \
							msfifo.c msfifo.h \
							ms.c ms.h\
							mssync.c mssync.h \
							msnosync.c msnosync.h \
							msread.c msread.h \
							mswrite.c mswrite.h \
							mscopy.c mscopy.h \
							msosswrite.c msosswrite.h  \
							msossread.c msossread.h \
							msringplayer.c msringplayer.h \
							msGSMencoder.c msGSMencoder.h \
							msGSMdecoder.c msGSMdecoder.h \
							msLPC10encoder.c msLPC10encoder.h \
							msLPC10decoder.c msLPC10decoder.h \
							msrtprecv.c msrtprecv.h   \
							msrtpsend.c msrtpsend.h	\
							msAlawenc.c msAlawenc.h g711common.h \
							msAlawdec.c msAlawdec.h g711common.h \
							msMUlawenc.c msMUlawenc.h g711common.h \
							msMUlawdec.c msMUlawdec.h g711common.h \
							mstimer.c mstimer.h \
							msqdispatcher.c msqdispatcher.h \
							msfdispatcher.c msfdispatcher.h \
							sndcard.c sndcard.h \
							osscard.c osscard.h\
							hpuxsndcard.c \
							alsacard.c alsacard.h \
							jackcard.c jackcard.h \
							audiostream.c mediastream.h \
							$(TRUESPEECH_SOURCES)\
							msspeexenc.c msspeexenc.h msspeexdec.c msspeexdec.h \
							$(VIDEO_SOURCES)

if BUILD_VIDEO
libmediastreamer_la_SOURCES+=msv4l.c msv4l.h  affine.c affine.h \
						msavencoder.c msavencoder.h\
						msavdecoder.c msavdecoder.h \
						videostream.c \
						msvideosource.c msvideosource.h \
						mssdlout.c mssdlout.h \
						rfc2429.h
endif

libmediastreamer_la_LIBADD= $(GLIB_LIBS) \
							../gsmlib/libgsm.la \
							../lpc10-1.5/liblpc10.la \
							../oRTP/src/libortp.la \
							$(JACK_LIBS)\
							$(SAMPLERATE_LIBS)\
							$(SUPPORTLIB) \
							$(ALSA_LIBS) \
							$(TRUESPEECH_LIBADD) \
							$(SPEEX_LIBS) \
							$(VIDEO_LIBS)



if BUILD_MEDIASTREAMER
noinst_PROGRAMS=mstest ring_test test_gsm test_lpc10 test_alaw test_mulaw \
                        test_speex \
			test_rtprecv \
			$(VIDEO_TESTPROGS) $(TRUESPEECH_TEST)

libexec_PROGRAMS=mediastream
endif


# test program to test TrueSpeech encoder and decoder objects
test_truespeech_SOURCES=test_truespeech.c
test_truespeech_LDADD=libmediastreamer.la

mstest_SOURCES=test.c
mstest_LDADD=libmediastreamer.la 

#test program to test MSRingPlayer object
ring_test_SOURCES=ring_test.c
ring_test_LDADD=libmediastreamer.la

#test program to test GSM dec and enc objects
test_gsm_SOURCES=test_gsm.c
test_gsm_LDADD=libmediastreamer.la

#test program to test speex dec and enc objects
test_speex_SOURCES=test_speex.c
test_speex_LDADD=libmediastreamer.la 

#test program to test LPC10-1.5 dec and enc objects
test_lpc10_SOURCES=test_lpc10.c
test_lpc10_LDADD=libmediastreamer.la

#test program to test ALAW dec and enc objects
test_alaw_SOURCES=test_alaw.c
test_alaw_LDADD=libmediastreamer.la

#test program to test MULAW dec and enc objects
test_mulaw_SOURCES=test_mulaw.c
test_mulaw_LDADD=libmediastreamer.la


#test program to test rtprecv object 
test_rtprecv_SOURCES=test_rtprecv.c
test_rtprecv_LDADD=libmediastreamer.la

#test program to test full video stream
test_videostream_SOURCES=test_videostream.c
test_videostream_LDADD=libmediastreamer.la 

#test program to test video4linux input plugin
test_v4l_SOURCES=test_v4l.c
test_v4l_LDADD=libmediastreamer.la 

#videoserver_SOURCES=videoserver.c
#videoserver_LDADD=libmediastreamer.la

#videoclient_SOURCES=videoclient.c
#videoclient_LDADD=libmediastreamer.la


#the mediastream program that runs a processing that will be used in linphone
mediastream_SOURCES=mediastream.c 
mediastream_LDADD=libmediastreamer.la 


ORTP_CFLAGS=`cat $(top_builddir)/oRTP/ortp.defs`

AM_CFLAGS=$(GLIB_CFLAGS) -DG_LOG_DOMAIN=\"MediaStreamer\" $(TRUESPEECH_CFLAGS) $(IPV6_CFLAGS) $(ORTP_CFLAGS) \
			$(VIDEO_CFLAGS) 

INCLUDES=  -I$(top_srcdir) \
						-I$(top_srcdir)/mediastreamer \
						-I$(top_srcdir)/oRTP/include \
						-I$(top_srcdir)/gsmlib   \
						-I$(top_srcdir)/lpc10-1.5    \
						$(SPEEX_CFLAGS) \
						$(TRUESPEECH_INCLUDES) 

linphone_includedir=$(includedir)/linphone

linphone_include_HEADERS=sndcard.h
