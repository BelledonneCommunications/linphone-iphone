#include "terminate.h"

using namespace std;

TerminateCommand::TerminateCommand() :
		DaemonCommand("terminate", "terminate <call id>", "Terminate a call.") {
	addExample(new DaemonCommandExample("terminate 2",
						"Status: Error\n"
						"Reason: No call with such id."));
	addExample(new DaemonCommandExample("terminate 1",
						"Status: Ok\n"));
	addExample(new DaemonCommandExample("terminate",
						"Status: Ok\n"));
	addExample(new DaemonCommandExample("terminate",
						"Status: Error\n"
						"Reason: No active call."));
}
void TerminateCommand::exec(Daemon *app, const char *args) {
	LinphoneCall *call = NULL;
	int cid;
	const MSList *elem;
	if (sscanf(args, "%i", &cid) == 1) {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	} else {
		elem = linphone_core_get_calls(app->getCore());
		if (elem != NULL && elem->next == NULL) {
			call = (LinphoneCall*) elem->data;
		}
	}
	if (call == NULL) {
		app->sendResponse(Response("No active call."));
		return;
	}
	linphone_core_terminate_call(app->getCore(), call);
	app->sendResponse(Response());
}
