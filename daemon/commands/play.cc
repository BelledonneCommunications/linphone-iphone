/*
play-wav.cc
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

#include "play.h"
#include "call.h"
#include <utility>
using namespace std;
#define VOIDPTR_TO_INT(p) ((int)(intptr_t)(p))

void IncallPlayerStartCommand::onEof(LinphonePlayer *player){
	pair<int, Daemon *> *callPlayingData = (pair<int, Daemon *> *)linphone_player_get_user_data(player);
	Daemon *app = callPlayingData->second;
	int id = callPlayingData->first;
	app->callPlayingComplete(id);
	delete callPlayingData;
	linphone_player_set_user_data(player, NULL);
}


IncallPlayerStartCommand::IncallPlayerStartCommand() :
	DaemonCommand("incall-player-start", "incall-player-start <filename> [<call_id>]",
				"Play a WAV audio file or a MKV audio/video file. The played media stream will be sent through \n"
				"the RTP session of the given call.\n"
				"<filename> is the file to be played.\n") {
	addExample(new DaemonCommandExample("incall-player-start /usr/local/share/sounds/linphone/hello8000.wav 1",
						"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-start /usr/local/share/sounds/linphone/hello8000.wav 1",
						"Status: Error\n"
						"Reason: No call with such id."));
	addExample(new DaemonCommandExample("incall-player-start /usr/local/share/sounds/linphone/hello8000.wav",
						"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-start /usr/local/share/sounds/linphone/hello8000.wav",
						"Status: Error\n"
						"Reason: No active call."));
}

void IncallPlayerStartCommand::exec(Daemon *app, const string& args) {
	LinphoneCall *call = NULL;
	int cid;
	const MSList *elem;
	istringstream ist(args);
	string filename;

	ist >> filename;
	if (ist.eof() && (filename.length() == 0)) {
		app->sendResponse(Response("Missing filename parameter.", Response::Error));
		return;
	}
	if (ist.fail()) {
		app->sendResponse(Response("Incorrect filename parameter.", Response::Error));
		return;
	}
	
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
	LinphonePlayer *p = linphone_call_get_player(call);
	
	LinphonePlayerCbs *cbs=linphone_player_get_callbacks(p);
	
	pair<int, Daemon *> *callPlayingData = (pair<int, Daemon *> *)linphone_player_get_user_data(p);
	if(callPlayingData) callPlayingData = new 	pair<int, Daemon *>({
		VOIDPTR_TO_INT(linphone_call_get_user_pointer(call)),
		app
	});
	linphone_player_set_user_data(p, callPlayingData);
	linphone_player_cbs_set_eof_reached(cbs, onEof);
	linphone_player_open(p,filename.c_str());
	linphone_player_start(p);
	app->sendResponse(Response());
}


IncallPlayerStopCommand::IncallPlayerStopCommand() :
	DaemonCommand("incall-player-stop", "incall-player-stop [<call_id>]","Close the opened file.\n") {
	addExample(new DaemonCommandExample("incall-player-stop 1",
										"Status: Error\n"
										"Reason: No call with such id."));
	addExample(new DaemonCommandExample("incall-player-stop 1",
										"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-stop",
										"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-stop",
										"Status: Error\n"
										"Reason: No active call."));
}

void IncallPlayerStopCommand::exec(Daemon *app, const string& args) {
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
	
	LinphonePlayer *p = linphone_call_get_player(call);
	
	linphone_player_close(p);
	app->sendResponse(Response());
	pair<int, Daemon *> *callPlayingData = (pair<int, Daemon *> *)linphone_player_get_user_data(p);
	if(callPlayingData) delete callPlayingData;
}

IncallPlayerPauseCommand::IncallPlayerPauseCommand() :
	DaemonCommand("incall-player-pause", "incall-player-pause [<call_id>]",
			  "Pause the playing of a file.\n") {
	addExample(new DaemonCommandExample("incall-player-pause 1",
										"Status: Error\n"
										"Reason: No call with such id."));
	addExample(new DaemonCommandExample("incall-player-pause 1",
										"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-pause",
										"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-pause",
										"Status: Error\n"
										"Reason: No active call."));
}

void IncallPlayerPauseCommand::exec(Daemon *app, const string& args) {
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
	
	LinphonePlayer *p = linphone_call_get_player(call);
	linphone_player_pause(p);
	app->sendResponse(Response());
}

IncallPlayerResumeCommand::IncallPlayerResumeCommand() :
	DaemonCommand("incall-player-resume", "incall-player-resume [<call_id>]",
	"Unpause the playing of a file.\n") {
	addExample(new DaemonCommandExample("incall-player-resume 1",
										"Status: Error\n"
										"Reason: No call with such id."));
	addExample(new DaemonCommandExample("incall-player-resume 1",
										"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-resume",
										"Status: Ok\n"));
	addExample(new DaemonCommandExample("incall-player-resume",
										"Status: Error\n"
										"Reason: No active call."));
}

void IncallPlayerResumeCommand::exec(Daemon *app, const string& args) {
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
	
	LinphonePlayer *p = linphone_call_get_player(call);
	linphone_player_start(p);
	app->sendResponse(Response());
}
