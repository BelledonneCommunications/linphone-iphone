#ifndef COMMAND_AUDIO_CODEC_TOGGLE_H_
#define COMMAND_AUDIO_CODEC_TOGGLE_H_

#include "../daemon.h"

class AudioCodecToggleCommand: public DaemonCommand {
public:
	AudioCodecToggleCommand(const char *name, const char *proto, const char *help, bool enable);
	virtual void exec(Daemon *app, const char *args);
protected:
	bool mEnable;
};

class AudioCodecEnableCommand: public AudioCodecToggleCommand {
public:
	AudioCodecEnableCommand();
};

class AudioCodecDisableCommand: public AudioCodecToggleCommand {
public:
	AudioCodecDisableCommand();
};

#endif //COMMAND_AUDIO_CODEC_TOGGLE_H_
