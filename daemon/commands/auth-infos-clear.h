#ifndef COMMAND_AUTH_INFOS_CLEAR_H_
#define COMMAND_AUTH_INFOS_CLEAR_H_

#include "../daemon.h"

class AuthInfosClearCommand: public DaemonCommand {
public:
	AuthInfosClearCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_AUTH_INFOS_CLEAR_H_
