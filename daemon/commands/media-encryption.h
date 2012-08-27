#ifndef COMMAND_MEDIA_ENCRYPTION_H_
#define COMMAND_MEDIA_ENCRYPTION_H_

#include "../daemon.h"

class MediaEncryptionCommand: public DaemonCommand {
public:
	MediaEncryptionCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_MEDIA_ENCRYPTION_H_
