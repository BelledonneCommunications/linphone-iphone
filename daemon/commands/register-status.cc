/*
register-status.cc
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

#include "register-status.h"

using namespace std;

class RegisterStatusResponse : public Response {
public:
	RegisterStatusResponse();
	RegisterStatusResponse(int id, const LinphoneProxyConfig *cfg);
	void append(int id, const LinphoneProxyConfig *cfg);
};

RegisterStatusResponse::RegisterStatusResponse() {
}

RegisterStatusResponse::RegisterStatusResponse(int id, const LinphoneProxyConfig *cfg) {
	append(id, cfg);
}

void RegisterStatusResponse::append(int id, const LinphoneProxyConfig* cfg) {
	ostringstream ost;
	ost << getBody();
	if (ost.tellp() > 0) {
		ost << "\n";
	}
	ost << "Id: " << id << "\n";
	ost << "State: " << linphone_registration_state_to_string(linphone_proxy_config_get_state(cfg)) << "\n";
	setBody(ost.str());
}

RegisterStatusCommand::RegisterStatusCommand() :
		DaemonCommand("register-status", "register-status <register_id>|ALL", "Return status of a registration or of all registrations.") {
	addExample(new DaemonCommandExample("register-status 1",
						"Status: Ok\n\n"
						"Id: 1\n"
						"State: LinphoneRegistrationOk"));
	addExample(new DaemonCommandExample("register-status ALL",
						"Status: Ok\n\n"
						"Id: 1\n"
						"State: LinphoneRegistrationOk\n\n"
						"Id: 2\n"
						"State: LinphoneRegistrationFailed"));
	addExample(new DaemonCommandExample("register-status 3",
						"Status: Error\n"
						"Reason: No register with such id."));
}

void RegisterStatusCommand::exec(Daemon *app, const string& args) {
	LinphoneProxyConfig *cfg = NULL;
	string param;
	int pid;

	istringstream ist(args);
	ist >> param;
	if (ist.fail()) {
		app->sendResponse(Response("Missing parameter.", Response::Error));
		return;
	}
	if (param.compare("ALL") == 0) {
		RegisterStatusResponse response;
		for (int i = 1; i <= app->maxProxyId(); i++) {
			cfg = app->findProxy(i);
			if (cfg != NULL) {
				response.append(i, cfg);
			}
		}
		app->sendResponse(response);
	} else {
		ist.clear();
		ist.str(param);
		ist >> pid;
		if (ist.fail()) {
			app->sendResponse(Response("Incorrect parameter.", Response::Error));
			return;
		}
		cfg = app->findProxy(pid);
		if (cfg == NULL) {
			app->sendResponse(Response("No register with such id.", Response::Error));
			return;
		}
		app->sendResponse(RegisterStatusResponse(pid, cfg));
	}
}
