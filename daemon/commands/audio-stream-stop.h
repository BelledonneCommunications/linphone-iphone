#ifndef COMMAND_AUDIO_STREAM_STOP_H_
#define COMMAND_AUDIO_STREAM_STOP_H_

#include "../daemon.h"

class AudioStreamStopCommand: public DaemonCommand {
public:
	AudioStreamStopCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_STREAM_STOP_H_
