#ifndef COMMAND_POP_EVENT_H_
#define COMMAND_POP_EVENT_H_

#include "../daemon.h"

class PopEventCommand: public DaemonCommand {
public:
	PopEventCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_POP_EVENT_H_
