#ifndef COMMAND_IPV6_H_
#define COMMAND_IPV6_H_

#include "../daemon.h"

class IPv6Command: public DaemonCommand {
public:
	IPv6Command();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_IPV6_H_
