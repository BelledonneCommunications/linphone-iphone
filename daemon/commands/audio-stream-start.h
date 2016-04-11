#ifndef COMMAND_AUDIO_STREAM_START_H_
#define COMMAND_AUDIO_STREAM_START_H_

#include "../daemon.h"

class AudioStreamStartCommand: public DaemonCommand {
public:
	AudioStreamStartCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_STREAM_START_H_
