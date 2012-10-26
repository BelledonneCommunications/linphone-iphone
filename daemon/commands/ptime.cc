#include "ptime.h"

using namespace std;

class PtimeResponse : public Response {
public:
	enum Direction {
		Upload,
		Download,
		BothDirections
	};
	PtimeResponse(LinphoneCore *core, Direction dir);
};

PtimeResponse::PtimeResponse(LinphoneCore *core, Direction dir) : Response() {
	ostringstream ost;
	switch (dir) {
		case Upload:
			ost << "Upload: " << linphone_core_get_upload_ptime(core) << "\n";
			break;
		case Download:
			ost << "Download: " << linphone_core_get_download_ptime(core) << "\n";
			break;
		case BothDirections:
			ost << "Upload: " << linphone_core_get_upload_ptime(core) << "\n";
			ost << "Download: " << linphone_core_get_download_ptime(core) << "\n";
			break;
	}
	setBody(ost.str().c_str());
}

PtimeCommand::PtimeCommand() :
		DaemonCommand("ptime", "ptime [up|down] <ms>", "Set the upload or download ptime if ms is defined, otherwise return the current value of the ptime.") {
	addExample(new DaemonCommandExample("ptime up 20",
						"Status: Ok\n\n"
						"Upload: 20"));
	addExample(new DaemonCommandExample("ptime down 30",
						"Status: Ok\n\n"
						"Download: 30"));
	addExample(new DaemonCommandExample("ptime up",
						"Status: Ok\n\n"
						"Upload: 20"));
	addExample(new DaemonCommandExample("ptime",
						"Status: Ok\n\n"
						"Upload: 20\n"
						"Download: 30"));
}

void PtimeCommand::exec(Daemon *app, const char *args) {
	string direction;
	int ms;
	istringstream ist(args);
	ist >> direction;
	if (ist.fail()) {
		app->sendResponse(PtimeResponse(app->getCore(), PtimeResponse::BothDirections));
	} else {
		if (direction.compare("up") == 0) {
			if (!ist.eof()) {
				ist >> ms;
				if (ist.fail()) {
					app->sendResponse(Response("Incorrect ms parameter.", Response::Error));
				}
				linphone_core_set_upload_ptime(app->getCore(), ms);
			}
			app->sendResponse(PtimeResponse(app->getCore(), PtimeResponse::Upload));
		} else if (direction.compare("down") == 0) {
			if (!ist.eof()) {
				ist >> ms;
				if (ist.fail()) {
					app->sendResponse(Response("Incorrect ms parameter.", Response::Error));
				}
				linphone_core_set_download_ptime(app->getCore(), ms);
			}
			app->sendResponse(PtimeResponse(app->getCore(), PtimeResponse::Download));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s).", Response::Error));
		}
	}
}
