#ifndef COMMAND_CALL_H_
#define COMMAND_CALL_H_

#include "../daemon.h"

class CallCommand: public DaemonCommand {
public:
	CallCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_CALL_H_
