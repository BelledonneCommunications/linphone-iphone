#ifndef COMMAND_ADAPTIVE_BUFFER_COMPENSATION_H_
#define COMMAND_ADAPTIVE_BUFFER_COMPENSATION_H_

#include "../daemon.h"

class AdaptiveBufferCompensationCommand: public DaemonCommand {
public:
	AdaptiveBufferCompensationCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_ADAPTIVE_BUFFER_COMPENSATION_H_
