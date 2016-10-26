/*
call.cc
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

#include "call.h"

using namespace std;

CallCommand::CallCommand() :
		DaemonCommand("call", "call <sip address>", "Place a call.") {
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org",
						"Status: Ok\n\n"
						"Id: 1"));
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org --early-media",
						"Status: Ok\n\n"
						"Early media: Ok\n"
						"Id: 1"));
	addExample(new DaemonCommandExample("call daemon-test@sip.linphone.org",
						"Status: Error\n"
						"Reason: Call creation failed."));
}

void CallCommand::exec(Daemon *app, const char *args) {
	LinphoneCall *call;
	Response resp;
	ostringstream ostr;
	char address[256] = { 0 }, early_media[256] = { 0 };
	if (sscanf(args, "%s %s", address, early_media) == 2) {
		char *opt;
		LinphoneCallParams *cp;
		opt = strstr(early_media,"--early-media");
		cp = linphone_core_create_call_params(app->getCore(), NULL);
		if (opt) {
			linphone_call_params_enable_early_media_sending(cp, TRUE);
			ostr << "Early media: Ok\n";
		}
		call = linphone_core_invite_with_params(app->getCore(), address, cp);
	} else {
		call = linphone_core_invite(app->getCore(), args);
	}
	
	if (call == NULL) {
		app->sendResponse(Response("Call creation failed."));
	} else {
		ostr << "Id: " << app->updateCallId(call) << "\n";
		resp.setBody(ostr.str().c_str());
		app->sendResponse(resp);
	}
}
