#ifndef COMMAND_PTIME_H_
#define COMMAND_PTIME_H_

#include "../daemon.h"

class PtimeCommandPrivate;

class PtimeCommand: public DaemonCommand {
public:
	PtimeCommand();
	~PtimeCommand();
	virtual void exec(Daemon *app, const char *args);
private:
	PtimeCommandPrivate *d;
};

#endif //COMMAND_PTIME_H_
