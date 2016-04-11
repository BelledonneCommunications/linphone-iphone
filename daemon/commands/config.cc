#include "configcommand.h"

using namespace std;

class ConfigResponse : public Response {
public:
	ConfigResponse(const char *value);
};

ConfigResponse::ConfigResponse(const char *value) : Response() {
	ostringstream ost;
	ost << "Value: "<<(value ? value : "<unset>");
	setBody(ost.str().c_str());
}

ConfigGetCommand::ConfigGetCommand() :
		DaemonCommand("config-get", "config section key",
				"Reads a configuration value from linphone's configuration database.") {
	addExample(new DaemonCommandExample("config rtp symmetric",
						"Status: Ok\n\n"
						"Value: <unset>"));
}

void ConfigGetCommand::exec(Daemon *app, const char *args) {
	string section,key;
	istringstream ist(args);
	ist >> section >> key;
	if (ist.fail()) {
		app->sendResponse(Response("Missing section and/or key names."));
	} else {
		const char *read_value=lp_config_get_string(linphone_core_get_config(app->getCore()),section.c_str(),key.c_str(),NULL);
		app->sendResponse(ConfigResponse(read_value));
	}
}


ConfigSetCommand::ConfigSetCommand() :
		DaemonCommand("config-set", "config section key value",
				"Sets a configuration value into linphone's configuration database.") {
	addExample(new DaemonCommandExample("config-set rtp symmetric 1",
						"Status: Ok\n\n"
						"Value: 2"));
	addExample(new DaemonCommandExample("config-set rtp symmetric",
						"Status: Ok\n\n"
						"Value: <unset>"));
}

void ConfigSetCommand::exec(Daemon *app, const char *args) {
	string section,key,value;
	istringstream ist(args);
	ist >> section >> key;
	if (ist.fail()) {
		app->sendResponse(Response("Missing section and/or key names."));
	} else {
		ist>>value;
		lp_config_set_string(linphone_core_get_config(app->getCore()), section.c_str(), key.c_str(), value.size()>0 ? value.c_str() : NULL);
		app->sendResponse(ConfigResponse(value.c_str()));
	}
}

