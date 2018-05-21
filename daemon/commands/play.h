/*
play-wav.h
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

#ifndef LINPHONE_DAEMON_COMMAND_PLAY_H_
#define LINPHONE_DAEMON_COMMAND_PLAY_H_

#include "daemon.h"

class IncallPlayerStartCommand: public DaemonCommand {
public:
	IncallPlayerStartCommand();

	void exec (Daemon *app, const std::string& args) override;

private:
	static void onEof(LinphonePlayer *player);
};

class IncallPlayerStopCommand: public DaemonCommand {
public:
	IncallPlayerStopCommand();

	void exec (Daemon *app, const std::string& args) override;
};

class IncallPlayerPauseCommand: public DaemonCommand {
public:
	IncallPlayerPauseCommand();

	void exec (Daemon *app, const std::string& args) override;
};

class IncallPlayerResumeCommand: public DaemonCommand {
public:
	IncallPlayerResumeCommand();

	void exec (Daemon *app, const std::string& args) override;
};
#endif // LINPHONE_DAEMON_COMMAND_PLAY_H_
