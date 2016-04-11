#include "help.h"

using namespace std;

HelpCommand::HelpCommand() :
		DaemonCommand("help", "help <command>", "Show <command> help notice, if command is unspecified or inexistent show all commands.") {
}

void HelpCommand::exec(Daemon *app, const char *args) {
	ostringstream ost;
	list<DaemonCommand*>::const_iterator it;
	const list<DaemonCommand*> &l = app->getCommandList();
	if (args){
		for (it = l.begin(); it != l.end(); ++it) {
			if ((*it)->matches(args)){
				ost << (*it)->getHelp();
				break;
			}
		}
		if (it==l.end()) args=NULL;
	}
	
	if (args==NULL){
		for (it = l.begin(); it != l.end(); ++it) {
			ost << (*it)->getProto() << endl;
		}
	}
	Response resp;
	resp.setBody(ost.str().c_str());
	app->sendResponse(resp);
}
