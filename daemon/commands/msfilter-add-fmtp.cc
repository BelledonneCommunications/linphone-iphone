/*
msfilter-add-fmtp.cc
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

#include "msfilter-add-fmtp.h"
#include <mediastreamer2/msfilter.h>
#include <private.h>

using namespace std;

MSFilterAddFmtpCommand::MSFilterAddFmtpCommand() :
	DaemonCommand("msfilter-add-fmtp", "msfilter-add-fmtp call|stream <id> <fmtp>", "Add fmtp to the encoder of a call or a stream") {
	addExample(new DaemonCommandExample("msfilter-add-fmtp call 1 vbr=on",
						"Status: Ok"));
	addExample(new DaemonCommandExample("msfilter-add-fmtp call 2 vbr=on",
						"Status: Error\n"
						"Reason: No Call with such id."));
	addExample(new DaemonCommandExample("msfilter-add-fmtp stream 7 vbr=on",
						"Status: Error\n"
						"Reason: No Audio Stream with such id."));
}

void MSFilterAddFmtpCommand::exec(Daemon *app, const string& args) {
	string type;
	int id;
	string fmtp;

	istringstream ist(args);
	ist >> type;
	ist >> id;
	ist >> fmtp;
	if (ist.fail()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		return;
	}
	if (type.compare("call") == 0) {
		LinphoneCall *call = app->findCall(id);
		if (call == NULL) {
			app->sendResponse(Response("No Call with such id."));
			return;
		}
		if (call->audiostream == NULL || call->audiostream->ms.encoder == NULL) {
			app->sendResponse(Response("This call doesn't have an active audio stream."));
			return;
		}
		ms_filter_call_method(call->audiostream->ms.encoder, MS_FILTER_ADD_FMTP, (void *)fmtp.c_str());
	} else if (type.compare("stream") == 0) {
		AudioStream *stream = app->findAudioStream(id);
		if (stream == NULL) {
			app->sendResponse(Response("No Audio Stream with such id."));
			return;
		}
		ms_filter_call_method(stream->ms.encoder, MS_FILTER_ADD_FMTP, (void *)fmtp.c_str());
	} else {
		app->sendResponse(Response("Incorrect parameter(s)."));
		return;
	}
	app->sendResponse(Response());
}
