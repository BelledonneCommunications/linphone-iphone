#ifndef COMMAND_CONTACT_H_
#define COMMAND_CONTACT_H_

#include "../daemon.h"

class ContactCommand: public DaemonCommand {
public:
	ContactCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_CONTACT_H_
