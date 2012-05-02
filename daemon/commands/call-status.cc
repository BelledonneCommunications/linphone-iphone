#include "call-status.h"

#include <sstream>

using namespace std;

CallStatusCommand::CallStatusCommand() :
		DaemonCommand("call-status", "call-status <call id>", "Return status of a call.") {
}
void CallStatusCommand::exec(Daemon *app, const char *args) {
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
	const LinphoneAddress *remoteAddress = linphone_call_get_remote_address(call);
	ostringstream ostr;
	ostr << linphone_call_state_to_string(call_state) << "\n";

	switch (call_state) {
	case LinphoneCallOutgoingInit:
	case LinphoneCallOutgoingProgress:
	case LinphoneCallOutgoingRinging:
	case LinphoneCallPaused:
	case LinphoneCallStreamsRunning:
	case LinphoneCallConnected:
	case LinphoneCallIncomingReceived:
		ostr << "From: " << linphone_address_as_string(remoteAddress) << "\n";
		break;
	default:
		break;
	}
	switch (call_state) {
	case LinphoneCallStreamsRunning:
	case LinphoneCallConnected:
		ostr << "Direction: " << ((linphone_call_get_dir(call) == LinphoneCallOutgoing) ? "out" : "in") << "\n";
		ostr << "Duration: " << linphone_call_get_duration(call) << "\n";
		break;
	default:
		break;
	}
	app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
}
