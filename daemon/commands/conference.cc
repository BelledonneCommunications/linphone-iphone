#include "conference.h"

Conference::Conference() :
	DaemonCommand("conference", "conference <subcommand> <call id>",
				  "Create and manage an audio conference.\n"
				  "Subcommands:\n"
				  "- add   : join the call with id 'call id' into the audio conference. Creates new one if none exists.\n"
				  "- rm    : remove the call with id 'call id' from the audio conference\n"
				  "- leave : temporarily leave the current conference.\n"
				  "- enter : re-join the conference after leaving it")
{
	addExample(new DaemonCommandExample("conference add 1",
										"Status: Ok\n\n"
										"Call Id: 1\n"
										"Conference: add OK"));
	addExample(new DaemonCommandExample("conference leave 1",
										"Status: Ok\n\n"
										"Call Id: 1\n"
										"Conference: leave OK"));
	addExample(new DaemonCommandExample("conference azerty 1",
										"Status: Error\n\n"
										"Reason: Invalid command format"));
	addExample(new DaemonCommandExample("conference leave 2",
										"Status: Error\n\n"
										"Reason: No call with such id."));
}

void Conference::exec(Daemon* app, const char* args)
{
	LinphoneCore* lc = app->getCore();
	long id;
	char subcommand[32]={0};
	int n;
	n=sscanf(args, "%31s %li", subcommand,&id);
	if (n == 2){
		LinphoneCall *call=app->findCall(id);
		if (call==NULL){
			app->sendResponse(Response("No call with such id."));
			return;
		}

		if (strcmp(subcommand,"add")==0){
			n = linphone_core_add_to_conference(lc,call);

		}else if (strcmp(subcommand,"rm")==0){
			n = linphone_core_remove_from_conference(lc,call);

		}else if (strcmp(subcommand,"enter")==0){
			n = linphone_core_enter_conference(lc);

		}else if (strcmp(subcommand,"leave")==0){
			n = linphone_core_leave_conference(lc);
		} else {
			app->sendResponse("Invalid command format.");
		}

		if ( n == 0 ){
			std::ostringstream ostr;
			ostr << "Call ID: " << id << "\n";
			ostr << "Conference: " << subcommand << " OK" << "\n";
			app->sendResponse(Response(ostr.str(), Response::Ok));
		} else {
			app->sendResponse("Conference: command failed");
		}

	} else {
		app->sendResponse("Invalid command format.");
	}
}
