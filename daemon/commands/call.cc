#include "call.h"

#include <sstream>

using namespace std;

CallCommand::CallCommand() :
		DaemonCommand("call", "call <sip address>", "Place a call.") {
}

void CallCommand::exec(Daemon *app, const char *args) {
	LinphoneCall *call;
	call = linphone_core_invite(app->getCore(), args);
	if (call == NULL) {
		app->sendResponse(Response("Call creation failed."));
	} else {
		Response resp;
		ostringstream ostr;
		ostr << "Id: " << app->setCallId(call) << "\n";
		resp.setBody(ostr.str().c_str());
		app->sendResponse(resp);
	}
}
