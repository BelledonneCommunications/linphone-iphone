#ifndef CALLCAMERA_H
#define CALLCAMERA_H

#include "daemon.h"

class Video : public DaemonCommand
{
public:
	Video();
	virtual void exec(Daemon *app, const char *args);
};


class VideoSource : public DaemonCommand
{
public:
	VideoSource();
	virtual void exec(Daemon *app, const char *args);
};

class AutoVideo : public DaemonCommand
{
public:
	AutoVideo();
	virtual void exec(Daemon *app, const char *args);
};

#endif // CALLCAMERA_H
