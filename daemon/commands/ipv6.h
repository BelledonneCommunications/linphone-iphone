#ifndef COMMAND_IPV6_H_
#define COMMAND_IPV6_H_

#include "../daemon.h"

class IPv6CommandPrivate;

class IPv6Command: public DaemonCommand {
public:
	IPv6Command();
	~IPv6Command();
	virtual void exec(Daemon *app, const char *args);
private:
	IPv6CommandPrivate *d;
};

#endif //COMMAND_IPV6_H_
