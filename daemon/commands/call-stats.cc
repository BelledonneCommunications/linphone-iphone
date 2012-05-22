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

	ostringstream ostr;
	ostr << CallStatsResponse(app, call, linphone_call_get_audio_stats(call), false).getBody();
	ostr << CallStatsResponse(app, call, linphone_call_get_video_stats(call), false).getBody();
	app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
}
