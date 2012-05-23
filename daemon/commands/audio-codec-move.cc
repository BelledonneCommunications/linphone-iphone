#include "audio-codec-move.h"

using namespace std;

AudioCodecMoveCommand::AudioCodecMoveCommand() :
		DaemonCommand("audio-codec-move", "audio-codec-move <payload type number> <index>", "Move a codec to the an index.") {
}
void AudioCodecMoveCommand::exec(Daemon *app, const char *args) {
	int payload_type;
	int index;
	if (sscanf(args, "%d %d", &payload_type, &index) == 2 && index >= 0) {
		PayloadType *selected_payload = NULL;
		for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
				selected_payload = payload;
				break;
			}
		}
		if (selected_payload == NULL) {
			app->sendResponse(Response("Audio codec not found."));
			return;
		}
		int i = 0;
		MSList *mslist = NULL;
		for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (i == index) {
				mslist = ms_list_append(mslist, selected_payload);
				++i;
			}
			if (selected_payload != payload) {
				mslist = ms_list_append(mslist, payload);
				++i;
			}
		}
		if (i <= index) {
			index = i;
			mslist = ms_list_append(mslist, selected_payload);
		}
		linphone_core_set_audio_codecs(app->getCore(), mslist);

		app->sendResponse(PayloadTypeResponse(app->getCore(), selected_payload, index));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
