#ifndef COMMAND_DTMF_H_
#define COMMAND_DTMF_H_

#include "../daemon.h"

class DtmfCommand: public DaemonCommand {
public:
	DtmfCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_DTMF_H_
