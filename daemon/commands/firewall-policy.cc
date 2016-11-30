/*
firewall-policy.cc
Copyright (C) 2016 Belledonne Communications, Grenoble, France 

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "firewall-policy.h"

using namespace std;

class FirewallPolicyResponse : public Response {
public:
	FirewallPolicyResponse(LinphoneCore *core);
};

FirewallPolicyResponse::FirewallPolicyResponse(LinphoneCore *core) : Response() {
	ostringstream ost;
	LinphoneFirewallPolicy policy = linphone_core_get_firewall_policy(core);
	ost << "Type: ";
	switch (policy) {
		case LinphonePolicyNoFirewall:
			ost << "none\n";
			break;
		case LinphonePolicyUseNatAddress:
			ost << "nat\n";
			ost << "Address: " << linphone_core_get_nat_address(core) << "\n";
			break;
		case LinphonePolicyUseStun:
			ost << "stun\n";
			ost << "Address: " << linphone_core_get_stun_server(core) << "\n";
			break;
		case LinphonePolicyUseIce:
			ost << "ice\n";
			ost << "Address: " << linphone_core_get_stun_server(core) << "\n";
			break;
		case LinphonePolicyUseUpnp:
			ost << "upnp\n";
			break;
	}
	setBody(ost.str());
}

FirewallPolicyCommand::FirewallPolicyCommand() :
		DaemonCommand("firewall-policy", "firewall-policy [none|nat|stun|ice|upnp] [<address>]",
				"Set the firewall policy if type is set, otherwise return the used firewall policy.\n"
				"<address> must be specified for the 'nat' and 'stun' types. "
				"It represents the public address of the gateway for the 'nat' type and the STUN server address for the 'stun' and 'ice' types.") {
	addExample(new DaemonCommandExample("firewall-policy stun stun.linphone.org",
						"Status: Ok\n\n"
						"Type: stun\n"
						"Address: stun.linphone.org"));
	addExample(new DaemonCommandExample("firewall-policy none",
						"Status: Ok\n\n"
						"Type: none"));
	addExample(new DaemonCommandExample("firewall-policy",
						"Status: Ok\n\n"
						"Type: none"));
}

void FirewallPolicyCommand::exec(Daemon *app, const string& args) {
	string type;
	string address;
	istringstream ist(args);
	ist >> type;
	if (ist.eof() && (type.length() == 0)) {
		app->sendResponse(FirewallPolicyResponse(app->getCore()));
		return;
	}
	if (ist.fail()) {
		app->sendResponse(Response("Incorrect type parameter.", Response::Error));
		return;
	}

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
	} else if (type.compare("upnp") == 0) {
		policy = LinphonePolicyUseUpnp;
		get_address = false;
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
	app->sendResponse(FirewallPolicyResponse(app->getCore()));
}
