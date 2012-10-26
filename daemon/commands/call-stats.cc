#include "call-stats.h"

using namespace std;

CallStatsCommand::CallStatsCommand() :
		DaemonCommand("call-stats", "call-stats <call id>", "Return all stats of a call.") {
	addExample(new DaemonCommandExample("call-stats 1",
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
						"Audio-Send-fmtp: vbr=on\n\n"
						"Video-ICE state: Not activated\n"
						"Video-RoundTripDelay: 0\n"
						"Video-Jitter: 0\n"
						"Video-JitterBufferSizeMs: 0State: disabled"));
	addExample(new DaemonCommandExample("call-stats 2",
						"Status: Error\n"
						"Reason: No call with such id."));
	addExample(new DaemonCommandExample("call-stats",
						"Status: Error\n"
						"Reason: No current call available."));
}

void CallStatsCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call = NULL;
	if (sscanf(args, "%i", &cid) == 1) {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	} else {
		call = linphone_core_get_current_call(lc);
		if (call == NULL) {
			app->sendResponse(Response("No current call available."));
			return;
		}
	}

	ostringstream ostr;
	ostr << CallStatsResponse(app, call, linphone_call_get_audio_stats(call), false).getBody();
	ostr << CallStatsResponse(app, call, linphone_call_get_video_stats(call), false).getBody();
	app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
}
