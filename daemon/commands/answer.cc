/*
answer.cc
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

#include "answer.h"

using namespace std;

AnswerCommand::AnswerCommand() :
		DaemonCommand("answer", "answer <call_id>", "Answer an incoming call.") {
	addExample(new DaemonCommandExample("answer 3",
						"Status: Error\n"
						"Reason: No call with such id."));
	addExample(new DaemonCommandExample("answer 2",
						"Status: Error\n"
						"Reason: Can't accept this call."));
	addExample(new DaemonCommandExample("answer 1",
						"Status: Ok"));
	addExample(new DaemonCommandExample("answer",
						"Status: Ok"));
	addExample(new DaemonCommandExample("answer",
						"Status: Error\n"
						"Reason: No call to accept."));
}

void AnswerCommand::exec(Daemon *app, const string& args) {
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call;
	istringstream ist(args);
	ist >> cid;
	if (ist.fail()) {
		for (const MSList* elem = linphone_core_get_calls(lc); elem != NULL; elem = elem->next) {
			call = (LinphoneCall*)elem->data;
			LinphoneCallState cstate = linphone_call_get_state(call);
			if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallIncomingEarlyMedia) {
				if (linphone_core_accept_call(lc, call) == 0) {
					app->sendResponse(Response());
					return;
				}
			}
		}
	} else {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}

		LinphoneCallState cstate = linphone_call_get_state(call);
		if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallIncomingEarlyMedia) {
			if (linphone_core_accept_call(lc, call) == 0) {
				app->sendResponse(Response());
				return;
			}
		}
		app->sendResponse(Response("Can't accept this call."));
		return;
	}

	app->sendResponse(Response("No call to accept."));
}
