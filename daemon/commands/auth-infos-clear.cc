#include "auth-infos-clear.h"

using namespace std;

AuthInfosClearCommand::AuthInfosClearCommand() :
		DaemonCommand("auth-infos-clear", "auth-infos-clear <auth_infos_id|ALL> ", "Remove auth infos context for the given id, or all.") {
	addExample(new DaemonCommandExample("auth-infos-clear 1",
						"Status: Ok\n"
						"Reason: Successfully cleared auth info 1."));
	addExample(new DaemonCommandExample("auth-infos-clear ALL",
						"Status: Ok\n"
						"Reason: Successfully cleared 5 auth infos."));
	addExample(new DaemonCommandExample("auth-infos-clear 3",
						"Status: Error\n"
						"Reason: No auth info with such id."));
}

void AuthInfosClearCommand::exec(Daemon *app, const char *args) {
	string param;
	int pid;
	ostringstream ostr;

	istringstream ist(args);
	ist >> param;
	if (ist.fail()) {
		app->sendResponse(Response("Missing parameter.", Response::Error));
		return;
	}
	if (param.compare("ALL") == 0) {
		int previous_size = app->maxAuthInfoId();
		linphone_core_clear_all_auth_info(app->getCore());
		ostr << "Successfully cleared " << previous_size - app->maxAuthInfoId() << " auth infos." << endl;
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	} else {
		LinphoneAuthInfo *auth_info = NULL;
		ist.clear();
		ist.str(param);
		ist >> pid;
		if (ist.fail()) {
			app->sendResponse(Response("Incorrect parameter.", Response::Error));
			return;
		}
		auth_info = app->findAuthInfo(pid);
		if (auth_info == NULL) {
			app->sendResponse(Response("No auth info with such id.", Response::Error));
			return;
		}
		linphone_core_remove_auth_info(app->getCore(), auth_info);
		ostr << "Successfully cleared auth info " << pid << "." << endl;
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	}
}
