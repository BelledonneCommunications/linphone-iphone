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
		DaemonCommand("contact", "contact sip:<username>@<hostname> or contact username hostname", "Set a contact name.") {
	addExample(new DaemonCommandExample("contact sip:root@unknown-host",
						"Status: Ok\n\n"));
}
void ContactCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	int result;
	char *contact;
	char username[256] = { 0 };
	char hostname[256] = { 0 };
	result = sscanf(args, "%255s %255s", username, hostname);
	if (result == 1) {
		linphone_core_set_primary_contact(lc,username);
		app->sendResponse(Response("", Response::Ok));
	}
	else if (result > 1) {
		contact=ortp_strdup_printf("sip:%s@%s",username,hostname);
		linphone_core_set_primary_contact(lc,contact);
		ms_free(contact);
		app->sendResponse(Response("", Response::Ok));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
