/*
audio-stream-start.cc
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

#include "audio-stream-start.h"
#include "private.h"

using namespace std;

AudioStreamStartCommand::AudioStreamStartCommand() :
		DaemonCommand("audio-stream-start", "audio-stream-start <remote_ip> <remote_port> <payload_type_number>", "Start an audio stream.") {
	addExample(new DaemonCommandExample("audio-stream-start 192.168.1.28 7078 9",
						"Status: Ok\n\n"
						"Id: 1"));
}

static PayloadType *getPayloadType(LinphoneCore *lc, const MSList *codecs, int number){
	const MSList *elem;
	for (elem = codecs; elem != NULL; elem = elem->next) {
		PayloadType *pt = (PayloadType*)elem->data;
		if (linphone_core_get_payload_type_number(lc, pt) == number)
			return pt;
	}
	return NULL;
}

void AudioStreamStartCommand::exec(Daemon *app, const string& args) {
	string addr;
	int port;
	int payload_type;
	MSFactory* factory = app->getCore()->factory; 

	istringstream ist(args);
	ist >> addr;
	ist >> port;
	ist >> payload_type;
	if (ist.fail()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		return;
	}

	int local_port = linphone_core_get_audio_port(app->getCore());
	int jitt = linphone_core_get_audio_jittcomp(app->getCore());
	bool_t echo_canceller = linphone_core_echo_cancellation_enabled(app->getCore());
	int ptime = linphone_core_get_upload_ptime(app->getCore());
	MSSndCardManager *manager = ms_factory_get_snd_card_manager(factory);
	MSSndCard *capture_card = ms_snd_card_manager_get_card(manager, linphone_core_get_capture_device(app->getCore()));
	MSSndCard *play_card = ms_snd_card_manager_get_card(manager, linphone_core_get_playback_device(app->getCore()));
	RtpProfile *prof = rtp_profile_new("stream");
	PayloadType *pt = getPayloadType(app->getCore(), linphone_core_get_audio_codecs(app->getCore()), payload_type);
		
	if (!pt){
		app->sendResponse(Response("No payload type were assigned to this number."));
		return;
	}
	AudioStream *stream = audio_stream_new(factory, local_port, local_port + 1, linphone_core_ipv6_enabled(app->getCore()));
	audio_stream_set_features(stream, linphone_core_get_audio_features(app->getCore()));
		
	pt = payload_type_clone(pt);
	if (ptime != 0) {
		char fmtp[32];
		snprintf(fmtp, sizeof(fmtp) - 1, "ptime=%i", ptime);
		payload_type_append_send_fmtp(pt, fmtp);
	}
	rtp_profile_set_payload(prof, payload_type,pt);
	if (linphone_core_generic_comfort_noise_enabled(app->getCore())){
		rtp_profile_set_payload(prof, 13, payload_type_clone(&payload_type_cn));
	}
	audio_stream_enable_adaptive_jittcomp(stream, linphone_core_audio_adaptive_jittcomp_enabled(app->getCore()));
	rtp_session_set_symmetric_rtp(stream->ms.sessions.rtp_session, linphone_core_symmetric_rtp_enabled(app->getCore()));

	int err = audio_stream_start_now(stream, prof, addr.c_str(), port, port + 1, payload_type, jitt, play_card, capture_card, echo_canceller);
	if (err != 0) {
		app->sendResponse(Response("Error during audio stream creation."));
		return;
	}

	ostringstream ostr;
	ostr << "Id: " << app->updateAudioStreamId(stream) << "\n";
	app->sendResponse(Response(ostr.str(), Response::Ok));
}
