/*
contact.cc
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

#include "contact.h"

using namespace std;

ContactCommand::ContactCommand() :
		DaemonCommand("contact", "contact <sip_address> or contact <username> <hostname>", "Set a contact name.") {
	addExample(new DaemonCommandExample("contact sip:root@unknown-host",
						"Status: Ok\n\n"));
}

void ContactCommand::exec(Daemon *app, const string& args) {
	LinphoneCore *lc = app->getCore();
	char *contact;
	string username;
	string hostname;
	istringstream ist(args);
	ist >> username;
	if (ist.fail()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		return;
	}
	ist >> hostname;
	if (ist.fail()) {
		linphone_core_set_primary_contact(lc, username.c_str());
		app->sendResponse(Response("", Response::Ok));
	} else {
		contact = ortp_strdup_printf("sip:%s@%s", username.c_str(), hostname.c_str());
		linphone_core_set_primary_contact(lc, contact);
		ms_free(contact);
		app->sendResponse(Response("", Response::Ok));
	}
}
