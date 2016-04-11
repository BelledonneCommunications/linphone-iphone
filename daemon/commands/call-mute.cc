#include "call-mute.h"

CallMute::CallMute() :
	DaemonCommand("call-mute", "call-mute 0|1", "mute/unmute the microphone (1 to mute, 0 to unmute). No argument means MUTE.")
{
	addExample(new DaemonCommandExample("call-mute 1",
										"Status: Ok\n\n"
										"Microphone Muted"));
	addExample(new DaemonCommandExample("call-mute",
										"Status: Ok\n\n"
										"Microphone Muted"));
	addExample(new DaemonCommandExample("call-mute 0",
										"Status: Ok\n\n"
										"Microphone Unmuted"));
	addExample(new DaemonCommandExample("call-mute 1",
										"Status: Error\n\n"
										"Reason: No call in progress. Can't mute."));
}

void CallMute::exec(Daemon* app, const char* args)
{
	LinphoneCore *lc = app->getCore();
	int muted = TRUE; // no arg means MUTE
	LinphoneCall *call = linphone_core_get_current_call(lc);

	if( call == NULL ){
		app->sendResponse(Response("No call in progress. Can't mute."));
		return;
	}

	if (sscanf(args, "%i", &muted) == 1) {
		linphone_core_enable_mic(lc, (muted != 0));
	} else {
		linphone_core_enable_mic(lc, (muted != 0));
	}

	app->sendResponse(Response(muted?"Microphone Muted"
									:"Microphone Unmuted",
							   Response::Ok));
}
