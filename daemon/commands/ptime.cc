#include "ptime.h"

#include <sstream>

using namespace std;

PtimeCommand::PtimeCommand() :
		DaemonCommand("ptime", "ptime <ms>", "Set the if ms is defined, otherwise return the ptime.") {
}
void PtimeCommand::exec(Daemon *app, const char *args) {
	int ms;
	int ret = sscanf(args, "%d", &ms);
	if (ret <= 0) {
		ostringstream ostr;
		ms = linphone_core_get_upload_ptime(app->getCore());
		ostr << "Ptime: " << ms << "\n";
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	} else if (ret == 1) {
		ostringstream ostr;
		linphone_core_set_upload_ptime(app->getCore(), ms);
		ms = linphone_core_get_upload_ptime(app->getCore());
		ostr << "Ptime: " << ms << "\n";
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
