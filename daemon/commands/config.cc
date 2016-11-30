/*
config.cc
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

#include "configcommand.h"

using namespace std;

class ConfigResponse : public Response {
public:
	ConfigResponse(const string &value);
};

ConfigResponse::ConfigResponse(const string& value) : Response() {
	ostringstream ost;
	ost << "Value: " << value ? value : "<unset>";
	setBody(ost.str());
}

ConfigGetCommand::ConfigGetCommand() :
		DaemonCommand("config-get", "config-get <section> <key>",
				"Reads a configuration value from linphone's configuration database.") {
	addExample(new DaemonCommandExample("config-get rtp symmetric",
						"Status: Ok\n\n"
						"Value: <unset>"));
}

void ConfigGetCommand::exec(Daemon *app, const string& args) {
	string section,key;
	istringstream ist(args);
	ist >> section >> key;
	if (ist.fail()) {
		app->sendResponse(Response("Missing section and/or key names."));
		return;
	}
	const char *read_value=lp_config_get_string(linphone_core_get_config(app->getCore()),section.c_str(),key.c_str(),NULL);
	app->sendResponse(ConfigResponse(read_value));
}


ConfigSetCommand::ConfigSetCommand() :
		DaemonCommand("config-set", "config-set <section> <key> <value>",
				"Sets a configuration value into linphone's configuration database.") {
	addExample(new DaemonCommandExample("config-set rtp symmetric 1",
						"Status: Ok\n\n"
						"Value: 2"));
	addExample(new DaemonCommandExample("config-set rtp symmetric",
						"Status: Ok\n\n"
						"Value: <unset>"));
}

void ConfigSetCommand::exec(Daemon *app, const string& args) {
	string section,key,value;
	istringstream ist(args);
	ist >> section >> key;
	if (ist.fail()) {
		app->sendResponse(Response("Missing section and/or key names."));
		return;
	}
	ist>>value;
	lp_config_set_string(linphone_core_get_config(app->getCore()), section.c_str(), key.c_str(), value.size()>0 ? value.c_str() : NULL);
	app->sendResponse(ConfigResponse(value.c_str()));
}

