#include "register.h"

#include <sstream>

using namespace std;

RegisterCommand::RegisterCommand() :
		DaemonCommand("register", "register <identity> <proxy-address> <password>", "Register the daemon to a SIP proxy.") {
}
void RegisterCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	char proxy[256] = { 0 }, identity[128] = { 0 }, password[64] = { 0 };
	if (sscanf(args, "%255s %127s %63s", identity, proxy, password) >= 2) {
		LinphoneProxyConfig *cfg = linphone_proxy_config_new();
		if (password[0] != '\0') {
			LinphoneAddress *from = linphone_address_new(identity);
			if (from != NULL) {
				LinphoneAuthInfo *info = linphone_auth_info_new(linphone_address_get_username(from), NULL, password, NULL, NULL); /*create authentication structure from identity*/
				linphone_core_add_auth_info(lc, info); /*add authentication info to LinphoneCore*/
				linphone_address_destroy(from);
				linphone_auth_info_destroy(info);
			}
		}
		linphone_proxy_config_set_identity(cfg, identity);
		linphone_proxy_config_set_server_addr(cfg, proxy);
		linphone_proxy_config_enable_register(cfg, TRUE);
		ostringstream ostr;
		ostr << "Id: " << app->updateProxyId(cfg) << "\n";
		linphone_core_add_proxy_config(lc, cfg);
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
