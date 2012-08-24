#include "ipv6.h"

using namespace std;

class IPv6CommandPrivate {
public:
	void outputIPv6(Daemon *app, ostringstream &ost);
};

void IPv6CommandPrivate::outputIPv6(Daemon* app, ostringstream& ost) {
	bool ipv6_enabled = linphone_core_ipv6_enabled(app->getCore()) == TRUE ? true : false;
	ost << "Status: ";
	if (ipv6_enabled) {
		ost << "enabled\n";
	} else {
		ost << "disabled\n";
	}
}

IPv6Command::IPv6Command() :
		DaemonCommand("ipv6", "ipv6 [enable|disable]",
				"Enable or disable IPv6 respectively with the 'enable' and 'disable' parameters, return the status of the use of IPv6 without parameter."),
		d(new IPv6CommandPrivate()) {
}

IPv6Command::~IPv6Command() {
	delete d;
}

void IPv6Command::exec(Daemon *app, const char *args) {
	string status;
	istringstream ist(args);
	ist >> status;
	if (ist.fail()) {
		ostringstream ost;
		d->outputIPv6(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	} else {
		if (status.compare("enable") == 0) {
			linphone_core_enable_ipv6(app->getCore(), TRUE);
		} else if (status.compare("disable") == 0) {
			linphone_core_enable_ipv6(app->getCore(), FALSE);
		} else {
			app->sendResponse(Response("Incorrect parameter.", Response::Error));
			return;
		}
		ostringstream ost;
		d->outputIPv6(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	}
}
