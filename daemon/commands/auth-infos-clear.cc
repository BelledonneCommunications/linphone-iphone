/*
auth-infos-clear.cc
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

#include "auth-infos-clear.h"

using namespace std;

AuthInfosClearCommand::AuthInfosClearCommand() :
		DaemonCommand("auth-infos-clear", "auth-infos-clear <auth_infos_id>|ALL", "Remove auth infos context for the given id, or all.") {
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

void AuthInfosClearCommand::exec(Daemon *app, const string& args) {
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
		app->sendResponse(Response(ostr.str(), Response::Ok));
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
		app->sendResponse(Response(ostr.str(), Response::Ok));
	}
}
