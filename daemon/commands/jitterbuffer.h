#ifndef COMMAND_JITTER_SIZE_H_
#define COMMAND_JITTER_SIZE_H_

#include "../daemon.h"


class JitterBufferCommand : public DaemonCommand{
public:
    JitterBufferCommand();
    virtual void exec(Daemon *app, const char *args);
};


class JitterBufferResetCommand : public DaemonCommand{
public:
    JitterBufferResetCommand();
    virtual void exec(Daemon *app, const char *args);
};

#endif
