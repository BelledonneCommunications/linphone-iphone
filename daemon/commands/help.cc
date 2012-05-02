#include "help.h"

using namespace std;

HelpCommand::HelpCommand() :
		DaemonCommand("help", "help", "Show available commands.") {
}
void HelpCommand::exec(Daemon *app, const char *args) {
	char str[4096] = { 0 };
	int written = 0;
	list<DaemonCommand*>::const_iterator it;
	const list<DaemonCommand*> &l = app->getCommandList();
	for (it = l.begin(); it != l.end(); ++it) {
		written += snprintf(str + written, sizeof(str) - written, "%s\t%s\n", (*it)->getProto().c_str(), (*it)->getHelp().c_str());
	}
	Response resp;
	resp.setBody(str);
	app->sendResponse(resp);
}
