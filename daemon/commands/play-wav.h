#ifndef COMMAND_PLAY_WAV_H_
#define COMMAND_PLAY_WAV_H_

#include "../daemon.h"

class PlayWavCommand: public DaemonCommand {
public:
	PlayWavCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_PLAY_WAV_H_
