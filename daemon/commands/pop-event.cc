#include "pop-event.h"

using namespace std;

PopEventCommand::PopEventCommand() :
		DaemonCommand("pop-event", "pop-event", "Pop an event from event queue and display it.") {
	addExample(new DaemonCommandExample("pop-event",
						"Status: Ok\n\n"
						"Size: 0"));
}
void PopEventCommand::exec(Daemon *app, const char *args) {
	app->pullEvent();
}
