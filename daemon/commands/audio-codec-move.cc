#include "audio-codec-move.h"

using namespace std;

AudioCodecMoveCommand::AudioCodecMoveCommand() :
		DaemonCommand("audio-codec-move", "audio-codec-move <payload_type_number|mime_type> <index>",
				"Move a codec to the specified index.\n"
				"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1") {
	addExample(new DaemonCommandExample("audio-codec-move 9 1",
						"Status: Ok\n\n"
						"Index: 1\n"
						"Payload-type-number: 9\n"
						"Clock-rate: 8000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: false"));
	addExample(new DaemonCommandExample("audio-codec-move G722/8000/1 9",
						"Status: Ok\n\n"
						"Index: 9\n"
						"Payload-type-number: 9\n"
						"Clock-rate: 8000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: false"));
}

void AudioCodecMoveCommand::exec(Daemon *app, const char *args) {
	istringstream ist(args);

	if (ist.peek() == EOF) {
		app->sendResponse(Response("Missing parameters.", Response::Error));
		return;
	}

	string mime_type;
	ist >> mime_type;
	if (ist.peek() == EOF) {
		app->sendResponse(Response("Missing index parameter.", Response::Error));
		return;
	}
	PayloadTypeParser parser(app->getCore(), mime_type);
	if (!parser.successful()) {
		app->sendResponse(Response("Incorrect mime type format.", Response::Error));
		return;
	}
	PayloadType *selected_payload = NULL;
	selected_payload = parser.getPayloadType();
	
	if (selected_payload == NULL) {
		app->sendResponse(Response("Audio codec not found.", Response::Error));
		return;
	}
	
	int index;
	ist >> index;
	if (ist.fail() || (index < 0)) {
		app->sendResponse(Response("Incorrect index parameter.", Response::Error));
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
}
