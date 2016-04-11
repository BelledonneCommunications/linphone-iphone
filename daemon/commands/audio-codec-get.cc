#include "audio-codec-get.h"

using namespace std;

AudioCodecGetCommand::AudioCodecGetCommand() :
		DaemonCommand("audio-codec-get", "audio-codec-get <payload_type_number|mime_type>",
				"Get an audio codec if a parameter is given, otherwise return the audio codec list.\n"
				"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1") {
	addExample(new DaemonCommandExample("audio-codec-get 9",
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
	addExample(new DaemonCommandExample("audio-codec-get G722/8000/1",
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
	addExample(new DaemonCommandExample("audio-codec-get 2",
						"Status: Error\n"
						"Reason: Audio codec not found."));
}

void AudioCodecGetCommand::exec(Daemon *app, const char *args) {
	bool list = false;
	bool found = false;
	istringstream ist(args);
	ostringstream ost;
	PayloadType *pt=NULL;

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
		pt = parser.getPayloadType();
	}

	int index = 0;
	for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
		PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
		if (list) {
			ost << PayloadTypeResponse(app->getCore(), payload, index).getBody() << "\n";
		} else if (pt == payload) {
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
