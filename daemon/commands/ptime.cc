#include "ptime.h"

using namespace std;

class PtimeCommandPrivate {
public:
	void outputPtime(Daemon *app, ostringstream &ost, int ms);
};

void PtimeCommandPrivate::outputPtime(Daemon* app, ostringstream& ost, int ms) {
	ost << "Value: " << ms << "\n";
}

PtimeCommand::PtimeCommand() :
		DaemonCommand("ptime", "ptime [up|down] <ms>", "Set the upload or download ptime if ms is defined, otherwise return the current value of the ptime."),
		d(new PtimeCommandPrivate()) {
}

PtimeCommand::~PtimeCommand()
{
	delete d;
}

void PtimeCommand::exec(Daemon *app, const char *args) {
	string direction;
	int ms;
	istringstream ist(args);
	ist >> direction;
	if (ist.fail()) {
		app->sendResponse(Response("Missing/Incorrect parameter(s).", Response::Error));
	} else {
		if (direction.compare("up") == 0) {
			if (!ist.eof()) {
				ist >> ms;
				if (ist.fail()) {
					app->sendResponse(Response("Incorrect ms parameter.", Response::Error));
				}
				linphone_core_set_upload_ptime(app->getCore(), ms);
			}
			ms = linphone_core_get_upload_ptime(app->getCore());
			ostringstream ost;
			d->outputPtime(app, ost, ms);
			app->sendResponse(Response(ost.str().c_str(), Response::Ok));
		} else if (direction.compare("down") == 0) {
			if (!ist.eof()) {
				ist >> ms;
				if (ist.fail()) {
					app->sendResponse(Response("Incorrect ms parameter.", Response::Error));
				}
				linphone_core_set_download_ptime(app->getCore(), ms);
			}
			ms = linphone_core_get_download_ptime(app->getCore());
			ostringstream ost;
			d->outputPtime(app, ost, ms);
			app->sendResponse(Response(ost.str().c_str(), Response::Ok));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s).", Response::Error));
		}
	}
}
