
TESTER_SOUNDS = sounds/ahbahouaismaisbon.wav \
				sounds/hello8000.wav  \
				sounds/hello44100.wav  \
				sounds/oldphone.wav \
				sounds/sintel_trailer_opus_h264.mkv \
				sounds/sintel_trailer_pcmu_h264.mkv \
				sounds/hello8000.mkv \
				sounds/hello8000_mkv_ref.wav\
				sounds/ringback.wav  \
				sounds/sintel_trailer_opus_vp8.mkv \
				sounds/vrroom.wav


SIPP_FILES = sipp/call_invite_200ok_without_contact_header.xml       \
			sipp/call_with_multiple_audio_mline_in_sdp.xml          \
			sipp/call_with_video_mline_before_audio_in_sdp.xml\
			sipp/call_with_audio_mline_before_video_in_sdp.xml   \
			sipp/call_with_multiple_video_mline_in_sdp.xml      \
			sipp/sip_update_within_icoming_reinvite_with_no_sdp.xml


CERTIFICATE_ALT_FILES = certificates/altname/agent.pem           \
					certificates/altname/cafile.pem          \
					certificates/altname/openssl-altname.cnf

CERTIFICATE_CN_FILES = certificates/cn/agent.pem      \
					 certificates/cn/cafile.pem     \
					 certificates/cn/openssl-cn.cnf

CERTIFICATE_CLIENT_FILES = certificates/client/cert.pem \
					certificates/client/key.pem

CERTIFICATE_FILES = $(CERTIFICATE_ALT_FILES) $(CERTIFICATE_CN_FILES) $(CERTIFICATE_CLIENT_FILES)

RCFILES = \
					rcfiles/empty_rc\
					rcfiles/laure_call_logs_rc\
					rcfiles/laure_rc_udp\
					rcfiles/marie_early_rc\
					rcfiles/marie_h264_rc\
					rcfiles/marie_quality_reporting_rc\
					rcfiles/marie_rc\
					rcfiles/marie2_rc\
					rcfiles/marie_rc_rtcp_xr\
					rcfiles/marie_remote_404_rc\
					rcfiles/marie_remote_default_values_rc\
					rcfiles/marie_remote_https_rc\
					rcfiles/marie_remote_invalid_rc\
					rcfiles/marie_remote_invalid_uri_rc\
					rcfiles/marie_remote_localfile2_rc\
					rcfiles/marie_remote_localfile_android_rc\
					rcfiles/marie_remote_localfile_rc\
					rcfiles/marie_remote_localfile_win10_rc\
					rcfiles/marie_remote_rc\
					rcfiles/marie_sips_rc\
					rcfiles/marie_transient_remote_rc\
					rcfiles/marie_zrtp_aes256_rc\
					rcfiles/marie_zrtp_b256_rc\
					rcfiles/marie_zrtp_srtpsuite_aes256_rc\
					rcfiles/michelle_rc_udp\
					rcfiles/multi_account_rc\
					rcfiles/pauline_alt_rc\
					rcfiles/pauline_h264_rc\
					rcfiles/pauline_rc\
					rcfiles/pauline_rc_rtcp_xr\
					rcfiles/pauline_sips_rc\
					rcfiles/pauline_tcp_rc\
					rcfiles/pauline_tls_client_rc\
					rcfiles/pauline_wild_rc\
					rcfiles/pauline_zrtp_aes256_rc\
					rcfiles/pauline_zrtp_b256_rc\
					rcfiles/pauline_zrtp_srtpsuite_aes256_rc\
					rcfiles/remote_zero_length_params_rc\
					rcfiles/stun_rc\
					rcfiles/upnp_rc\
					rcfiles/zero_length_params_rc\
					rcfiles/friends_rc\
					rcfiles/carddav_rc

IMAGE_FILES = images/nowebcamCIF.jpg images/nowebcamVGA.jpg

VCARDS_FILE = vcards/vcards.vcf vcards/thousand_vcards.vcf

EXTRA_DIST = tester_hosts\
			messages.db\
			$(TESTER_SOUNDS)\
			$(SIPP_FILES)\
			$(CERTIFICATE_FILES)\
			$(RCFILES)\
			$(IMAGE_FILES)\
			$(VCARDS_FILE)

