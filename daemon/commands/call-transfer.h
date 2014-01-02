#ifndef CALLTRANSFER_H
#define CALLTRANSFER_H

#include "daemon.h"

class CallTransfer : public DaemonCommand
{
public:
	CallTransfer();
	virtual void exec(Daemon *app, const char *args);
};

#endif // CALLTRANSFER_H
