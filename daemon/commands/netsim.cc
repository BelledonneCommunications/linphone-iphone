/*
netsim.cc
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

#include "netsim.h"

using namespace std;

class NetsimResponse : public Response {
public:
	NetsimResponse(LinphoneCore *core);
};

NetsimResponse::NetsimResponse(LinphoneCore *lc) : Response() {
	ostringstream ost;
	const OrtpNetworkSimulatorParams *params=linphone_core_get_network_simulator_params(lc);
	ost << "State: ";
	if (params->enabled) {
		ost << "enabled\n";
	} else {
		ost << "disabled\n";
	}
	ost<<"max_bandwidth: "<<params->max_bandwidth<<endl;
	ost<<"max_buffer_size: "<<params->max_buffer_size<<endl;
	ost<<"loss_rate: "<<params->loss_rate<<endl;
	ost<<"latency: "<<params->latency<<endl;
	ost<<"consecutive_loss_probability: "<<params->consecutive_loss_probability<<endl;
	ost<<"jitter_burst_density: "<<params->jitter_burst_density<<endl;
	ost<<"jitter_strength: "<<params->jitter_strength<<endl;
	ost<<"mode: "<<ortp_network_simulator_mode_to_string(params->mode)<<endl;
	setBody(ost.str());
}

NetsimCommand::NetsimCommand(): DaemonCommand("netsim","netsim [enable|disable|parameters] [<parameters>]",
	"Configure the network simulator. Parameters are to be provided in the form param-name=param-value, separated with ';' only. Supported parameters are:\n"
	"\tmax_bandwidth (kbit/s)\n"
	"\tmax_buffer_size (bits)\n"
	"\tloss_rate (percentage)\n"
	"\tlatency (milliseconds)\n"
	"\tconsecutive_loss_probability (0..1)\n"
	"\tjitter_burst_density (frequency of bursts 0..1)\n"
	"\tjitter_strength (percentage of max_bandwidth artifically consumed during bursts events)\n"
	"\tmode (inbound, outbound, outbound-controlled)\n")
{
	addExample(new DaemonCommandExample("netsim",
						"Status: Ok\n\n"
						"State: disabled\nmax_bandwidth: 384000\nmax_buffer_size: 384000\nloss_rate: 2"));
	addExample(new DaemonCommandExample("netsim enable",
						"Status: Ok\n\n"
						"State: enabled\nmax_bandwidth: 384000\nmax_buffer_size: 384000\nloss_rate: 2"));
	addExample(new DaemonCommandExample("netsim parameters loss_rate=5;consecutive_loss_probability=0.5",
						"Status: Ok\n\n"
						"State: enabled\nmax_bandwidth: 384000\nmax_buffer_size: 384000\nloss_rate: 2"));
}

void NetsimCommand::exec(Daemon* app, const string& args) {
	LinphoneCore *lc=app->getCore();
	OrtpNetworkSimulatorParams params=*linphone_core_get_network_simulator_params(lc);
	string subcommand;
	istringstream ist(args);
	ist >> subcommand;
	if (ist.fail()) {
		app->sendResponse(NetsimResponse(app->getCore()));
		return;
	}
	if (subcommand.compare("enable")==0){
		params.enabled = TRUE;
	}else if (subcommand.compare("disable")==0){
		params.enabled = FALSE;
	}else if (subcommand.compare("parameters")==0){
		string parameters;
		char value[128] = { 0 };
		ist >> parameters;
		if (fmtp_get_value(parameters.c_str(), "max_bandwidth", value, sizeof(value))) {
			params.max_bandwidth = (float)atoi(value);
		}
		if (fmtp_get_value(parameters.c_str(), "max_buffer_size", value, sizeof(value))) {
			params.max_buffer_size = atoi(value);
		}
		if (fmtp_get_value(parameters.c_str(), "loss_rate", value, sizeof(value))) {
			params.loss_rate = (float)atoi(value);
		}
		if (fmtp_get_value(parameters.c_str(), "latency", value, sizeof(value))) {
			params.latency = atoi(value);
		}
		if (fmtp_get_value(parameters.c_str(), "consecutive_loss_probability", value, sizeof(value))) {
			params.consecutive_loss_probability = (float)atof(value);
		}
		if (fmtp_get_value(parameters.c_str(), "jitter_burst_density", value, sizeof(value))) {
			params.jitter_burst_density = (float)atof(value);
		}
		if (fmtp_get_value(parameters.c_str(), "jitter_strength", value, sizeof(value))) {
			params.jitter_strength = (float)atof(value);
		}
		if (fmtp_get_value(parameters.c_str(), "mode",value, sizeof(value))) {
			OrtpNetworkSimulatorMode mode = ortp_network_simulator_mode_from_string(value);
			if (mode == OrtpNetworkSimulatorInvalid) {
				app->sendResponse(Response("Invalid mode"));
				return;
			}
			params.mode = mode;
		}
	}
	linphone_core_set_network_simulator_params(lc, &params);
	app->sendResponse(NetsimResponse(app->getCore()));
}



