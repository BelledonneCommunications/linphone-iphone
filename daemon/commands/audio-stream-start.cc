#include "audio-stream-start.h"
#include "private.h"

using namespace std;

AudioStreamStartCommand::AudioStreamStartCommand() :
		DaemonCommand("audio-stream-start", "audio-stream-start <remote ip> <remote port> <payload type number>", "Start an audio stream.") {
	addExample(new DaemonCommandExample("audio-stream-start 192.168.1.28 7078 9",
						"Status: Ok\n\n"
						"Id: 1"));
}

void AudioStreamStartCommand::exec(Daemon *app, const char *args) {
	char addr[256];
	int port;
	int payload_type;
	RtpProfile *default_profile=app->getCore()->default_profile;
	if (sscanf(args, "%255s %d %d", addr, &port, &payload_type) == 3) {
		int local_port = linphone_core_get_audio_port(app->getCore());
		int jitt = linphone_core_get_audio_jittcomp(app->getCore());
		bool_t echo_canceller = linphone_core_echo_cancellation_enabled(app->getCore());
		int ptime=linphone_core_get_upload_ptime(app->getCore());
		MSSndCardManager *manager = ms_snd_card_manager_get();
		MSSndCard *capture_card = ms_snd_card_manager_get_card(manager, linphone_core_get_capture_device(app->getCore()));
		MSSndCard *play_card = ms_snd_card_manager_get_card(manager, linphone_core_get_playback_device(app->getCore()));
		PayloadType *oldpt=rtp_profile_get_payload(default_profile,payload_type);
		PayloadType *pt;
		AudioStream *stream = audio_stream_new(local_port, local_port + 1, linphone_core_ipv6_enabled(app->getCore()));
		
		if (oldpt){
			if (ptime>0){
				char fmtp[256];
				snprintf(fmtp,sizeof(fmtp)-1,"ptime=%i",ptime);
				pt=payload_type_clone(oldpt);
				payload_type_append_send_fmtp(pt,fmtp);
				rtp_profile_set_payload(default_profile,payload_type,pt);
			}
		}
		int err=audio_stream_start_now(stream, default_profile, addr, port, port + 1, payload_type, jitt, play_card, capture_card, echo_canceller);
		if (oldpt) rtp_profile_set_payload(default_profile,payload_type,oldpt);
		if (err != 0) {
			app->sendResponse(Response("Error during audio stream creation."));
			return;
		}
		ostringstream ostr;
		ostr << "Id: " << app->updateAudioStreamId(stream) << "\n";
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
