#ifndef COMMAND_MSFILTER_ADD_FMTP
#define COMMAND_MSFILTER_ADD_FMTP

#include "../daemon.h"

class MSFilterAddFmtpCommand : public DaemonCommand {
public:
	MSFilterAddFmtpCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif /* COMMAND_MSFILTER_ADD_FMTP */
