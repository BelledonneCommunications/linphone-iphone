#ifndef COMMAND_AUDIO_STREAM_STATS_H_
#define COMMAND_AUDIO_STREAM_STATS_H_

#include "../daemon.h"

class AudioStreamStatsCommand: public DaemonCommand {
public:
	AudioStreamStatsCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUDIO_STREAM_STATS_H_
