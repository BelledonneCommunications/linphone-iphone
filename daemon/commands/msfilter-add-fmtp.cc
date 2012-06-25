#include "msfilter-add-fmtp.h"
#include <mediastreamer2/msfilter.h>
#include <private.h>

using namespace std;

MSFilterAddFmtpCommand::MSFilterAddFmtpCommand() :
		DaemonCommand("msfilter-add-fmtp", "msfilter-add-fmtp <fmtp>", "Add fmtp to current encoder") {
}

void MSFilterAddFmtpCommand::exec(Daemon *app, const char *args) {
	LinphoneCore *lc = app->getCore();
	LinphoneCall *call = linphone_core_get_current_call(lc);
	if (call == NULL) {
		app->sendResponse(Response("No current call available."));
		return;
	}
	ms_filter_call_method(call->audiostream->encoder, MS_FILTER_ADD_FMTP, (void*) args);
}
