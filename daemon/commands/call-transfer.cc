#include "call-transfer.h"

CallTransfer::CallTransfer() :
	DaemonCommand("call-transfer",
				  "call-transfer <call id> <sip url>",
				  "Transfer a call that you aswered to another party")
{
	addExample(new DaemonCommandExample("call-transfer 1 sip:john",
										"Status: Ok\n\n"
										"Call ID: 1\n"
										"Transfer to: sip:john"));

	addExample(new DaemonCommandExample("call-transfer 2 sip:john",
										"Status: Error\n"
										"Reason: No call with such id."));
}

void CallTransfer::exec(Daemon* app, const char* args)
{
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call = NULL;
	char sipurl[512];
	memset(sipurl, 0x00, 512);

	if (sscanf(args, "%i %511s", &cid, sipurl) == 2) {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	} else {
		app->sendResponse(Response("Invalid command format."));
		return;
	}

	if( linphone_core_transfer_call(lc, call, sipurl) == 0 ) {
		std::ostringstream ostr;
		ostr << "Call ID: "     << cid    << "\n";
		ostr << "Transfer to: " << sipurl << "\n";
		app->sendResponse(Response(ostr.str(), Response::Ok));
	} else {
		app->sendResponse(Response("Error pausing call"));
	}
}
