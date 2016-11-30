/*
audio-stream-stats.cc
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

#include "audio-stream-stats.h"

using namespace std;

AudioStreamStatsCommand::AudioStreamStatsCommand() :
		DaemonCommand("audio-stream-stats", "audio-stream-stats <stream_id>", "Return stats of a given audio stream.") {
	addExample(new DaemonCommandExample("audio-stream-stats 1",
						"Status: Ok\n\n"
						"Audio-ICE state: Not activated\n"
						"Audio-RoundTripDelay: 0.0859833\n"
						"Audio-Jitter: 296\n"
						"Audio-JitterBufferSizeMs: 47.7778\n"
						"Audio-Received-InterarrivalJitter: 154\n"
						"Audio-Received-FractionLost: 0\n"
						"Audio-Sent-InterarrivalJitter: 296\n"
						"Audio-Sent-FractionLost: 0\n"
						"Audio-Payload-type-number: 111\n"
						"Audio-Clock-rate: 16000\n"
						"Audio-Bitrate: 44000\n"
						"Audio-Mime: speex\n"
						"Audio-Channels: 1\n"
						"Audio-Recv-fmtp: vbr=on\n"
						"Audio-Send-fmtp: vbr=on"));
	addExample(new DaemonCommandExample("audio-stream-stats 2",
						"Status: Error\n"
						"Reason: No audio stream with such id."));
}

void AudioStreamStatsCommand::exec(Daemon *app, const string& args) {
	int sid;
	AudioStreamAndOther *stream = NULL;
	istringstream ist(args);
	ist >> sid;
	if (ist.fail()) {
		app->sendResponse(Response("No stream specified."));
		return;
	}

	stream = app->findAudioStreamAndOther(sid);
	if (!stream) {
		app->sendResponse(Response("No audio stream with such id."));
		return;
	}

	app->sendResponse(Response(AudioStreamStatsResponse(app, stream->stream, &stream->stats, false).getBody(), Response::Ok));
}
