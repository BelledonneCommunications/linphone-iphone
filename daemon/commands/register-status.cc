#include "register-status.h"

using namespace std;

RegisterStatusCommand::RegisterStatusCommand() :
		DaemonCommand("register-status", "register-status <register id>", "Return status of a register.") {
}
void RegisterStatusCommand::exec(Daemon *app, const char *args) {
	LinphoneProxyConfig *cfg = NULL;
	int pid;
	if (sscanf(args, "%i", &pid) == 1) {
		cfg = app->findProxy(pid);
		if (cfg == NULL) {
			app->sendResponse(Response("No register with such id."));
			return;
		}
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		return;
	}

	app->sendResponse(Response(linphone_registration_state_to_string(linphone_proxy_config_get_state(cfg)), Response::Ok));
}
