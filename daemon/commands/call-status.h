#ifndef COMMAND_CALL_STATUS_H_
#define COMMAND_CALL_STATUS_H_

#include "../daemon.h"

class CallStatusCommand: public DaemonCommand {
public:
	CallStatusCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_CALL_STATUS_H_
