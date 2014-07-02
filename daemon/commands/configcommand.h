#ifndef COMMAND_CONFIG_H_
#define COMMAND_CONFIG_H_

#include "../daemon.h"

class ConfigGetCommand: public DaemonCommand {
public:
	ConfigGetCommand();
	virtual void exec(Daemon *app, const char *args);
};

class ConfigSetCommand: public DaemonCommand {
public:
	ConfigSetCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_IPV6_H_
