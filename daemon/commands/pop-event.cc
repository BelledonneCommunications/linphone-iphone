#include "pop-event.h"

using namespace std;

PopEventCommand::PopEventCommand() :
		DaemonCommand("pop-event", "pop-event", "Pop an event from event queue and display it.") {
}
void PopEventCommand::exec(Daemon *app, const char *args) {
	app->pullEvent();
}
