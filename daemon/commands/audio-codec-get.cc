/*
audio-codec-get.cc
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

#include "audio-codec-get.h"

using namespace std;

AudioCodecGetCommand::AudioCodecGetCommand() :
		DaemonCommand("audio-codec-get", "audio-codec-get <payload_type_number>|<mime_type>",
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

void AudioCodecGetCommand::exec(Daemon *app, const string& args) {
	bool list = false;
	bool found = false;
	istringstream ist(args);
	ostringstream ost;
	PayloadType *pt = NULL;

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
	for (const bctbx_list_t *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = bctbx_list_next(node)) {
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
		app->sendResponse(Response(ost.str(), Response::Ok));
	}
}
