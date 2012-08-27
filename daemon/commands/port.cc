#include "port.h"

using namespace std;

enum PortType {
	SIPPort,
	AudioRTPPort,
	VideoRTPPort
};

enum Protocol {
	UDPProtocol,
	TCPProtocol,
	TLSProtocol
};

class PortCommandPrivate {
public:
	void outputPort(Daemon *app, ostringstream &ost, PortType type);
	void outputPorts(Daemon *app, ostringstream &ost);
};

void PortCommandPrivate::outputPort(Daemon* app, ostringstream& ost, PortType type) {
	switch (type) {
		case SIPPort:
			LCSipTransports transports;
			linphone_core_get_sip_transports(app->getCore(), &transports);
			ost << "SIP: ";
			if (transports.udp_port > 0) {
				ost << transports.udp_port << " UDP\n";
			} else if (transports.tcp_port > 0) {
				ost << transports.tcp_port << " TCP\n";
			} else {
				ost << transports.tls_port << " TLS\n";
			}
			break;
		case AudioRTPPort:
			ost << "Audio RTP: " << linphone_core_get_audio_port(app->getCore()) << "\n";
			break;
		case VideoRTPPort:
			ost << "Video RTP: " << linphone_core_get_video_port(app->getCore()) << "\n";
			break;
	}
}

void PortCommandPrivate::outputPorts(Daemon* app, ostringstream& ost) {
	outputPort(app, ost, SIPPort);
	outputPort(app, ost, AudioRTPPort);
	outputPort(app, ost, VideoRTPPort);
}

PortCommand::PortCommand() :
		DaemonCommand("port", "port [<type>] [<port>] [<protocol>]",
				"Set the port to use for type if port is set, otherwise return the port used for type if specified or all the used ports if no type is specified.\n"
				"<type> must be one of these values: sip, audio, video.\n"
				"<protocol> should be defined only for sip port and have one of these values: udp, tcp, tls."),
		d(new PortCommandPrivate()) {
}

PortCommand::~PortCommand() {
	delete d;
}

void PortCommand::exec(Daemon *app, const char *args) {
	string type;
	int port;
	istringstream ist(args);
	ostringstream ost;
	ist >> type;
	if (ist.eof() && (type.length() == 0)) {
		d->outputPorts(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	} else if (ist.fail()) {
		app->sendResponse(Response("Incorrect type parameter.", Response::Error));
	} else {
		ist >> port;
		if (ist.fail()) {
			if (type.compare("sip") == 0) {
				d->outputPort(app, ost, SIPPort);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else if (type.compare("audio") == 0) {
				d->outputPort(app, ost, AudioRTPPort);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else if (type.compare("video") == 0) {
				d->outputPort(app, ost, VideoRTPPort);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else {
				app->sendResponse(Response("Incorrect type parameter.", Response::Error));
			}
		} else {
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
				d->outputPort(app, ost, SIPPort);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else if (type.compare("audio") == 0) {
				linphone_core_set_audio_port(app->getCore(), port);
				d->outputPort(app, ost, AudioRTPPort);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else if (type.compare("video") == 0) {
				linphone_core_set_video_port(app->getCore(), port);
				d->outputPort(app, ost, VideoRTPPort);
				app->sendResponse(Response(ost.str().c_str(), Response::Ok));
			} else {
				app->sendResponse(Response("Incorrect type parameter.", Response::Error));
			}
		}
	}
}
