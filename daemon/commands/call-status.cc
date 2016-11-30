/*
call-status.cc
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

#include "call-status.h"

using namespace std;

CallStatusCommand::CallStatusCommand() :
		DaemonCommand("call-status", "call-status [<call_id>]", "Return status of a call.") {
	addExample(new DaemonCommandExample("call-status 1",
						"Status: Ok\n\n"
						"State: LinphoneCallStreamsRunning\n"
						"From: <sip:daemon-test@sip.linphone.org>\n"
						"Direction: out\n"
						"Duration: 6"));
	addExample(new DaemonCommandExample("call-status 2",
						"Status: Error\n"
						"Reason: No call with such id."));
	addExample(new DaemonCommandExample("call-status",
						"Status: Error\n"
						"Reason: No current call available."));
}
void CallStatusCommand::exec(Daemon *app, const string& args) {
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call = NULL;
	istringstream ist(args);
	ist >> cid;
	if (ist.fail()) {
		call = linphone_core_get_current_call(lc);
		if (call == NULL) {
			app->sendResponse(Response("No current call available."));
			return;
		}
	} else {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	}

	LinphoneCallState call_state = LinphoneCallIdle;
	call_state = linphone_call_get_state(call);
	const LinphoneAddress *remoteAddress = linphone_call_get_remote_address(call);
	ostringstream ostr;
	ostr << "State: " << linphone_call_state_to_string(call_state) << "\n";

	switch (call_state) {
	case LinphoneCallOutgoingInit:
	case LinphoneCallOutgoingProgress:
	case LinphoneCallOutgoingRinging:
	case LinphoneCallPaused:
	case LinphoneCallStreamsRunning:
	case LinphoneCallConnected:
	case LinphoneCallIncomingReceived:
		ostr << "From: " << linphone_address_as_string(remoteAddress) << "\n";
		break;
	default:
		break;
	}
	switch (call_state) {
	case LinphoneCallStreamsRunning:
	case LinphoneCallConnected:
		ostr << "Direction: " << ((linphone_call_get_dir(call) == LinphoneCallOutgoing) ? "out" : "in") << "\n";
		ostr << "Duration: " << linphone_call_get_duration(call) << "\n";
		break;
	default:
		break;
	}
	app->sendResponse(Response(ostr.str(), Response::Ok));
}
