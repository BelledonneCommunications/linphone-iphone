#ifndef CALLPAUSE_H
#define CALLPAUSE_H

#include "daemon.h"

class CallPause : public DaemonCommand
{
public:
	CallPause();

	virtual void exec(Daemon *app, const char *args);
};

#endif // CALLPAUSE_H
