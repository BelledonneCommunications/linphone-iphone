#include "audio-codec-get.h"

using namespace std;

AudioCodecGetCommand::AudioCodecGetCommand() :
		DaemonCommand("audio-codec-get", "audio-codec-get <payload_type_number|mime_type>",
				"Get an audio codec if a parameter is given, otherwise return the audio codec list.\n"
				"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1") {
}

void AudioCodecGetCommand::exec(Daemon *app, const char *args) {
	int ptnum;
	bool list = false;
	bool found = false;
	istringstream ist(args);
	ostringstream ost;

	if (ist.peek() == EOF) {
		found = list = true;
	} else {
		string mime_type;
		ist >> mime_type;
		PayloadTypeParser parser(app->getCore(), mime_type);
		if (!parser.successful()) {
			app->sendResponse(Response("Incorrect mime type format.", Response::Error));
			return;
		}
		ptnum = parser.payloadTypeNumber();
	}

	int index = 0;
	for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
		PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
		if (list) {
			ost << PayloadTypeResponse(app->getCore(), payload, index).getBody() << "\n";
		} else if (ptnum == linphone_core_get_payload_type_number(app->getCore(), payload)) {
			ost << PayloadTypeResponse(app->getCore(), payload, index).getBody();
			found = true;
			break;
		}
		++index;
	}

	if (!found) {
		app->sendResponse(Response("Audio codec not found.", Response::Error));
	} else {
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	}
}
