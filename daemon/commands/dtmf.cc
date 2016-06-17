#include "dtmf.h"

using namespace std;

DtmfCommand::DtmfCommand() :
		DaemonCommand("dtmf", "dtmf <digit>", "Generate a DTMF (one of: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, *, #.") {
	addExample(new DaemonCommandExample("dtmf 4",
						"Status: Ok"));
	addExample(new DaemonCommandExample("dtmf B",
						"Status: Ok"));
	addExample(new DaemonCommandExample("dtmf #",
						"Status: Ok"));
}

void DtmfCommand::exec(Daemon *app, const char *args) {
	string digit_str;
	char digit;
	istringstream ist(args);
	ist >> digit_str;
	if (ist.fail()) {
		app->sendResponse(Response("Missing digit parameter.", Response::Error));
	} else {
	digit = digit_str.at(0);
		if (isdigit(digit) || (digit == 'A') || (digit == 'B') || (digit == 'C') || (digit == 'D') || (digit == '*') || (digit == '#')) {
			LinphoneCall *call = linphone_core_get_current_call(app->getCore());
			linphone_core_play_dtmf(app->getCore(), digit, 100);
			if (call == NULL) {
				linphone_call_send_dtmf(call, digit);
			}
			app->sendResponse(Response());
		} else {
			app->sendResponse(Response("Incorrect digit parameter.", Response::Error));
		}
	}
}
