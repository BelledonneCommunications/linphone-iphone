#include "call-pause.h"

CallPause::CallPause()
{
}

void CallPause::exec(Daemon* app, const char* args)
{
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

	if( linphone_core_pause_call(lc, call) == 0 ) {
		app->sendResponse(Response("Call was paused", Response::Ok));
	} else {
		app->sendResponse(Response("Error pausing call"));
	}
}
