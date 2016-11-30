/*
port.cc
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

#include "port.h"

using namespace std;

enum Protocol {
	UDPProtocol,
	TCPProtocol,
	TLSProtocol
};

class PortResponse : public Response {
public:
	enum PortType {
		SIPPort,
		AudioRTPPort,
		VideoRTPPort,
		AllPorts
	};

	PortResponse(LinphoneCore *core, PortType type);

private:
	void outputSIPPort(LinphoneCore *core, ostringstream &ost);
	void outputAudioRTPPort(LinphoneCore *core, ostringstream &ost);
	void outputVideoRTPPort(LinphoneCore *core, ostringstream &ost);
};

PortResponse::PortResponse(LinphoneCore *core, PortResponse::PortType type) : Response() {
	ostringstream ost;
	switch (type) {
		case SIPPort:
			outputSIPPort(core, ost);
			break;
		case AudioRTPPort:
			outputAudioRTPPort(core, ost);
			break;
		case VideoRTPPort:
			outputVideoRTPPort(core, ost);
			break;
		case AllPorts:
			outputSIPPort(core, ost);
			outputAudioRTPPort(core, ost);
			outputVideoRTPPort(core, ost);
			break;
	}
	setBody(ost.str());
}

void PortResponse::outputSIPPort(LinphoneCore *core, ostringstream &ost) {
	LCSipTransports transports;
	linphone_core_get_sip_transports(core, &transports);
	ost << "SIP: ";
	if (transports.udp_port > 0) {
		ost << transports.udp_port << " UDP\n";
	} else if (transports.tcp_port > 0) {
		ost << transports.tcp_port << " TCP\n";
	} else {
		ost << transports.tls_port << " TLS\n";
	}
}

void PortResponse::outputAudioRTPPort(LinphoneCore *core, ostringstream &ost) {
	ost << "Audio RTP: " << linphone_core_get_audio_port(core) << "\n";
}

void PortResponse::outputVideoRTPPort(LinphoneCore *core, ostringstream &ost) {
	ost << "Video RTP: " << linphone_core_get_video_port(core) << "\n";
}

PortCommand::PortCommand() :
		DaemonCommand("port", "port [sip|audio|video] [<port>] [udp|tcp|tls]",
				"Set the port to use for type if port is set, otherwise return the port used for type if specified or all the used ports if no type is specified.\n"
				"The protocol should be defined only for sip port and have one of these values: udp, tcp, tls.") {
	addExample(new DaemonCommandExample("port sip 5060 tls",
						"Status: Ok\n\n"
						"SIP: 5060 TLS"));
	addExample(new DaemonCommandExample("port sip 5060 udp",
						"Status: Ok\n\n"
						"SIP: 5060 UDP"));
	addExample(new DaemonCommandExample("port audio 7078",
						"Status: Ok\n\n"
						"Audio RTP: 7078"));
	addExample(new DaemonCommandExample("port video 9078",
						"Status: Ok\n\n"
						"Video RTP: 9078"));
	addExample(new DaemonCommandExample("port sip",
						"Status: Ok\n\n"
						"SIP: 5060 UDP"));
	addExample(new DaemonCommandExample("port audio",
						"Status: Ok\n\n"
						"Audio RTP: 7078"));
	addExample(new DaemonCommandExample("port",
						"Status: Ok\n\n"
						"SIP: 5060 UDP\n"
						"Audio RTP: 7078\n"
						"Video RTP: 9078"));
}

void PortCommand::exec(Daemon *app, const string& args) {
	string type;
	int port;
	istringstream ist(args);
	ostringstream ost;
	ist >> type;
	if (ist.eof() && (type.length() == 0)) {
		app->sendResponse(PortResponse(app->getCore(), PortResponse::AllPorts));
		return;
	}
	if (ist.fail()) {
		app->sendResponse(Response("Incorrect type parameter.", Response::Error));
		return;
	}
	ist >> port;
	if (ist.fail()) {
		if (type.compare("sip") == 0) {
			app->sendResponse(PortResponse(app->getCore(), PortResponse::SIPPort));
		} else if (type.compare("audio") == 0) {
			app->sendResponse(PortResponse(app->getCore(), PortResponse::AudioRTPPort));
		} else if (type.compare("video") == 0) {
			app->sendResponse(PortResponse(app->getCore(), PortResponse::VideoRTPPort));
		} else {
			app->sendResponse(Response("Incorrect type parameter.", Response::Error));
		}
		return;
	}
	if (type.compare("sip") == 0) {
		Protocol protocol = UDPProtocol;
		string protocol_str;
		ist >> protocol_str;
		if (!ist.fail()) {
			if (protocol_str.compare("udp") == 0) {
				protocol = UDPProtocol;
			} else if (protocol_str.compare("tcp") == 0) {
				protocol = TCPProtocol;
			} else if (protocol_str.compare("tls") == 0) {
				protocol = TLSProtocol;
			} else {
				app->sendResponse(Response("Incorrect protocol parameter.", Response::Error));
				return;
			}
		}
		LCSipTransports transports;
		memset(&transports, 0, sizeof(transports));
		switch (protocol) {
			case UDPProtocol:
				transports.udp_port = port;
				break;
			case TCPProtocol:
				transports.tcp_port = port;
				break;
			case TLSProtocol:
				transports.tls_port = port;
				break;
		}
		linphone_core_set_sip_transports(app->getCore(), &transports);
		app->sendResponse(PortResponse(app->getCore(), PortResponse::SIPPort));
	} else if (type.compare("audio") == 0) {
		linphone_core_set_audio_port(app->getCore(), port);
		app->sendResponse(PortResponse(app->getCore(), PortResponse::AudioRTPPort));
	} else if (type.compare("video") == 0) {
		linphone_core_set_video_port(app->getCore(), port);
		app->sendResponse(PortResponse(app->getCore(), PortResponse::VideoRTPPort));
	} else {
		app->sendResponse(Response("Incorrect type parameter.", Response::Error));
	}
}