if ENABLE_TESTS

sounddir = $(datadir)/liblinphone_tester/sounds
sound_DATA = $(TESTER_SOUNDS)
sippdir = $(datadir)/liblinphone_tester/sipp
sipp_DATA = $(SIPP_FILES)
certificatesaltdir=$(datadir)/liblinphone_tester/certificates/altname
certificatesalt_DATA = $(CERTIFICATE_ALT_FILES)
certificatescndir=$(datadir)/liblinphone_tester/certificates/cn
certificatescn_DATA = $(CERTIFICATE_CN_FILES)
certificatesclientdir=$(datadir)/liblinphone_tester/certificates/client
certificatesclient_DATA = $(CERTIFICATE_CLIENT_FILES)
rcfilesdir = $(datadir)/liblinphone_tester/rcfiles
rcfiles_DATA = $(RCFILES)
imagesdir = $(datadir)/liblinphone_tester/images
images_DATA = $(IMAGE_FILES)
vcardsdir = $(datadir)/liblinphone_tester/vcards
vcards_DATA = $(VCARDS_FILE)

# there are 2 targets: liblinphonetester.la and the executable liblinphone_tester

liblinphonedir = $(includedir)/linphone
liblinphone_HEADERS = liblinphone_tester.h


lib_LTLIBRARIES = liblinphonetester.la

liblinphonetester_la_HEADERS = audio_bypass_wav_header.h

liblinphonetester_la_SOURCES = \
				accountmanager.c \
				audio_bypass_tester.c \
				call_multi_tester.c \
				call_multicast_tester.c \
				call_single_tester.c \
				call_video_tester.c \
				complex_sip_case_tester.c \
				dtmf_tester.c \
				eventapi_tester.c \
				flexisip_tester.c \
				log_collection_tester.c \
				message_tester.c \
				offeranswer_tester.c \
				player_tester.c \
				presence_server_tester.c \
				presence_tester.c \
				proxy_config_tester.c \
				quality_reporting_tester.c \
				register_tester.c \
				remote_provisioning_tester.c \
				setup_tester.c \
				stun_tester.c \
				tester.c \
				tunnel_tester.c \
				upnp_tester.c \
				vcard_tester.c \
				video_tester.c

liblinphonetester_ladir = $(includedir)/linphone

liblinphonetester_la_LDFLAGS= -no-undefined
liblinphonetester_la_LIBADD= ../coreapi/liblinphone.la $(BCTOOLBOXTESTER_LIBS)

liblinphone_tester_bindir = $(bindir)/liblinphone_tester
liblinphone_testerdir = $(datadir)/liblinphone_tester

dist_liblinphone_tester_DATA =  tester_hosts messages.db


AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/coreapi
AM_CFLAGS    = -DBC_CONFIG_FILE=\"config.h\" $(STRICT_OPTIONS) $(STRICT_OPTIONS_CC) \
		$(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS) $(BCTOOLBOXTESTER_CFLAGS) \
		$(BELLESIP_CFLAGS) $(LIBXML2_CFLAGS) $(SQLITE3_CFLAGS) $(BELCARD_CFLAGS)

if BUILD_GTK_UI

liblinphonetester_la_LIBADD += $(LIBGTK_LIBS) $(LIBGTKMAC_LIBS)
AM_CFLAGS += $(LIBGTK_CFLAGS) $(LIBGTKMAC_CFLAGS) -DHAVE_GTK

endif

#AM_LDFLAGS=-Wl,-rpath -Wl,$(libdir)

bin_PROGRAMS=

if !BUILD_IOS
#noinst_PROGRAMS = liblinphone_tester
bin_PROGRAMS += liblinphone_tester

liblinphone_tester_SOURCES = liblinphone_tester.c
liblinphone_tester_LDADD   = $(top_builddir)/coreapi/liblinphone.la liblinphonetester.la -lm

endif


test: liblinphone_tester
	./liblinphone_tester  --config $(abs_srcdir) $(TEST_OPTIONS)

else !ENABLE_TESTS

test:
	@echo "bctoolbox-tester must be installed to be able to run the tests!"

endif !ENABLE_TESTS
