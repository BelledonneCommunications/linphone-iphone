#include "version.h"

using namespace std;

class VersionResponse : public Response {
public:
	VersionResponse(LinphoneCore *core);
};

VersionResponse::VersionResponse(LinphoneCore *core) : Response() {
	ostringstream ost;
	ost << "Version: " << linphone_core_get_version();
	setBody(ost.str().c_str());
}

VersionCommand::VersionCommand() :
		DaemonCommand("version", "version", "Get the version number.") {
}

void VersionCommand::exec(Daemon *app, const char *args) {
	app->sendResponse(VersionResponse(app->getCore()));
}
