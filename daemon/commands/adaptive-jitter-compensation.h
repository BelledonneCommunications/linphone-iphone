#ifndef COMMAND_ADAPTIVE_BUFFER_COMPENSATION_H_
#define COMMAND_ADAPTIVE_BUFFER_COMPENSATION_H_

#include "../daemon.h"

class AdaptiveBufferCompensationCommandPrivate;

class AdaptiveBufferCompensationCommand: public DaemonCommand {
public:
	AdaptiveBufferCompensationCommand();
	~AdaptiveBufferCompensationCommand();
	virtual void exec(Daemon *app, const char *args);
private:
	AdaptiveBufferCompensationCommandPrivate *d;
};

#endif //COMMAND_ADAPTIVE_BUFFER_COMPENSATION_H_
