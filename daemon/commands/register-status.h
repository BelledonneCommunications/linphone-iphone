#ifndef COMMAND_REGISTER_STATUS_H_
#define COMMAND_REGISTER_STATUS_H_

#include "../daemon.h"

class RegisterStatusCommand: public DaemonCommand {
public:
	RegisterStatusCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_REGISTER_STATUS_H_
