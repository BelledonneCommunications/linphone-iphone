#ifndef COMMAND_AUDIO_CODEC_ENABLE_H_
#define COMMAND_AUDIO_CODEC_ENABLE_H_

#include "../daemon.h"

class AudioCodecEnableCommand: public DaemonCommand {
public:
	AudioCodecEnableCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_CODEC_ENABLE_H_
