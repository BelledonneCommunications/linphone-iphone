#include "msfilter-add-fmtp.h"
#include <mediastreamer2/msfilter.h>
#include <private.h>

using namespace std;

MSFilterAddFmtpCommand::MSFilterAddFmtpCommand() :
	DaemonCommand("msfilter-add-fmtp", "msfilter-add-fmtp <call/stream> <id> <fmtp>", "Add fmtp to the encoder of a call or a stream") {
}

void MSFilterAddFmtpCommand::exec(Daemon *app, const char *args) {
	char type[256], fmtp[256];
	int id;

	if (sscanf(args, "%255s %d %255s", type, &id, fmtp) == 3) {
		if(strcmp(type, "call") == 0) {
			LinphoneCall *call = app->findCall(id);
			if (call == NULL) {
				app->sendResponse(Response("No Call with such id."));
				return;
			}
			ms_filter_call_method(call->audiostream->encoder, MS_FILTER_ADD_FMTP, (void*) args);
		} else if(strcmp(type, "stream") == 0) {
			AudioStream *stream = app->findAudioStream(id);
			if (stream == NULL) {
				app->sendResponse(Response("No Audio Stream with such id."));
				return;
			}
			ms_filter_call_method(stream->encoder, MS_FILTER_ADD_FMTP, (void*) args);
		} else {
			app->sendResponse(Response("Incorrect parameter(s)."));
		}
		app->sendResponse(Response());
	} else {
		app->sendResponse(Response("Missing/Incorrect parameter(s)."));
	}
}
