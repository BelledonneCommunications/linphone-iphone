#include "adaptive-jitter-compensation.h"

using namespace std;

class AdaptiveBufferCompensationCommandPrivate {
public:
	enum StreamType {
		AudioStream,
		VideoStream
	};

	void outputAdaptiveBufferCompensation(Daemon *app, ostringstream &ost, StreamType type);
	void outputAdaptiveBufferCompensations(Daemon *app, ostringstream &ost);
};

void AdaptiveBufferCompensationCommandPrivate::outputAdaptiveBufferCompensation(Daemon* app, ostringstream& ost, StreamType type) {
	bool enabled = false;
	switch (type) {
		case AudioStream:
			enabled = linphone_core_audio_adaptive_jittcomp_enabled(app->getCore());
			ost << "Audio: ";
			break;
		case VideoStream:
			enabled = linphone_core_video_adaptive_jittcomp_enabled(app->getCore());
			ost << "Video: ";
			break;
	}
	if (enabled) {
		ost << "enabled\n";
	} else {
		ost << "disabled\n";
	}
}

void AdaptiveBufferCompensationCommandPrivate::outputAdaptiveBufferCompensations(Daemon* app, ostringstream& ost)
{
	outputAdaptiveBufferCompensation(app, ost, AdaptiveBufferCompensationCommandPrivate::AudioStream);
	outputAdaptiveBufferCompensation(app, ost, AdaptiveBufferCompensationCommandPrivate::VideoStream);
}

AdaptiveBufferCompensationCommand::AdaptiveBufferCompensationCommand() :
		DaemonCommand("adaptive-jitter-compensation", "adaptive-jitter-compensation [<stream>] [enable|disable]",
				"Enable or disable adaptive buffer compensation respectively with the 'enable' and 'disable' parameters for the specified stream, "
				"return the status of the use of adaptive buffer compensation without parameter.\n"
				"<stream> must be one of these values: audio, video."),
		d(new AdaptiveBufferCompensationCommandPrivate()) {
}

AdaptiveBufferCompensationCommand::~AdaptiveBufferCompensationCommand() {
	delete d;
}

void AdaptiveBufferCompensationCommand::exec(Daemon *app, const char *args) {
	string stream;
	string state;
	istringstream ist(args);
	ostringstream ost;
	ist >> stream;
	if (ist.fail()) {
		d->outputAdaptiveBufferCompensations(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	} else {
		ist >> state;
		if (ist.fail()) {
			if (stream.compare("audio") == 0) {
				d->outputAdaptiveBufferCompensation(app, ost, AdaptiveBufferCompensationCommandPrivate::AudioStream);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else if (stream.compare("video") == 0) {
				d->outputAdaptiveBufferCompensation(app, ost, AdaptiveBufferCompensationCommandPrivate::VideoStream);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else {
				app->sendResponse(Response("Incorrect stream parameter.", Response::Error));
			}
		} else {
			AdaptiveBufferCompensationCommandPrivate::StreamType type;
			bool enabled;
			if (stream.compare("audio") == 0) {
				type = AdaptiveBufferCompensationCommandPrivate::AudioStream;
			} else if (stream.compare("video") == 0) {
				type = AdaptiveBufferCompensationCommandPrivate::VideoStream;
			} else {
				app->sendResponse(Response("Incorrect stream parameter.", Response::Error));
				return;
			}
			if (state.compare("enable") == 0) {
				enabled = TRUE;
			} else if (state.compare("disable") == 0) {
				enabled = FALSE;
			} else {
				app->sendResponse(Response("Incorrect parameter.", Response::Error));
				return;
			}
			switch (type) {
				case AdaptiveBufferCompensationCommandPrivate::AudioStream:
					linphone_core_enable_audio_adaptive_jittcomp(app->getCore(), enabled);
					break;
				case AdaptiveBufferCompensationCommandPrivate::VideoStream:
					linphone_core_enable_video_adaptive_jittcomp(app->getCore(), enabled);
					break;
			}
			d->outputAdaptiveBufferCompensation(app, ost, type);
			app->sendResponse(Response(ost.str().c_str(), Response::Ok));
		}
	}
}
