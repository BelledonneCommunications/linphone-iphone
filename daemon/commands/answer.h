#ifndef COMMAND_ANSWER_H_
#define COMMAND_ANSWER_H_

#include "../daemon.h"

class AnswerCommand: public DaemonCommand {
public:
	AnswerCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_ANSWER_H_
