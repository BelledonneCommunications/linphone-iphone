#include "call.h"

using namespace std;

CallCommand::CallCommand() :
		DaemonCommand("call", "call <sip address>", "Place a call.") {
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org",
						"Status: Ok\n\n"
						"Id: 1"));
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org --early-media",
						"Status: Ok\n\n"
						"Early media: Ok\n"
						"Id: 1"));
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org",
						"Status: Error\n"
						"Reason: Call creation failed."));
}

void CallCommand::exec(Daemon *app, const char *args) {
	LinphoneCall *call;
	Response resp;
	ostringstream ostr;
	char address[256] = { 0 }, early_media[256] = { 0 };
	if (sscanf(args, "%s %s", address, early_media) == 2) {
		char *opt;
		LinphoneCallParams *cp;
		opt = strstr(early_media,"--early-media");
		cp = linphone_core_create_call_params(app->getCore(), NULL);
		if (opt) {
			linphone_call_params_enable_early_media_sending(cp, TRUE);
			ostr << "Early media: Ok\n";
		}
		call = linphone_core_invite_with_params(app->getCore(), address, cp);
	} else {
		call = linphone_core_invite(app->getCore(), args);
	}
	
	if (call == NULL) {
		app->sendResponse(Response("Call creation failed."));
	} else {
		ostr << "Id: " << app->updateCallId(call) << "\n";
		resp.setBody(ostr.str().c_str());
		app->sendResponse(resp);
	}
}
