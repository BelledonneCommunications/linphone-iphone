/*
terminate.cc
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

#include "terminate.h"

using namespace std;

TerminateCommand::TerminateCommand() :
		DaemonCommand("terminate", "terminate [<call_id>]", "Terminate a call.") {
	addExample(new DaemonCommandExample("terminate 2",
						"Status: Error\n"
						"Reason: No call with such id."));
	addExample(new DaemonCommandExample("terminate 1",
						"Status: Ok\n"));
	addExample(new DaemonCommandExample("terminate",
						"Status: Ok\n"));
	addExample(new DaemonCommandExample("terminate",
						"Status: Error\n"
						"Reason: No active call."));
}
void TerminateCommand::exec(Daemon *app, const string& args) {
	LinphoneCall *call = NULL;
	int cid;
	const MSList *elem;
	istringstream ist(args);
	ist >> cid;
	if (ist.fail()) {
		elem = linphone_core_get_calls(app->getCore());
		if (elem != NULL && elem->next == NULL) {
			call = (LinphoneCall*)elem->data;
		}
	} else {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	}
	if (call == NULL) {
		app->sendResponse(Response("No active call."));
		return;
	}
	linphone_core_terminate_call(app->getCore(), call);
	app->sendResponse(Response());
}
