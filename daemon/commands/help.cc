#include "help.h"

using namespace std;

HelpCommand::HelpCommand() :
		DaemonCommand("help", "help <command>", "Show <command> help notice, if command is unspecified or inexistent show all commands.") {
}
void HelpCommand::exec(Daemon *app, const char *args) {
	char str[16384] = { 0 };
	int written = 0;
	list<DaemonCommand*>::const_iterator it;
	const list<DaemonCommand*> &l = app->getCommandList();
	if (args){
		for (it = l.begin(); it != l.end(); ++it) {
			if ((*it)->matches(args)){
				written += snprintf(str + written, sizeof(str)-1 - written, "\t%s\n%s\n", (*it)->getProto().c_str(),(*it)->getHelp().c_str());
				break;
			}
		}
		if (it==l.end()) args=NULL;
	}
	
	if (args==NULL){
		for (it = l.begin(); it != l.end(); ++it) {
			written += snprintf(str + written, sizeof(str)-1 - written, "\t%s\n", (*it)->getProto().c_str());
		}
	}
	Response resp;
	resp.setBody(str);
	app->sendResponse(resp);
}
