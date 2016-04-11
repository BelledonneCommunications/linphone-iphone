#ifndef COMMAND_AUDIO_CODEC_GET_H_
#define COMMAND_AUDIO_CODEC_GET_H_

#include "../daemon.h"

class AudioCodecGetCommand: public DaemonCommand {
public:
	AudioCodecGetCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_CODEC_GET_H_
