#include "play-wav.h"

using namespace std;

PlayWavCommand::PlayWavCommand() :
		DaemonCommand("play-wav", "play-wav <filename>",
				"Play an WAV audio file (needs to have enabled the linphone sound daemon (LSD).\n"
				"<filename> is the WAV file to be played.") {
	addExample(new DaemonCommandExample("play-wav /usr/local/share/sounds/linphone/hello8000.wav",
						"Status: Ok"));
}

void playWavFinished(LsdPlayer *p) {
	linphone_sound_daemon_release_player(lsd_player_get_daemon(p), p);
}

void PlayWavCommand::exec(Daemon *app, const char *args) {
	LinphoneSoundDaemon *lsd = app->getLSD();
	if (!lsd) {
		app->sendResponse(Response("The linphone sound daemon (LSD) is not enabled.", Response::Error));
		return;
	}

	string filename;
	istringstream ist(args);
	ist >> filename;
	if (ist.eof() && (filename.length() == 0)) {
		app->sendResponse(Response("Missing filename parameter.", Response::Error));
	} else if (ist.fail()) {
		app->sendResponse(Response("Incorrect filename parameter.", Response::Error));
	} else {
		LsdPlayer *p = linphone_sound_daemon_get_player(lsd);
		lsd_player_set_callback(p, playWavFinished);
		lsd_player_play(p, filename.c_str());
		app->sendResponse(Response());
	}
}
