/*
call-transfer.cc
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

#include "call-transfer.h"

CallTransfer::CallTransfer() :
	DaemonCommand("call-transfer",
				  "call-transfer <call id> <sip url>",
				  "Transfer a call that you aswered to another party")
{
	addExample(new DaemonCommandExample("call-transfer 1 sip:john",
										"Status: Ok\n\n"
										"Call ID: 1\n"
										"Transfer to: sip:john"));

	addExample(new DaemonCommandExample("call-transfer 2 sip:john",
										"Status: Error\n"
										"Reason: No call with such id."));
}

void CallTransfer::exec(Daemon* app, const char* args)
{
	LinphoneCore *lc = app->getCore();
	int cid;
	LinphoneCall *call = NULL;
	char sipurl[512];
	memset(sipurl, 0x00, 512);

	if (sscanf(args, "%i %511s", &cid, sipurl) == 2) {
		call = app->findCall(cid);
		if (call == NULL) {
			app->sendResponse(Response("No call with such id."));
			return;
		}
	} else {
		app->sendResponse(Response("Invalid command format."));
		return;
	}

	if( linphone_core_transfer_call(lc, call, sipurl) == 0 ) {
		std::ostringstream ostr;
		ostr << "Call ID: "     << cid    << "\n";
		ostr << "Transfer to: " << sipurl << "\n";
		app->sendResponse(Response(ostr.str(), Response::Ok));
	} else {
		app->sendResponse(Response("Error pausing call"));
	}
}
