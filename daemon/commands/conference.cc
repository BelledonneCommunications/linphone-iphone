/*
conference.cc
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

#include "conference.h"

using namespace std;

ConferenceCommand::ConferenceCommand() :
	DaemonCommand("conference", "conference add|rm|leave|enter <call_id>",
				  "Create and manage an audio conference.\n"
				  "You can:\n"
				  "- add   : join the call with id 'call id' into the audio conference. Creates new one if none exists.\n"
				  "- rm    : remove the call with id 'call id' from the audio conference.\n"
				  "- leave : temporarily leave the current conference.\n"
				  "- enter : re-join the conference after leaving it.")
{
	addExample(new DaemonCommandExample("conference add 1",
										"Status: Ok\n\n"
										"Call Id: 1\n"
										"Conference: add OK"));
	addExample(new DaemonCommandExample("conference leave 1",
										"Status: Ok\n\n"
										"Call Id: 1\n"
										"Conference: leave OK"));
	addExample(new DaemonCommandExample("conference azerty 1",
										"Status: Error\n\n"
										"Reason: Invalid command format"));
	addExample(new DaemonCommandExample("conference leave 2",
										"Status: Error\n\n"
										"Reason: No call with such id."));
}

void ConferenceCommand::exec(Daemon* app, const string& args) {
	LinphoneCore* lc = app->getCore();
	int id;
	string subcommand;
	int ret;
	istringstream ist(args);
	ist >> subcommand;
	ist >> id;
	if (ist.fail()) {
		app->sendResponse(Response("Invalid command format.", Response::Error));
		return;
	}

	LinphoneCall *call=app->findCall(id);
	if (call == NULL) {
		app->sendResponse(Response("No call with such id.", Response::Error));
		return;
	}

	if (subcommand.compare("add") == 0) {
		ret = linphone_core_add_to_conference(lc, call);
	} else if (subcommand.compare("rm") == 0) {
		ret = linphone_core_remove_from_conference(lc, call);
	} else if (subcommand.compare("enter") == 0) {
		ret = linphone_core_enter_conference(lc);
	} else if (subcommand.compare("leave") == 0) {
		ret = linphone_core_leave_conference(lc);
	} else {
		app->sendResponse(Response("Invalid command format.", Response::Error));
		return;
	}

	if (ret == 0) {
		std::ostringstream ostr;
		ostr << "Call ID: " << id << "\n";
		ostr << "Conference: " << subcommand << " OK" << "\n";
		app->sendResponse(Response(ostr.str(), Response::Ok));
		return;
	}

	app->sendResponse(Response("Command failed", Response::Error));
}
