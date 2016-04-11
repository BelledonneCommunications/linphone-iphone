#include "contact.h"

using namespace std;

ContactCommand::ContactCommand() :
		DaemonCommand("contact", "contact sip:<username>@<hostname> or contact username hostname", "Set a contact name.") {
	addExample(new DaemonCommandExample("contact sip:root@unknown-host",
						"Status: Ok\n\n"));
}
void ContactCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	int result;
	char *contact;
	char username[256] = { 0 };
	char hostname[256] = { 0 };
	result = sscanf(args, "%255s %255s", username, hostname);
	if (result == 1) {
		linphone_core_set_primary_contact(lc,username);
		app->sendResponse(Response("", Response::Ok));
	}
	else if (result > 1) {
		contact=ortp_strdup_printf("sip:%s@%s",username,hostname);
		linphone_core_set_primary_contact(lc,contact);
		ms_free(contact);
		app->sendResponse(Response("", Response::Ok));
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
