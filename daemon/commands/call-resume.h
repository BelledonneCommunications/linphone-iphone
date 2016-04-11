#ifndef CALLRESUME_H
#define CALLRESUME_H

#include "daemon.h"

class CallResume : public DaemonCommand
{
public:
	CallResume();
	virtual void exec(Daemon *app, const char *args);
};

#endif // CALLRESUME_H
