#ifndef COMMAND_TERMINATE_H_
#define COMMAND_TERMINATE_H_

#include "../daemon.h"

class TerminateCommand: public DaemonCommand {
public:
	TerminateCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_TERMINATE_H_
