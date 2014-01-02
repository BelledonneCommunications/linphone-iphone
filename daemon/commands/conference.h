#ifndef CONFERENCE_H
#define CONFERENCE_H

#include "daemon.h"

class Conference : public DaemonCommand
{
public:
	Conference();
	virtual void exec(Daemon *app, const char *args);
};

#endif // CONFERENCE_H
