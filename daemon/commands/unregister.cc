#include "unregister.h"

using namespace std;

UnregisterCommand::UnregisterCommand() :
		DaemonCommand("unregister", "unregister <register_id>", "Unregister the daemon from proxy.") {
}
void UnregisterCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
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
	linphone_core_remove_proxy_config(lc, cfg);
	app->sendResponse(Response());
}
