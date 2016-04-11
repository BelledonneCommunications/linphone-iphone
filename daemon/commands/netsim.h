#ifndef COMMAND_NETSIM_H_
#define COMMAND_NETSIM_H_

#include "../daemon.h"

class NetsimCommand: public DaemonCommand {
public:
	NetsimCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_NETSIM_H_
