#include "audio-codec-disable.h"

using namespace std;

AudioCodecDisableCommand::AudioCodecDisableCommand() :
		DaemonCommand("audio-codec-disable", "audio-codec-disable <payload type number>", "Disable an audio codec.") {
}

void AudioCodecDisableCommand::exec(Daemon *app, const char *args) {
	int payload_type;
	if (sscanf(args, "%d", &payload_type) == 1) {
		int index = 0;
		for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
				linphone_core_enable_payload_type(app->getCore(), payload, false);
				app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
				return;
			}
			++index;
		}
		app->sendResponse(Response("Audio codec not found."));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}

