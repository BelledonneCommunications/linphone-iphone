/*
help.cc
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

#include "help.h"

using namespace std;

HelpCommand::HelpCommand() :
		DaemonCommand("help", "help [<command>]", "Show <command> help notice, if command is unspecified or inexistent show all commands.") {
}

void HelpCommand::exec(Daemon *app, const string& args) {
	ostringstream ost;
	list<DaemonCommand*>::const_iterator it;
	const list<DaemonCommand*> &l = app->getCommandList();
	bool found = false;
	if (!args.empty()){
		for (it = l.begin(); it != l.end(); ++it) {
			if ((*it)->matches(args)){
				ost << (*it)->getHelp();
				found = true;
				break;
			}
		}
	}
	
	if (!found){
		for (it = l.begin(); it != l.end(); ++it) {
			ost << (*it)->getProto() << endl;
		}
	}
	Response resp;
	resp.setBody(ost.str().c_str());
	app->sendResponse(resp);
}
