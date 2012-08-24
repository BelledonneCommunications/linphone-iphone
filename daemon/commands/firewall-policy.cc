#include "firewall-policy.h"

using namespace std;

class FirewallPolicyCommandPrivate {
public:
	void outputFirewallPolicy(Daemon *app, ostringstream &ost);
};

void FirewallPolicyCommandPrivate::outputFirewallPolicy(Daemon* app, ostringstream& ost) {
	LinphoneFirewallPolicy policy = linphone_core_get_firewall_policy(app->getCore());
	ost << "Type: ";
	switch (policy) {
		case LinphonePolicyNoFirewall:
			ost << "none\n";
			break;
		case LinphonePolicyUseNatAddress:
			ost << "nat\n";
			ost << "Address: " << linphone_core_get_nat_address(app->getCore()) << "\n";
			break;
		case LinphonePolicyUseStun:
			ost << "stun\n";
			ost << "Address: " << linphone_core_get_stun_server(app->getCore()) << "\n";
			break;
		case LinphonePolicyUseIce:
			ost << "ice\n";
			ost << "Address: " << linphone_core_get_stun_server(app->getCore()) << "\n";
			break;
	}
}

FirewallPolicyCommand::FirewallPolicyCommand() :
		DaemonCommand("firewall-policy", "firewall-policy <type> [<address>]",
				"Set the firewall policy if type is set, otherwise return the used firewall policy.\n"
				"<type> must be one of these values: none, nat, stun, ice.\n"
				"<address> must be specified for the 'nat' and 'stun' types. "
				"It represents the public address of the gateway for the 'nat' type and the STUN server address for the 'stun' and 'ice' types."),
		d(new FirewallPolicyCommandPrivate()) {
}

FirewallPolicyCommand::~FirewallPolicyCommand() {
	delete d;
}

void FirewallPolicyCommand::exec(Daemon *app, const char *args) {
	string type;
	string address;
	istringstream ist(args);
	ist >> type;
	if (ist.eof() && (type.length() == 0)) {
		ostringstream ost;
		d->outputFirewallPolicy(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	} else if (ist.fail()) {
		app->sendResponse(Response("Incorrect type parameter.", Response::Error));
	} else {
		bool get_address;
		LinphoneFirewallPolicy policy;
		if (type.compare("none") == 0) {
			policy = LinphonePolicyNoFirewall;
			get_address = false;
		} else if (type.compare("nat") == 0) {
			policy = LinphonePolicyUseNatAddress;
			get_address = true;
		} else if (type.compare("stun") == 0) {
			policy = LinphonePolicyUseStun;
			get_address = true;
		} else if (type.compare("ice") == 0) {
			policy = LinphonePolicyUseIce;
			get_address = true;
		} else {
			app->sendResponse(Response("Incorrect type parameter.", Response::Error));
			return;
		}
		if (get_address) {
			ist >> address;
			if (ist.fail()) {
				app->sendResponse(Response("Missing/Incorrect address parameter.", Response::Error));
				return;
			}
		}
		linphone_core_set_firewall_policy(app->getCore(), policy);
		if (policy == LinphonePolicyUseNatAddress) {
			linphone_core_set_nat_address(app->getCore(), address.c_str());
		} else if ((policy == LinphonePolicyUseStun) || (policy == LinphonePolicyUseIce)) {
			linphone_core_set_stun_server(app->getCore(), address.c_str());
		}
		ostringstream ost;
		d->outputFirewallPolicy(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	}
}
