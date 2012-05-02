#include "answer.h"

using namespace std;

AnswerCommand::AnswerCommand() :
		DaemonCommand("answer", "answer <call id>", "Answer an incoming call.") {
}
void AnswerCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call;
	if (sscanf(args, "%i", &cid) == 1) {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		} else {
			LinphoneCallState cstate = linphone_call_get_state(call);
			if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallIncomingEarlyMedia) {
				if (linphone_core_accept_call(lc, call) == 0) {
					app->sendResponse(Response());
					return;
				}
			}
			app->sendResponse(Response("Can't accept this call."));
			return;
		}
	} else {
		for (const MSList* elem = linphone_core_get_calls(lc); elem != NULL; elem = elem->next) {
			call = (LinphoneCall*) elem->data;
			LinphoneCallState cstate = linphone_call_get_state(call);
			if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallIncomingEarlyMedia) {
				if (linphone_core_accept_call(lc, call) == 0) {
					app->sendResponse(Response());
					return;
				}
			}
		}
	}
	app->sendResponse(Response("No call to accept."));
}
