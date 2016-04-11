#ifndef COMMAND_VERSION_H_
#define COMMAND_VERSION_H_

#include "../daemon.h"

class VersionCommand: public DaemonCommand {
public:
	VersionCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_VERSION_H_
