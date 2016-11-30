/*
dtmf.cc
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

#include "dtmf.h"

using namespace std;

DtmfCommand::DtmfCommand() :
		DaemonCommand("dtmf", "dtmf <digit>", "Generate a DTMF (one of 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, *, #).") {
	addExample(new DaemonCommandExample("dtmf 4",
						"Status: Ok"));
	addExample(new DaemonCommandExample("dtmf B",
						"Status: Ok"));
	addExample(new DaemonCommandExample("dtmf #",
						"Status: Ok"));
}

void DtmfCommand::exec(Daemon *app, const string& args) {
	string digit_str;
	char digit;
	istringstream ist(args);
	ist >> digit_str;
	if (ist.fail()) {
		app->sendResponse(Response("Missing digit parameter.", Response::Error));
		return;
	}

	digit = digit_str.at(0);
	if (isdigit(digit) || (digit == 'A') || (digit == 'B') || (digit == 'C') || (digit == 'D') || (digit == '*') || (digit == '#')) {
		LinphoneCall *call = linphone_core_get_current_call(app->getCore());
		linphone_core_play_dtmf(app->getCore(), digit, 100);
		if (call == NULL) {
			linphone_call_send_dtmf(call, digit);
		}
		app->sendResponse(Response());
	} else {
		app->sendResponse(Response("Incorrect digit parameter.", Response::Error));
	}
}
