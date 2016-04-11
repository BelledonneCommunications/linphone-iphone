#ifndef COMMAND_PTIME_H_
#define COMMAND_PTIME_H_

#include "../daemon.h"

class PtimeCommand: public DaemonCommand {
public:
	PtimeCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_PTIME_H_
