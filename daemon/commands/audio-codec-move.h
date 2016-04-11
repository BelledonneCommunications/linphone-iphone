#ifndef COMMAND_AUDIO_CODEC_MOVE_H_
#define COMMAND_AUDIO_CODEC_MOVE_H_

#include "../daemon.h"

class AudioCodecMoveCommand: public DaemonCommand {
public:
	AudioCodecMoveCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_CODEC_MOVE_H_
