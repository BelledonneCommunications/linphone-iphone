#include "audio-codec-set.h"

/*hack, until this function comes to linphonecore*/
#define _payload_type_set_number(pt,n)	(pt)->user_data=(void*)(long)(n)

using namespace std;

AudioCodecSetCommand::AudioCodecSetCommand() :
		DaemonCommand("audio-codec-set", "audio-codec-set <payload_type_number|mime_type> <property> <value>",
				"Set a property (number, clock_rate, recv_fmtp, send_fmtp) of a codec. Numbering of payload type is automatically performed at startup, any change will be lost after restart.\n"
				"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1") {
}

static PayloadType *findPayload(LinphoneCore *lc, int payload_type, int *index){
	if (index) *index=0;
	for (const MSList *node = linphone_core_get_audio_codecs(lc); node != NULL; node = ms_list_next(node)) {
		PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
		if (index) (*index)++;
		if (payload_type == linphone_core_get_payload_type_number(lc, payload)) {
			
			return payload;
		}
	}
	return NULL;
}

void AudioCodecSetCommand::exec(Daemon *app, const char *args) {
	istringstream ist(args);

	if (ist.peek() == EOF) {
		app->sendResponse(Response("Missing parameters.", Response::Error));
		return;
	}

	string mime_type;
	ist >> mime_type;
	PayloadTypeParser parser(app->getCore(), mime_type);
	if (!parser.successful()) {
		app->sendResponse(Response("Incorrect mime type format.", Response::Error));
		return;
	}
	int ptnum = parser.payloadTypeNumber();
	string param;
	string value;
	ist >> param;
	if (ist.fail()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s).", Response::Error));
		return;
	}
	ist >> value;
	if (value.length() > 255) value.resize(255);

	PayloadType *payload;
	int index;
	if ((payload = findPayload(app->getCore(), ptnum, &index)) != NULL) {
		bool handled = false;
		if (param.compare("clock_rate") == 0) {
			if (value.length() > 0) {
				payload->clock_rate = atoi(value.c_str());
				handled = true;
			}
		} else if (param.compare("recv_fmtp") == 0) {
			payload_type_set_recv_fmtp(payload, value.c_str());
			handled = true;
		} else if (param.compare("send_fmtp") == 0) {
			payload_type_set_send_fmtp(payload, value.c_str());
			handled = true;
		} else if (param.compare("number") == 0) {
			if (value.length() > 0) {
				PayloadType *conflict = findPayload(app->getCore(), atoi(value.c_str()), NULL);
				if (conflict) {
					app->sendResponse(Response("New payload type number is already used.", Response::Error));
				} else {
					_payload_type_set_number(payload, atoi(value.c_str()));
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
				}
				return;
			}
		}
		if (handled) {
			app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
		} else {
			app->sendResponse(Response("Invalid codec parameter.", Response::Error));
		}
		return;
	}
	app->sendResponse(Response("Audio codec not found.", Response::Error));
}
