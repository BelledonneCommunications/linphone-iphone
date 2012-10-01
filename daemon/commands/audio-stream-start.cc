#include "audio-stream-start.h"

using namespace std;

AudioStreamStartCommand::AudioStreamStartCommand() :
		DaemonCommand("audio-stream-start", "audio-stream-start <remote ip> <remote port> <payload type number>", "Start an audio stream.") {
}

void AudioStreamStartCommand::exec(Daemon *app, const char *args) {
	char addr[256];
	int port;
	int payload_type;
	if (sscanf(args, "%255s %d %d", addr, &port, &payload_type) == 3) {
		int local_port = linphone_core_get_audio_port(app->getCore());
		const int jitt = linphone_core_get_audio_jittcomp(app->getCore());
		const bool_t echo_canceller = linphone_core_echo_cancellation_enabled(app->getCore());
		MSSndCardManager *manager = ms_snd_card_manager_get();
		MSSndCard *capture_card = ms_snd_card_manager_get_card(manager, linphone_core_get_capture_device(app->getCore()));
		MSSndCard *play_card = ms_snd_card_manager_get_card(manager, linphone_core_get_playback_device(app->getCore()));
		AudioStream *stream = audio_stream_new(local_port, local_port + 1, linphone_core_ipv6_enabled(app->getCore()));
		if (audio_stream_start_now(stream, &av_profile, addr, port, port + 1, payload_type, jitt, play_card, capture_card, echo_canceller) != 0) {
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
