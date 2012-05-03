#ifndef COMMAND_AUDIO_CODEC_SET_H_
#define COMMAND_AUDIO_CODEC_SET_H_

#include "../daemon.h"

class AudioCodecSetCommand: public DaemonCommand {
public:
	AudioCodecSetCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_CODEC_SET_H_
