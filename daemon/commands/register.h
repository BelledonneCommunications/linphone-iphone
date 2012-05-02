#ifndef COMMAND_REGISTER_H_
#define COMMAND_REGISTER_H_

#include "../daemon.h"

class RegisterCommand: public DaemonCommand {
public:
	RegisterCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_REGISTER_H_
