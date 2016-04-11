#ifndef COMMAND_HELP_H_
#define COMMAND_HELP_H_

#include "../daemon.h"

class HelpCommand: public DaemonCommand {
public:
	HelpCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_HELP_H_
