#ifndef COMMAND_UNREGISTER_H_
#define COMMAND_UNREGISTER_H_

#include "../daemon.h"

class UnregisterCommand: public DaemonCommand {
public:
	UnregisterCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_UNREGISTER_H_
