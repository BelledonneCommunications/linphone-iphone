#ifndef COMMAND_MEDIA_ENCRYPTION_H_
#define COMMAND_MEDIA_ENCRYPTION_H_

#include "../daemon.h"

class MediaEncryptionCommandPrivate;

class MediaEncryptionCommand: public DaemonCommand {
public:
	MediaEncryptionCommand();
	~MediaEncryptionCommand();
	virtual void exec(Daemon *app, const char *args);
private:
	MediaEncryptionCommandPrivate *d;
};

#endif //COMMAND_MEDIA_ENCRYPTION_H_
