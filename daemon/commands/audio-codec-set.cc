/*
audio-codec-set.cc
Copyright (C) 2016 Belledonne Communications, Grenoble, France 

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "audio-codec-set.h"

#include "private.h"

/*hack, until this function comes to linphonecore*/
#define _payload_type_get_number(pt)	((long)(pt)->user_data)
#define _payload_type_set_number(pt,n)	(pt)->user_data=(void*)(long)(n)

using namespace std;

AudioCodecSetCommand::AudioCodecSetCommand() :
		DaemonCommand("audio-codec-set", "audio-codec-set <payload_type_number>|<mime_type> <property> <value>",
				"Set a property (number, clock_rate, recv_fmtp, send_fmtp, bitrate (in kbps/s)) of a codec. Numbering of payload type is automatically performed at startup, any change will be lost after restart.\n"
				"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1") {
	addExample(new DaemonCommandExample("audio-codec-set 9 number 18",
						"Status: Ok\n\n"
						"Index: 10\n"
						"Payload-type-number: 18\n"
						"Clock-rate: 8000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: false"));
	addExample(new DaemonCommandExample("audio-codec-set G722/8000/1 number 9",
						"Status: Ok\n\n"
						"Index: 10\n"
						"Payload-type-number: 9\n"
						"Clock-rate: 8000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: false"));
	addExample(new DaemonCommandExample("audio-codec-set 9 clock_rate 16000",
						"Status: Ok\n\n"
						"Index: 10\n"
						"Payload-type-number: 9\n"
						"Clock-rate: 16000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: false"));
}

static PayloadType *findPayload(LinphoneCore *lc, int payload_type, int *index){
	if (index) *index=0;
	for (const bctbx_list_t *node = linphone_core_get_audio_codecs(lc); node != NULL; node = bctbx_list_next(node)) {
		PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
		if (index) (*index)++;
		if (payload_type == linphone_core_get_payload_type_number(lc, payload)) {
			return payload;
		}
	}
	return NULL;
}

void AudioCodecSetCommand::exec(Daemon *app, const string& args) {
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
	string param;
	string value;
	ist >> param;
	if (ist.fail()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s).", Response::Error));
		return;
	}
	ist >> value;
	if (value.length() > 255) value.resize(255);

	PayloadType *payload=parser.getPayloadType();
	if (payload) {
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
				int idx=atoi(value.c_str());
				PayloadType *conflict=NULL;
				if (idx!=-1){
					conflict=findPayload(app->getCore(), atoi(value.c_str()), NULL);
				}
				if (conflict) {
					app->sendResponse(Response("New payload type number is already used.", Response::Error));
				} else {
					linphone_core_set_payload_type_number(app->getCore(),payload, idx);
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, parser.getPosition()));
				}
				return;
			}
		} else if (param.compare("bitrate") == 0) {
			linphone_core_set_payload_type_bitrate(app->getCore(), payload, atoi(value.c_str()));
			handled=true;
		}
		if (handled) {
			app->sendResponse(PayloadTypeResponse(app->getCore(), payload, parser.getPosition()));
		} else {
			app->sendResponse(Response("Invalid codec parameter.", Response::Error));
		}
		return;
	}
	app->sendResponse(Response("Audio codec not found.", Response::Error));
}
