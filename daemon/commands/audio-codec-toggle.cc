/*
audio-codec-toggle.cc
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

#include "audio-codec-toggle.h"
#include "audio-codec-get.h"

using namespace std;

AudioCodecToggleCommand::AudioCodecToggleCommand(const char *name, const char *proto, const char *help, bool enable) :
		DaemonCommand(name, proto, help), mEnable(enable) {
}

void AudioCodecToggleCommand::exec(Daemon *app, const string& args) {
	istringstream ist(args);

	if (ist.peek() == EOF) {
		app->sendResponse(Response("Missing parameter.", Response::Error));
	} else {
		string mime_type;
		PayloadType *pt = NULL;
		ist >> mime_type;
		PayloadTypeParser parser(app->getCore(), mime_type, true);
		if (!parser.successful()) {
			app->sendResponse(Response("Incorrect mime type format.", Response::Error));
			return;
		}
		if (!parser.all()) pt = parser.getPayloadType();

		int index = 0;
		for (const bctbx_list_t *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = bctbx_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (parser.all()) {
				linphone_core_enable_payload_type(app->getCore(), payload, mEnable);
			} else {
				if (pt == payload) {
					linphone_core_enable_payload_type(app->getCore(), payload, mEnable);
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
					return;
				}
			}
			++index;
		}
		if (parser.all()) {
			AudioCodecGetCommand getCommand;
			getCommand.exec(app, "");
		} else {
			app->sendResponse(Response("Audio codec not found.", Response::Error));
		}
	}
}

AudioCodecEnableCommand::AudioCodecEnableCommand() :
		AudioCodecToggleCommand("audio-codec-enable", "audio-codec-enable <payload_type_number>|<mime_type>|ALL",
					"Enable an audio codec.\n"
					"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1", true) {
	addExample(new DaemonCommandExample("audio-codec-enable G722/8000/1",
						"Status: Ok\n\n"
						"Index: 9\n"
						"Payload-type-number: 9\n"
						"Clock-rate: 8000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: true"));
	addExample(new DaemonCommandExample("audio-codec-enable 9",
						"Status: Ok\n\n"
						"Index: 9\n"
						"Payload-type-number: 9\n"
						"Clock-rate: 8000\n"
						"Bitrate: 64000\n"
						"Mime: G722\n"
						"Channels: 1\n"
						"Recv-fmtp: \n"
						"Send-fmtp: \n"
						"Enabled: true"));
}

AudioCodecDisableCommand::AudioCodecDisableCommand() :
		AudioCodecToggleCommand("audio-codec-disable", "audio-codec-disable <payload_type_number>|<mime_type>|ALL",
					"Disable an audio codec.\n"
					"<mime_type> is of the form mime/rate/channels, eg. speex/16000/1", false) {
	addExample(new DaemonCommandExample("audio-codec-disable G722/8000/1",
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
	addExample(new DaemonCommandExample("audio-codec-disable 9",
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
