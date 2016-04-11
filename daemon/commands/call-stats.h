#ifndef COMMAND_CALL_STATS_H_
#define COMMAND_CALL_STATS_H_

#include "../daemon.h"

class CallStatsCommand: public DaemonCommand {
public:
	CallStatsCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_CALL_STATS_H_
