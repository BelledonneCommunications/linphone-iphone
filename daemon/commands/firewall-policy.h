#ifndef COMMAND_FIREWALL_POLICY_H_
#define COMMAND_FIREWALL_POLICY_H_

#include "../daemon.h"

class FirewallPolicyCommandPrivate;

class FirewallPolicyCommand: public DaemonCommand {
public:
	FirewallPolicyCommand();
	~FirewallPolicyCommand();
	virtual void exec(Daemon *app, const char *args);
private:
	FirewallPolicyCommandPrivate *d;
};

#endif //COMMAND_FIREWALL_POLICY_H_
