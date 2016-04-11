#ifndef COMMAND_PORT_H_
#define COMMAND_PORT_H_

#include "../daemon.h"

class PortCommand: public DaemonCommand {
public:
	PortCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_PORT_H_
