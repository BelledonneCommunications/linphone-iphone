#ifndef COMMAND_FIREWALL_POLICY_H_
#define COMMAND_FIREWALL_POLICY_H_

#include "../daemon.h"

class FirewallPolicyCommand: public DaemonCommand {
public:
	FirewallPolicyCommand();
	virtual void exec(Daemon *app, const char *args);
};

#endif //COMMAND_FIREWALL_POLICY_H_
