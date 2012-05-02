#ifndef COMMAND_AUDIO_CODEC_DISABLE_H_
#define COMMAND_AUDIO_CODEC_DISABLE_H_

#include "../daemon.h"

class AudioCodecDisableCommand: public DaemonCommand {
public:
	AudioCodecDisableCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_CODEC_DISABLE_H_
