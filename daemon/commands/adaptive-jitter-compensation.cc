#include "adaptive-jitter-compensation.h"

using namespace std;

class AdaptiveBufferCompensationResponse : public Response {
public:
	enum StreamType {
		AudioStream,
		VideoStream,
		AllStreams
	};

	AdaptiveBufferCompensationResponse(LinphoneCore *core, StreamType type);

private:
	void outputAdaptiveBufferCompensation(LinphoneCore *core, ostringstream &ost, const char *header, bool_t value);
};

AdaptiveBufferCompensationResponse::AdaptiveBufferCompensationResponse(LinphoneCore *core, StreamType type) : Response() {
	bool enabled = false;
	ostringstream ost;
	switch (type) {
		case AudioStream:
			enabled = linphone_core_audio_adaptive_jittcomp_enabled(core);
			outputAdaptiveBufferCompensation(core, ost, "Audio", enabled);
			break;
		case VideoStream:
			enabled = linphone_core_video_adaptive_jittcomp_enabled(core);
			outputAdaptiveBufferCompensation(core, ost, "Video", enabled);
			break;
		case AllStreams:
			enabled = linphone_core_audio_adaptive_jittcomp_enabled(core);
			outputAdaptiveBufferCompensation(core, ost, "Audio", enabled);
			enabled = linphone_core_video_adaptive_jittcomp_enabled(core);
			outputAdaptiveBufferCompensation(core, ost, "Video", enabled);
			break;
	}
	setBody(ost.str().c_str());
}

void AdaptiveBufferCompensationResponse::outputAdaptiveBufferCompensation(LinphoneCore *core, ostringstream &ost, const char *header, bool_t value) {
	ost << header << ": ";
	if (value) {
		ost << "enabled\n";
	} else {
		ost << "disabled\n";
	}
}

AdaptiveBufferCompensationCommand::AdaptiveBufferCompensationCommand() :
		DaemonCommand("adaptive-jitter-compensation", "adaptive-jitter-compensation [<stream>] [enable|disable]",
				"Enable or disable adaptive buffer compensation respectively with the 'enable' and 'disable' parameters for the specified stream, "
				"return the status of the use of adaptive buffer compensation without parameter.\n"
				"<stream> must be one of these values: audio, video.") {
	addExample(new DaemonCommandExample("adaptive-jitter-compensation audio",
						"Status: Ok\n\n"
						"Audio: enabled"));
	addExample(new DaemonCommandExample("adaptive-jitter-compensation video",
						"Status: Ok\n\n"
						"Video: disabled"));
	addExample(new DaemonCommandExample("adaptive-jitter-compensation",
						"Status: Ok\n\n"
						"Audio: enabled\n"
						"Video: disabled"));
	addExample(new DaemonCommandExample("adaptive-jitter-compensation video enable",
						"Status: Ok\n\n"
						"Video: enabled"));
}

void AdaptiveBufferCompensationCommand::exec(Daemon *app, const char *args) {
	string stream;
	string state;
	istringstream ist(args);
	ist >> stream;
	if (ist.fail()) {
		app->sendResponse(AdaptiveBufferCompensationResponse(app->getCore(), AdaptiveBufferCompensationResponse::AllStreams));
	} else {
		ist >> state;
		if (ist.fail()) {
			if (stream.compare("audio") == 0) {
				app->sendResponse(AdaptiveBufferCompensationResponse(app->getCore(), AdaptiveBufferCompensationResponse::AudioStream));
			} else if (stream.compare("video") == 0) {
				app->sendResponse(AdaptiveBufferCompensationResponse(app->getCore(), AdaptiveBufferCompensationResponse::VideoStream));
			} else {
				app->sendResponse(Response("Incorrect stream parameter.", Response::Error));
			}
		} else {
			AdaptiveBufferCompensationResponse::StreamType type;
			bool enabled;
			if (stream.compare("audio") == 0) {
				type = AdaptiveBufferCompensationResponse::AudioStream;
			} else if (stream.compare("video") == 0) {
				type = AdaptiveBufferCompensationResponse::VideoStream;
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
			if (type == AdaptiveBufferCompensationResponse::AudioStream) {
				linphone_core_enable_audio_adaptive_jittcomp(app->getCore(), enabled);
			} else if (type == AdaptiveBufferCompensationResponse::VideoStream) {
				linphone_core_enable_video_adaptive_jittcomp(app->getCore(), enabled);
			}
			app->sendResponse(AdaptiveBufferCompensationResponse(app->getCore(), AdaptiveBufferCompensationResponse::AllStreams));
		}
	}
}
