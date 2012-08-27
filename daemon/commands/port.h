#ifndef COMMAND_PORT_H_
#define COMMAND_PORT_H_

#include "../daemon.h"

class PortCommandPrivate;

class PortCommand: public DaemonCommand {
public:
	PortCommand();
	~PortCommand();
	virtual void exec(Daemon *app, const char *args);
private:
	PortCommandPrivate *d;
};

#endif //COMMAND_PORT_H_
