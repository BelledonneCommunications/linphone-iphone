#include "call-stats.h"

#include <sstream>

using namespace std;

CallStatsCommand::CallStatsCommand() :
		DaemonCommand("call-stats", "call-stats <call id>", "Return all stats of a call.") {
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

	LinphoneCallState call_state = LinphoneCallIdle;
	call_state = linphone_call_get_state(call);
	const LinphoneCallParams *callParams = linphone_call_get_current_params(call);
	const PayloadType *audioCodec = linphone_call_params_get_used_audio_codec(callParams);
	const PayloadType *videoCodec = linphone_call_params_get_used_video_codec(callParams);
	ostringstream ostr;
	ostr << linphone_call_state_to_string(call_state) << "\n";

	switch (call_state) {
	case LinphoneCallStreamsRunning:
	case LinphoneCallConnected:
		if (audioCodec != NULL)
			ostr << PayloadTypeResponse(app->getCore(), audioCodec, -1, "Audio-", false).getBody() << "\n";
		if (videoCodec != NULL)
			ostr << PayloadTypeResponse(app->getCore(), videoCodec, -1, "Audio-", false).getBody() << "\n";
		break;
	default:
		break;
	}
	app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
}
