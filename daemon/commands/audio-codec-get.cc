#include "audio-codec-get.h"

using namespace std;

AudioCodecGetCommand::AudioCodecGetCommand() :
		DaemonCommand("audio-codec-get", "audio-codec-get <payload type number>", "Get an audio codec if codec-mime is defined, otherwise return the audio codec list.") {
}
void AudioCodecGetCommand::exec(Daemon *app, const char *args) {
	int payload_type;
	bool list = sscanf(args, "%d", &payload_type) != 1;
	bool find = list;
	int index = 0;
	ostringstream ostr;
	for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
		PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
		if (list) {
			ostr << PayloadTypeResponse(app->getCore(), payload, index).getBody() << "\n";
		} else if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
			ostr << PayloadTypeResponse(app->getCore(), payload, index).getBody();
			find = true;
			break;
		}
		++index;
	}

	if (!find) {
		app->sendResponse(Response("Audio codec not found."));
	} else {
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	}
}
