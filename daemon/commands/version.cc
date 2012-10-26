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
	addExample(new DaemonCommandExample("version",
						"Status: Ok\n\n"
						"Version: 3.5.99.0_6c2f4b9312fd4717b2f8ae0a7d7c97b752768c7c"));
}

void VersionCommand::exec(Daemon *app, const char *args) {
	app->sendResponse(VersionResponse(app->getCore()));
}
