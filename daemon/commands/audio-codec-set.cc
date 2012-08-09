#include "audio-codec-set.h"

/*hack, until this function comes to linphonecore*/
#define _payload_type_set_number(pt,n)	(pt)->user_data=(void*)(long)(n)

using namespace std;

AudioCodecSetCommand::AudioCodecSetCommand() :
		DaemonCommand("audio-codec-set", "audio-codec-set <payload type number> <param> <value>", "Set a property(number,clock_rate, recv_fmtp, send_fmtp) of a codec") {
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
	int payload_type;
	char param[256], value[256];
	if (sscanf(args, "%d %255s %255s", &payload_type, param, value) == 3) {
		PayloadType *payload;
		int index;
		if ((payload=findPayload(app->getCore(),payload_type,&index))!=NULL){
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
			}else if (strcmp("number", param) == 0) {
				PayloadType *conflict=findPayload(app->getCore(),atoi(value),NULL);
				if (conflict){
					app->sendResponse(Response("New payload type number is already used."));
				}else{
					_payload_type_set_number(payload, atoi(value));
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
				}
				return;
			}
			if (handled) {
				app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
			} else {
				app->sendResponse(Response("Invalid codec parameter"));
			}
			return;
		}
		app->sendResponse(Response("Audio codec not found."));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
