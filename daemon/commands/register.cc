/*
register.cc
Copyright (C) 2016 Belledonne Communications, Grenoble, France 

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "register.h"
#include "linphone/core.h"
#include "private.h"
#include "linphone/lpconfig.h"
#include <string.h>


using namespace std;

RegisterCommand::RegisterCommand() :
		DaemonCommand("register", "register <identity> <proxy_address> [<password>] [<userid>] [<realm>] [<parameter>]", "Register the daemon to a SIP proxy. If one of the parameters <password>, <userid> and <realm> is not needed, send the string \"NULL\"") {
	addExample(new DaemonCommandExample("register sip:daemon-test@sip.linphone.org sip.linphone.org password bob linphone.org",
						"Status: Ok\n\n"
						"Id: 1"));
}
void RegisterCommand::exec(Daemon *app, const string& args) {
	LinphoneCore *lc = app->getCore();
	ostringstream ostr;
	string identity;
	string proxy;
	string password;
	string userid;
	string realm;
	string parameter;
	istringstream ist(args);
	const char *cidentity;
	const char *cproxy;
	const char *cpassword = NULL;
	const char *cuserid = NULL;
	const char *crealm = NULL;
	const char *cparameter = NULL;

	ist >> identity;
	ist >> proxy;
	ist >> password;
	ist >> userid;
	ist >> realm;
	ist >> parameter;
	if (proxy.empty()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		return;
	}
	cidentity = identity.c_str();
	cproxy = proxy.c_str();
	if (!password.empty() && (password.compare("NULL") != 0)) cpassword = password.c_str();
	if (!userid.empty() && (userid.compare("NULL") != 0)) cuserid = userid.c_str();
	if (!realm.empty() && (realm.compare("NULL") != 0)) crealm = realm.c_str();
	if (!parameter.empty()) cparameter = parameter.c_str();
	LinphoneProxyConfig *cfg = linphone_proxy_config_new();
	if (cpassword != NULL) {
		LinphoneAddress *from = linphone_address_new(cidentity);
		if (from != NULL) {
			LinphoneAuthInfo *info = linphone_auth_info_new(linphone_address_get_username(from), cuserid, cpassword, NULL, crealm, NULL);
			linphone_core_add_auth_info(lc, info); /* Add authentication info to LinphoneCore */
			linphone_address_unref(from);
			linphone_auth_info_destroy(info);
		}
	}
	linphone_proxy_config_set_identity(cfg, cidentity);
	linphone_proxy_config_set_server_addr(cfg, cproxy);
	linphone_proxy_config_enable_register(cfg, TRUE);
	linphone_proxy_config_set_contact_parameters(cfg, cparameter);
	ostr << "Id: " << app->updateProxyId(cfg) << "\n";
	linphone_core_add_proxy_config(lc, cfg);
	app->sendResponse(Response(ostr.str(), Response::Ok));
}
