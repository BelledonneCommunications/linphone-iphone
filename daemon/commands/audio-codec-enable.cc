#include "audio-codec-enable.h"

using namespace std;

AudioCodecEnableCommand::AudioCodecEnableCommand() :
		DaemonCommand("audio-codec-enable", "audio-codec-enable <payload_type_number|mime_type>",
				"Enable an audio codec.\n"
				"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1") {
}

void AudioCodecEnableCommand::exec(Daemon *app, const char *args) {
	istringstream ist(args);

	if (ist.peek() == EOF) {
		app->sendResponse(Response("Missing parameter.", Response::Error));
	} else {
		string mime_type;
		int ptnum;
		ist >> mime_type;
		PayloadTypeParser parser(app->getCore(), mime_type);
		if (!parser.successful()) {
			app->sendResponse(Response("Incorrect mime type format.", Response::Error));
			return;
		}
		ptnum = parser.payloadTypeNumber();

		int index = 0;
		for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (ptnum == linphone_core_get_payload_type_number(app->getCore(), payload)) {
				linphone_core_enable_payload_type(app->getCore(), payload, true);
				app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
				return;
			}
			++index;
		}
		app->sendResponse(Response("Audio codec not found.", Response::Error));
	}
}
