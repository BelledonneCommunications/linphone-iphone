#include "register.h"
#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"
#include <string.h>


using namespace std;

RegisterCommand::RegisterCommand() :
		DaemonCommand("register", "register <identity> <proxy-address> <password> <userid> <realm>", "Register the daemon to a SIP proxy. If one of the parameters <password>, <userid> and <realm> is not needed, send the string \"NULL\"") {
	addExample(new DaemonCommandExample("register sip:daemon-test@sip.linphone.org sip.linphone.org password bob linphone.org",
						"Status: Ok\n\n"
						"Id: 1"));
}
void RegisterCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	ostringstream ostr;
	char proxy[256] = { 0 }, identity[128] = { 0 }, password[64] = { 0 }, userid[128] = { 0 }, realm[128] = { 0 }, parameter[256] = { 0 };
	if (sscanf(args, "%255s %127s %63s %127s %127s %255s", identity, proxy, password, userid, realm, parameter) >= 2) {
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		if (strcmp(password, "NULL") == 0) {
			password[0] = 0;
		}
		if (strcmp(userid, "NULL") == 0) {
			userid[0] = 0;
		}
		if (strcmp(realm, "NULL") == 0) {
			realm[0] = 0;
		}
		if (strcmp(parameter, "NULL") == 0) {
			parameter[0] = 0;
		}
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		LinphoneProxyConfig *cfg = linphone_proxy_config_new();
		if (password[0] != '\0') {
			LinphoneAddress *from = linphone_address_new(identity);
			if (from != NULL) {
				LinphoneAuthInfo *info = linphone_auth_info_new(linphone_address_get_username(from),
																userid, password, NULL, realm, NULL);
				linphone_core_add_auth_info(lc, info); /*add authentication info to LinphoneCore*/
				linphone_address_destroy(from);
				linphone_auth_info_destroy(info);
			}
		}
		linphone_proxy_config_set_identity(cfg, identity);
		linphone_proxy_config_set_server_addr(cfg, proxy);
		linphone_proxy_config_enable_register(cfg, TRUE);
		linphone_proxy_config_set_contact_parameters(cfg, parameter);
		ostr << "Id: " << app->updateProxyId(cfg) << "\n";
		linphone_core_add_proxy_config(lc, cfg);
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
