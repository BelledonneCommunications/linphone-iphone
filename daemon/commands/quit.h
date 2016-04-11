#ifndef COMMAND_QUIT_H_
#define COMMAND_QUIT_H_

#include "../daemon.h"

class QuitCommand: public DaemonCommand {
public:
	QuitCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_QUIT_H_
