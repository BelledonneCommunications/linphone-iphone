/*
register-info.cc
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

#include <stdexcept>
#include <string>
#include "register-info.h"

class RegisterInfoResponse: public Response {
public:
	RegisterInfoResponse(): Response() {}
	RegisterInfoResponse(int id, const ::LinphoneProxyConfig *cfg): Response() {
		append(id, cfg);
	}
	void append(int id, const ::LinphoneProxyConfig *cfg) {
		std::ostringstream ost;
		ost << getBody();
		if (ost.tellp() > 0) ost << std::endl;
		ost << "Id: " << id << std::endl;
		ost << "Identity: " << linphone_proxy_config_get_identity(cfg) << std::endl;
		ost << "Proxy: " << linphone_proxy_config_get_server_addr(cfg) << std::endl;
		
		const char *route = linphone_proxy_config_get_route(cfg);
		if (route != NULL) {
			ost << "Route: " << route << std::endl;
		}
		
		ost << "State: " << linphone_registration_state_to_string(linphone_proxy_config_get_state(cfg)) << std::endl;
		setBody(ost.str());
	}
};

RegisterInfoCommand::RegisterInfoCommand():
	DaemonCommand("register-info", "register-info <register_id>|ALL",
		"Get informations about one or more registrations.")
{
	addExample(new DaemonCommandExample("register-info 1",
						"Status: Ok\n\n"
						"Id: 1\n"
						"Identity: sip:toto@sip.linphone.org\n"
						"Proxy: <sip:sip.linphone.org;transport=tls>\n"
						"Route: <sip:sip.linphone.org;transport=tls>\n"
						"State: LinphoneRegistrationOk"));
	addExample(new DaemonCommandExample("register-info ALL",
						"Status: Ok\n\n"
						"Id: 1\n"
						"Identity: sip:toto@sip.linphone.org\n"
						"Proxy: <sip:sip.linphone.org;transport=tls>\n"
						"Route: <sip:sip.linphone.org;transport=tls>\n"
						"State: LinphoneRegistrationOk\n\n"
						"Id: 2\n"
						"Identity: sip:toto2@sip.linphone.org\n"
						"Proxy: <sip:sip.linphone.org;transport=udp>\n"
						"State: LinphoneRegistrationFailed"));
	addExample(new DaemonCommandExample("register-info 3",
						"Status: Error\n"
						"Reason: No register with such id."));
}

void RegisterInfoCommand::exec(Daemon *app, const std::string& args) {
	std::string param;
	std::istringstream ist(args);
	ist >> param;
	if (ist.fail()) {
		app->sendResponse(Response("Missing parameter.", Response::Error));
		return;
	}
	if (param == "ALL") {
		RegisterInfoResponse response;
		for (int i=1; i<=app->maxProxyId(); i++) {
			::LinphoneProxyConfig *cfg = app->findProxy(i);
			if (cfg != NULL) {
				response.append(i, cfg);
			}
		}
		app->sendResponse(response);
	} else {
		int id;
		try {
			id = std::stoi(param);
		} catch (std::invalid_argument) {
			app->sendResponse(Response("Invalid ID.", Response::Error));
			return;
		} catch (std::out_of_range) {
			app->sendResponse(Response("Out of range ID.", Response::Error));
			return;
		}
		::LinphoneProxyConfig *cfg = app->findProxy(id);
		if (cfg == NULL) {
			app->sendResponse(Response("No register with such id.", Response::Error));
			return;
		}
		app->sendResponse(RegisterInfoResponse(id, cfg));
	}
}
