#include "call-resume.h"

CallResume::CallResume():
	DaemonCommand("call-resume", "call-resume <call id>", "Pause a call (pause current if no id is specified).")
{
	addExample(new DaemonCommandExample("call-resume 1",
										"Status: Ok\n\n"
										"Call was resumed"));

	addExample(new DaemonCommandExample("call-resume 2",
										"Status: Error\n"
										"Reason: No call with such id."));

	addExample(new DaemonCommandExample("call-resume",
										"Status: Error\n"
										"Reason: No current call available."));
}

void CallResume::exec(Daemon* app, const char* args)
{
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call = NULL;
	bool current = false;
	if (sscanf(args, "%i", &cid) == 1) {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	} else {
		call = linphone_core_get_current_call(lc);
		current = true;
		if (call == NULL) {
			app->sendResponse(Response("No current call available."));
			return;
		}
	}

	if( linphone_core_resume_call(lc, call) == 0 ) {
		app->sendResponse(Response(current?"Current call was resumed":
										   "Call was resumed", Response::Ok));
	} else {
		app->sendResponse(Response("Error pausing call"));
	}
}
