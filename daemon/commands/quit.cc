#include "quit.h"

#include <sstream>

using namespace std;

QuitCommand::QuitCommand() :
		DaemonCommand("quit", "quit", "Quit the application.") {
}

void QuitCommand::exec(Daemon *app, const char *args) {
	app->quit();
	app->sendResponse(Response());
}
