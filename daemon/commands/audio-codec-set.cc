#include "audio-codec-set.h"

#include <sstream>

using namespace std;

AudioCodecSetCommand::AudioCodecSetCommand() :
		DaemonCommand("audio-codec-set", "audio-codec-set <payload type number> <param> <value>", "Set a property(clock_rate, recv_fmtp, send_fmtp) of a codec") {
}
void AudioCodecSetCommand::exec(Daemon *app, const char *args) {
	int payload_type;
	char param[256], value[256];
	if (sscanf(args, "%d %255s %255s", &payload_type, param, value) == 3) {
		int index = 0;
		for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
				bool handled = false;
				if (strcmp("clock_rate", param) == 0) {
					payload->clock_rate = atoi(value);
					handled = true;
				} else if (strcmp("recv_fmtp", param) == 0) {
					payload_type_set_recv_fmtp(payload, value);
					handled = true;
				} else if (strcmp("send_fmtp", param) == 0) {
					payload_type_set_send_fmtp(payload, value);
					handled = true;
				}
				if (handled) {
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
				} else {
					app->sendResponse(Response("Invalid codec parameter"));
				}
				return;
			}
			++index;
		}
		app->sendResponse(Response("Audio codec not found."));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
