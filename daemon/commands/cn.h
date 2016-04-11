#ifndef COMMAND_CN_H_
#define COMMAND_CN_H_

#include "../daemon.h"

class CNCommand: public DaemonCommand {
public:
	CNCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_IPV6_H_
