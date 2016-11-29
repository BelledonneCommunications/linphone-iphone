/*
version.cc
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

#include "version.h"

using namespace std;

class VersionResponse : public Response {
public:
	VersionResponse(LinphoneCore *core);
};

VersionResponse::VersionResponse(LinphoneCore *core) : Response() {
	ostringstream ost;
	ost << "Version: " << linphone_core_get_version();
	setBody(ost.str());
}

VersionCommand::VersionCommand() :
		DaemonCommand("version", "version", "Get the version number.") {
	addExample(new DaemonCommandExample("version",
						"Status: Ok\n\n"
						"Version: 3.5.99.0_6c2f4b9312fd4717b2f8ae0a7d7c97b752768c7c"));
}

void VersionCommand::exec(Daemon *app, const string& args) {
	app->sendResponse(VersionResponse(app->getCore()));
}
