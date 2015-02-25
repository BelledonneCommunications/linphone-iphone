#include "cn.h"

using namespace std;

class CNResponse : public Response {
public:
	CNResponse(LinphoneCore *core);
};

CNResponse::CNResponse(LinphoneCore *core) : Response() {
	ostringstream ost;
	bool cn_enabled = linphone_core_generic_confort_noise_enabled(core) == TRUE ? true : false;
	ost << "State: ";
	if (cn_enabled) {
		ost << "enabled\n";
	} else {
		ost << "disabled\n";
	}
	setBody(ost.str().c_str());
}


CNCommand::CNCommand() :
		DaemonCommand("cn", "cn [enable|disable]",
				"Enable or disable generic confort noice (CN payload type) with the 'enable' and 'disable' parameters, return the status of the use of confort noise without parameter.") {
	addExample(new DaemonCommandExample("cn enable",
						"Status: Ok\n\n"
						"State: enabled"));
	addExample(new DaemonCommandExample("cn disable",
						"Status: Ok\n\n"
						"State: disabled"));
	addExample(new DaemonCommandExample("cn",
						"Status: Ok\n\n"
						"State: disabled"));
}

void CNCommand::exec(Daemon *app, const char *args) {
	string status;
	istringstream ist(args);
	ist >> status;
	if (ist.fail()) {
		app->sendResponse(CNResponse(app->getCore()));
	} else {
		if (status.compare("enable") == 0) {
			linphone_core_enable_generic_confort_noise(app->getCore(), TRUE);
		} else if (status.compare("disable") == 0) {
			linphone_core_enable_generic_confort_noise(app->getCore(), FALSE);
		} else {
			app->sendResponse(Response("Incorrect parameter.", Response::Error));
			return;
		}
		app->sendResponse(CNResponse(app->getCore()));
	}
}
