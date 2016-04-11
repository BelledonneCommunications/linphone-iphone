#ifndef CALLMUTE_H
#define CALLMUTE_H

#include "daemon.h"

class CallMute : public DaemonCommand
{
public:
	CallMute();
	virtual void exec(Daemon *app, const char *args);
};

#endif // CALLMUTE_H
