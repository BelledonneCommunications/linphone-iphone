#include "audio-stream-stats.h"

using namespace std;

AudioStreamStatsCommand::AudioStreamStatsCommand() :
		DaemonCommand("audio-stream-stats", "audio-stream-stats <stream id>", "Return stats of a given audio stream.") {
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

void AudioStreamStatsCommand::exec(Daemon *app, const char *args) {
	int sid;
	AudioStreamAndOther *stream = NULL;
	if (sscanf(args, "%i", &sid) == 1) {
		stream = app->findAudioStreamAndOther(sid);
		if (!stream) {
			app->sendResponse(Response("No audio stream with such id."));
			return;
		}
	} else {
		app->sendResponse(Response("No stream specified."));
		return;
	}

	ostringstream ostr;
	ostr << AudioStreamStatsResponse(app, stream->stream, &stream->stats, false).getBody();
	app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
}
