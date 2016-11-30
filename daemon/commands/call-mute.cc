/*
call-mute.cc
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

#include "call-mute.h"

using namespace std;

CallMuteCommand::CallMuteCommand() :
	DaemonCommand("call-mute", "call-mute 0|1", "Mute/unmute the microphone (1 to mute, 0 to unmute). No argument means MUTE.")
{
	addExample(new DaemonCommandExample("call-mute 1",
										"Status: Ok\n\n"
										"Microphone Muted"));
	addExample(new DaemonCommandExample("call-mute",
										"Status: Ok\n\n"
										"Microphone Muted"));
	addExample(new DaemonCommandExample("call-mute 0",
										"Status: Ok\n\n"
										"Microphone Unmuted"));
	addExample(new DaemonCommandExample("call-mute 1",
										"Status: Error\n\n"
										"Reason: No call in progress. Can't mute."));
}

void CallMuteCommand::exec(Daemon* app, const string& args)
{
	LinphoneCore *lc = app->getCore();
	int muted;
	LinphoneCall *call = linphone_core_get_current_call(lc);

	if (call == NULL) {
		app->sendResponse(Response("No call in progress. Can't mute."));
		return;
	}

	istringstream ist(args);
	ist >> muted;
	if (ist.fail() || (muted != 0)) {
		muted = TRUE;
	} else {
		muted = FALSE;
	}
	linphone_core_enable_mic(lc, !muted);

	app->sendResponse(Response(muted ? "Microphone Muted" : "Microphone Unmuted", Response::Ok));
}
