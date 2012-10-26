#include "call.h"

using namespace std;

CallCommand::CallCommand() :
		DaemonCommand("call", "call <sip address>", "Place a call.") {
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org",
						"Status: Ok\n\n"
						"Id: 1"));
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org",
						"Status: Error\n"
						"Reason: Call creation failed."));
}

void CallCommand::exec(Daemon *app, const char *args) {
	LinphoneCall *call;
	call = linphone_core_invite(app->getCore(), args);
	if (call == NULL) {
		app->sendResponse(Response("Call creation failed."));
	} else {
		Response resp;
		ostringstream ostr;
		ostr << "Id: " << app->updateCallId(call) << "\n";
		resp.setBody(ostr.str().c_str());
		app->sendResponse(resp);
	}
}
