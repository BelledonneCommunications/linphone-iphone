#include "ipv6.h"

using namespace std;

class IPv6Response : public Response {
public:
	IPv6Response(LinphoneCore *core);
};

IPv6Response::IPv6Response(LinphoneCore *core) : Response() {
	ostringstream ost;
	bool ipv6_enabled = linphone_core_ipv6_enabled(core) == TRUE ? true : false;
	ost << "State: ";
	if (ipv6_enabled) {
		ost << "enabled\n";
	} else {
		ost << "disabled\n";
	}
	setBody(ost.str().c_str());
}

IPv6Command::IPv6Command() :
		DaemonCommand("ipv6", "ipv6 [enable|disable]",
				"Enable or disable IPv6 respectively with the 'enable' and 'disable' parameters, return the status of the use of IPv6 without parameter.") {
	addExample(new DaemonCommandExample("ipv6 enable",
						"Status: Ok\n\n"
						"State: enabled"));
	addExample(new DaemonCommandExample("ipv6 disable",
						"Status: Ok\n\n"
						"State: disabled"));
	addExample(new DaemonCommandExample("ipv6",
						"Status: Ok\n\n"
						"State: disabled"));
}

void IPv6Command::exec(Daemon *app, const char *args) {
	string status;
	istringstream ist(args);
	ist >> status;
	if (ist.fail()) {
		app->sendResponse(IPv6Response(app->getCore()));
	} else {
		if (status.compare("enable") == 0) {
			linphone_core_enable_ipv6(app->getCore(), TRUE);
		} else if (status.compare("disable") == 0) {
			linphone_core_enable_ipv6(app->getCore(), FALSE);
		} else {
			app->sendResponse(Response("Incorrect parameter.", Response::Error));
			return;
		}
		app->sendResponse(IPv6Response(app->getCore()));
	}
}
