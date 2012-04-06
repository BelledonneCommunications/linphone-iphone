#include <linphonecore.h>
#include <mediastreamer2/mediastream.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <poll.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <list>
#include <map>
#include <queue>

using namespace std;

class Daemon;

class DaemonCommand {
public:
	virtual void exec(Daemon *app, const char *args)=0;
	bool matches(const char *name) const;
	const std::string &getProto() const {
		return mProto;
	}
	const std::string &getHelp() const {
		return mHelp;
	}
protected:
	DaemonCommand(const char *name, const char *proto, const char *help);
	const std::string mName;
	const std::string mProto;
	const std::string mHelp;
};

class Response {
public:
	enum Status {
		Ok, Error
	};
	virtual ~Response() {
	}
	Response() :
			mStatus(Ok) {
	}
	Response(const char *msg, Status status = Error) :
			mStatus(status) {
		if (status == Ok) {
			mBody = msg;
		} else {
			mReason = msg;
		}
	}
	void setStatus(Status st) {
		mStatus = st;
	}
	void setReason(const char *reason) {
		mReason = reason;
	}
	void setBody(const char *body) {
		mBody = body;
	}
	const std::string &getBody() const {
		return mBody;
	}
	virtual int toBuf(char *dst, int dstlen) const {
		int i = 0;
		i += snprintf(dst + i, dstlen - i, "Status: %s\n", mStatus == Ok ? "Ok" : "Error");
		if (mReason.size() > 0) {
			i += snprintf(dst + i, dstlen - i, "Reason: %s\n", mReason.c_str());
		}
		if (mBody.size() > 0) {
			i += snprintf(dst + i, dstlen - i, "\n%s\n", mBody.c_str());
		}
		return i;
	}
private:
	Status mStatus;
	string mReason;
	string mBody;
};

class EventResponse: public Response {
public:
	EventResponse(LinphoneCall *call, LinphoneCallState state);
private:
};

class PayloadTypeResponse: public Response {
public:
	PayloadTypeResponse(LinphoneCore *core, PayloadType *payloadType, int index);
private:
};

class Daemon {
	friend class DaemonCommand;
public:
	typedef Response::Status Status;
	Daemon(const char *config_path, bool using_pipes, bool display_video, bool capture_video);
	~Daemon();
	int run();
	void quit();
	void sendResponse(const Response &resp);
	LinphoneCore *getCore();
	const list<DaemonCommand*> &getCommandList() const;
	LinphoneCall *findCall(int id);
	LinphoneProxyConfig *findProxy(int id);
	AudioStream *findAudioStream(int id);
	bool pullEvent();
	static int getCallId(LinphoneCall *call);
	int setCallId(LinphoneCall *call);
	static int getProxyId(LinphoneProxyConfig *proxy);
	int setProxyId(LinphoneProxyConfig *proxy);
	int setAudioStreamId(AudioStream *audio_stream);
private:

	static void callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg);
	static int readlineHook();
	void callStateChanged(LinphoneCall *call, LinphoneCallState state, const char *msg);
	void execCommand(const char *cl);
	void initReadline();
	char *readPipe(char *buffer, int buflen);
	void iterate();
	void initCommands();
	void uninitCommands();
	LinphoneCore *mLc;
	std::list<DaemonCommand*> mCommands;
	queue<EventResponse*> mEventQueue;
	int mServerFd;
	int mChildFd;
	string mHistfile;
	bool mRunning;
	static Daemon *sZis;
	static int sCallIds;
	static int sProxyIds;
	static int sAudioStreamIds;
	static const int sLineSize = 512;
	std::map<int, AudioStream*> mAudioStreams;
};

class CallCommand: public DaemonCommand {
public:
	CallCommand() :
			DaemonCommand("call", "call <sip address>", "Place a call.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneCall *call;
		call = linphone_core_invite(app->getCore(), args);
		if (call == NULL) {
			app->sendResponse(Response("Call creation failed."));
		} else {
			Response resp;
			ostringstream ostr;
			ostr << "Id: " << app->setCallId(call) << "\n";
			resp.setBody(ostr.str().c_str());
			app->sendResponse(resp);
		}
	}
};

class TerminateCommand: public DaemonCommand {
public:
	TerminateCommand() :
			DaemonCommand("terminate", "terminate <call id>", "Terminate a call.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneCall *call = NULL;
		int cid;
		const MSList *elem;
		if (sscanf(args, "%i", &cid) == 1) {
			call = app->findCall(cid);
			if (call == NULL) {
				app->sendResponse(Response("No call with such id."));
				return;
			}
		} else {
			elem = linphone_core_get_calls(app->getCore());
			if (elem != NULL && elem->next == NULL) {
				call = (LinphoneCall*) elem->data;
			}
		}
		if (call == NULL) {
			app->sendResponse(Response("No active call."));
			return;
		}
		linphone_core_terminate_call(app->getCore(), call);
		app->sendResponse(Response());
	}
};

class QuitCommand: public DaemonCommand {
public:
	QuitCommand() :
			DaemonCommand("quit", "quit", "Quit the application.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		app->quit();
		app->sendResponse(Response());
	}
};

class HelpCommand: public DaemonCommand {
public:
	HelpCommand() :
			DaemonCommand("help", "help", "Show available commands.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		char str[4096] = { 0 };
		int written = 0;
		list<DaemonCommand*>::const_iterator it;
		const list<DaemonCommand*> &l = app->getCommandList();
		for (it = l.begin(); it != l.end(); ++it) {
			written += snprintf(str + written, sizeof(str) - written, "%s\t%s\n", (*it)->getProto().c_str(), (*it)->getHelp().c_str());
		}
		Response resp;
		resp.setBody(str);
		app->sendResponse(resp);
	}
};

class RegisterCommand: public DaemonCommand {
public:
	RegisterCommand() :
			DaemonCommand("register", "register <identity> <proxy-address> <password>", "Register the daemon to a SIP proxy.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneCore *lc = app->getCore();
		char proxy[256] = { 0 }, identity[128] = { 0 }, password[64] = { 0 };
		if (sscanf(args, "%255s %127s %63s", identity, proxy, password) >= 2) {
			LinphoneProxyConfig *cfg = linphone_proxy_config_new();
			if (password[0] != '\0') {
				LinphoneAddress *from = linphone_address_new(identity);
				if (from != NULL) {
					LinphoneAuthInfo *info = linphone_auth_info_new(linphone_address_get_username(from), NULL, password, NULL, NULL); /*create authentication structure from identity*/
					linphone_core_add_auth_info(lc, info); /*add authentication info to LinphoneCore*/
					linphone_address_destroy(from);
					linphone_auth_info_destroy(info);
				}
			}
			linphone_proxy_config_set_identity(cfg, identity);
			linphone_proxy_config_set_server_addr(cfg, proxy);
			linphone_proxy_config_enable_register(cfg, TRUE);
			app->setProxyId(cfg);
			ostringstream ostr;
			ostr << "Id: " << Daemon::getProxyId(cfg) << "\n";
			linphone_core_add_proxy_config(lc, cfg);
			app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class UnregisterCommand: public DaemonCommand {
public:
	UnregisterCommand() :
			DaemonCommand("unregister", "unregister <register_id>", "Unregister the daemon from proxy.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneCore *lc = app->getCore();
		LinphoneProxyConfig *cfg = NULL;
		int pid;
		if (sscanf(args, "%i", &pid) == 1) {
			cfg = app->findProxy(pid);
			if (cfg == NULL) {
				app->sendResponse(Response("No register with such id."));
				return;
			}
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
			return;
		}
		linphone_core_remove_proxy_config(lc, cfg);
		app->sendResponse(Response());
	}
};

class PopEventCommand: public DaemonCommand {
public:
	PopEventCommand() :
			DaemonCommand("pop-event", "pop-event", "Pop an event from event queue and display it.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		app->pullEvent();
	}
};

class AudioStreamStartCommand: public DaemonCommand {
public:
	AudioStreamStartCommand() :
			DaemonCommand("audio-stream-start", "audio-stream-start <remote ip> <remote port> <payload type number>", "Start an audio stream.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		char addr[256];
		int port;
		int payload_type;
		if (sscanf(args, "%255s %d %d", addr, &port, &payload_type) == 3) {
			int local_port = linphone_core_get_audio_port(app->getCore());
			const int jitt = linphone_core_get_audio_jittcomp(app->getCore());
			const bool_t echo_canceller = linphone_core_echo_cancellation_enabled(app->getCore());
			MSSndCardManager *manager = ms_snd_card_manager_get();
			MSSndCard *capture_card = ms_snd_card_manager_get_card(manager, linphone_core_get_capture_device(app->getCore()));
			MSSndCard *play_card = ms_snd_card_manager_get_card(manager, linphone_core_get_playback_device(app->getCore()));
			AudioStream *stream = audio_stream_new(local_port, false);
			if (audio_stream_start_now(stream, &av_profile, addr, port, port + 1, payload_type, jitt, play_card, capture_card, echo_canceller) != 0) {
				app->sendResponse(Response("Error during audio stream creation."));
			}
			ostringstream ostr;
			ostr << "Id: " << app->setAudioStreamId(stream) << "\n";
			app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class AudioStreamStopCommand: public DaemonCommand {
public:
	AudioStreamStopCommand() :
			DaemonCommand("audio-stream-stop", "audio-stream-stop <audio stream id>", "Stop an audio stream.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		int id;
		if (sscanf(args, "%d", &id) == 1) {
			AudioStream *stream = app->findAudioStream(id);
			if (stream == NULL) {
				app->sendResponse(Response("No Audio Stream with such id."));
				return;
			}
			audio_stream_stop(stream);
			app->sendResponse(Response());
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class RegisterStatusCommand: public DaemonCommand {
public:
	RegisterStatusCommand() :
			DaemonCommand("register-status", "register-status <register id>", "Return status of a register.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneProxyConfig *cfg = NULL;
		int pid;
		if (sscanf(args, "%i", &pid) == 1) {
			cfg = app->findProxy(pid);
			if (cfg == NULL) {
				app->sendResponse(Response("No register with such id."));
				return;
			}
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
			return;
		}

		app->sendResponse(Response(linphone_registration_state_to_string(linphone_proxy_config_get_state(cfg)), Response::Ok));
	}
};

class CallStatusCommand: public DaemonCommand {
public:
	CallStatusCommand() :
			DaemonCommand("call-status", "call-status <call id>", "Return status of a call.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneCore *lc = app->getCore();
		int cid;
		LinphoneCall *call = NULL;
		if (sscanf(args, "%i", &cid) == 1) {
			call = app->findCall(cid);
			if (call == NULL) {
				app->sendResponse(Response("No call with such id."));
				return;
			}
		} else {
			call = linphone_core_get_current_call(lc);
			if (call == NULL) {
				app->sendResponse(Response("No current call available."));
				return;
			}
		}

		LinphoneCallState call_state = LinphoneCallIdle;
		call_state = linphone_call_get_state(call);
		const LinphoneAddress *remoteAddress = linphone_call_get_remote_address(call);
		ostringstream ostr;
		ostr << linphone_call_state_to_string(call_state) << "\n";

		switch (call_state) {
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallPaused:
		case LinphoneCallStreamsRunning:
		case LinphoneCallConnected:
		case LinphoneCallIncomingReceived:
			ostr << "From: " << linphone_address_as_string(remoteAddress) << "\n";
			break;
		default:
			break;
		}
		switch (call_state) {
		case LinphoneCallStreamsRunning:
		case LinphoneCallConnected:
			ostr << "Direction: " << ((linphone_call_get_dir(call) == LinphoneCallOutgoing) ? "out" : "in") << "\n";
			ostr << "Duration: " << linphone_call_get_duration(call) << "\n";
		default:
			break;
		}
		app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
	}
};

class PtimeCommand: public DaemonCommand {
public:
	PtimeCommand() :
			DaemonCommand("ptime", "ptime <ms>", "Set the if ms is defined, otherwise return the ptime.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		int ms;
		int ret = sscanf(args, "%d", &ms);
		if (ret <= 0) {
			ostringstream ostr;
			ms = linphone_core_get_upload_ptime(app->getCore());
			ostr << "Ptime: " << ms << "\n";
			app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		} else if (ret == 1) {
			ostringstream ostr;
			linphone_core_set_upload_ptime(app->getCore(), ms);
			ms = linphone_core_get_upload_ptime(app->getCore());
			ostr << "Ptime: " << ms << "\n";
			app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class AudioCodecGetCommand: public DaemonCommand {
public:
	AudioCodecGetCommand() :
			DaemonCommand("audio-codec-get", "audio-codec-get <payload type number>", "Get an audio codec if codec-mime is defined, otherwise return the audio codec list.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		int payload_type;
		bool list = sscanf(args, "%d", &payload_type) != 1;
		bool find = list;
		int index = 0;
		ostringstream ostr;
		for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
			PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
			if (list) {
				ostr << PayloadTypeResponse(app->getCore(), payload, index).getBody() << "\n";
			} else if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
				ostr << PayloadTypeResponse(app->getCore(), payload, index).getBody();
				find = true;
				break;
			}
			++index;
		}

		if (!find) {
			app->sendResponse(Response("Audio codec not found."));
		} else {
			app->sendResponse(Response(ostr.str().c_str(), Response::Ok));
		}
	}
};

class AudioCodecMoveCommand: public DaemonCommand {
public:
	AudioCodecMoveCommand() :
			DaemonCommand("audio-codec-move", "audio-codec-move <payload type number> <index>", "Move a codec to the an index.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		int payload_type;
		int index;
		if (sscanf(args, "%d %d", &payload_type, &index) == 2 && index >= 0) {
			PayloadType *selected_payload = NULL;
			for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
				PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
				if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
					selected_payload = payload;
					break;
				}
			}
			if (selected_payload == NULL) {
				app->sendResponse(Response("Audio codec not found."));
				return;
			}
			int i = 0;
			MSList *mslist = NULL;
			for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
				PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
				if (i == index) {
					mslist = ms_list_append(mslist, selected_payload);
					++i;
				}
				if (selected_payload != payload) {
					mslist = ms_list_append(mslist, payload);
					++i;
				}
			}
			if (i <= index) {
				index = i;
				mslist = ms_list_append(mslist, selected_payload);
			}
			linphone_core_set_audio_codecs(app->getCore(), mslist);

			app->sendResponse(PayloadTypeResponse(app->getCore(), selected_payload, index));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class AudioCodecEnableCommand: public DaemonCommand {
public:
	AudioCodecEnableCommand() :
			DaemonCommand("audio-codec-enable", "audio-codec-enable <payload type number>", "Enable an audio codec.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		int payload_type;
		if (sscanf(args, "%d", &payload_type) == 1) {
			int index = 0;
			for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
				PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
				if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
					linphone_core_enable_payload_type(app->getCore(), payload, true);
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
					return;
				}
				++index;
			}
			app->sendResponse(Response("Audio codec not found."));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class AudioCodecDisableCommand: public DaemonCommand {
public:
	AudioCodecDisableCommand() :
			DaemonCommand("audio-codec-disable", "audio-codec-disable <codec-mime>", "Disable an audio codec.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		int payload_type;
		if (sscanf(args, "%d", &payload_type) == 1) {
			int index = 0;
			for (const MSList *node = linphone_core_get_audio_codecs(app->getCore()); node != NULL; node = ms_list_next(node)) {
				PayloadType *payload = reinterpret_cast<PayloadType*>(node->data);
				if (payload_type == linphone_core_get_payload_type_number(app->getCore(), payload)) {
					linphone_core_enable_payload_type(app->getCore(), payload, false);
					app->sendResponse(PayloadTypeResponse(app->getCore(), payload, index));
					return;
				}
				++index;
			}
			app->sendResponse(Response("Audio codec not found."));
		} else {
			app->sendResponse(Response("Missing/Incorrect parameter(s)."));
		}
	}
};

class AnswerCommand: public DaemonCommand {
public:
	AnswerCommand() :
			DaemonCommand("answer", "answer <call id>", "Answer an incoming call.") {
	}
	virtual void exec(Daemon *app, const char *args) {
		LinphoneCore *lc = app->getCore();
		int cid;
		LinphoneCall *call;
		if (sscanf(args, "%i", &cid) == 1) {
			call = app->findCall(cid);
			if (call == NULL) {
				app->sendResponse(Response("No call with such id."));
				return;
			} else {
				LinphoneCallState cstate = linphone_call_get_state(call);
				if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallIncomingEarlyMedia) {
					if (linphone_core_accept_call(lc, call) == 0) {
						app->sendResponse(Response());
						return;
					}
				}
				app->sendResponse(Response("Can't accept this call."));
				return;
			}
		} else {
			for (const MSList* elem = linphone_core_get_calls(lc); elem != NULL; elem = elem->next) {
				call = (LinphoneCall*) elem->data;
				LinphoneCallState cstate = linphone_call_get_state(call);
				if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallIncomingEarlyMedia) {
					if (linphone_core_accept_call(lc, call) == 0) {
						app->sendResponse(Response());
						return;
					}
				}
			}
		}
		app->sendResponse(Response("No call to accept."));
	}
};

EventResponse::EventResponse(LinphoneCall *call, LinphoneCallState state) {
	ostringstream ostr;
	char *remote = linphone_call_get_remote_address_as_string(call);
	ostr << "Event-type: call-state-changed\nEvent: " << linphone_call_state_to_string(state) << "\n";
	ostr << "From: " << remote << "\n";
	ostr << "Id: " << Daemon::getCallId(call) << "\n";
	setBody(ostr.str().c_str());
	ms_free(remote);
}

PayloadTypeResponse::PayloadTypeResponse(LinphoneCore *core, PayloadType *payloadType, int index) {
	ostringstream ostr;
	ostr << "Index: " << index << "\n";
	ostr << "Payload-type-number: " << linphone_core_get_payload_type_number(core, payloadType) << "\n";
	ostr << "Clock-rate: " << payloadType->clock_rate << "\n";
	ostr << "Bitrate: " << payloadType->normal_bitrate << "\n";
	ostr << "Mime: " << payloadType->mime_type << "\n";
	ostr << "Channels: " << payloadType->channels << "\n";
	ostr << "Recv-fmtp: " << ((payloadType->recv_fmtp) ? payloadType->recv_fmtp : "") << "\n";
	ostr << "Send-fmtp: " << ((payloadType->send_fmtp) ? payloadType->send_fmtp : "") << "\n";
	ostr << "Enabled: " << (linphone_core_payload_type_enabled(core, payloadType) == TRUE ? "true" : "false") << "\n";
	setBody(ostr.str().c_str());
}

DaemonCommand::DaemonCommand(const char *name, const char *proto, const char *help) :
		mName(name), mProto(proto), mHelp(help) {
}

bool DaemonCommand::matches(const char *name) const {
	return strcmp(name, mName.c_str()) == 0;
}

Daemon * Daemon::sZis = NULL;
int Daemon::sCallIds = 0;
int Daemon::sProxyIds = 0;
int Daemon::sAudioStreamIds = 0;

Daemon::Daemon(const char *config_path, bool using_pipes, bool display_video, bool capture_video) {
	sZis = this;
	mServerFd = -1;
	mChildFd = -1;
	if (!using_pipes) {
		initReadline();
	} else {
		mServerFd = ortp_server_pipe_create("linphone-daemon");
		listen(mServerFd, 2);
		fprintf(stdout, "Server unix socket created, fd=%i\n", mServerFd);
	}

	linphone_core_disable_logs();

	LinphoneCoreVTable vtable = { 0 };
	vtable.call_state_changed = callStateChanged;
	mLc = linphone_core_new(&vtable, NULL, config_path, this);
	linphone_core_enable_video(mLc, display_video, capture_video);
	linphone_core_enable_echo_cancellation(mLc, false);
	initCommands();
}

const list<DaemonCommand*> &Daemon::getCommandList() const {
	return mCommands;
}

LinphoneCore *Daemon::getCore() {
	return mLc;
}

int Daemon::getCallId(LinphoneCall *call) {
	return (int) (long) linphone_call_get_user_pointer(call);
}

int Daemon::setCallId(LinphoneCall *call) {
	linphone_call_set_user_pointer(call, (void*) (long) ++sCallIds);
	return sCallIds;
}

LinphoneCall *Daemon::findCall(int id) {
	const MSList *elem = linphone_core_get_calls(mLc);
	for (; elem != NULL; elem = elem->next) {
		LinphoneCall *call = (LinphoneCall *) elem->data;
		if (linphone_call_get_user_pointer(call) == (void*) (long) id)
			return call;
	}
	return NULL;
}

int Daemon::getProxyId(LinphoneProxyConfig *proxy) {
	return (int) (long) linphone_proxy_config_get_user_data(proxy);
}

int Daemon::setProxyId(LinphoneProxyConfig *cfg) {
	linphone_proxy_config_set_user_data(cfg, (void*) (long) ++sProxyIds);
	return sProxyIds;
}

LinphoneProxyConfig *Daemon::findProxy(int id) {
	const MSList *elem = linphone_core_get_proxy_config_list(mLc);
	for (; elem != NULL; elem = elem->next) {
		LinphoneProxyConfig *proxy = (LinphoneProxyConfig *) elem->data;
		if (linphone_proxy_config_get_user_data(proxy) == (void*) (long) id)
			return proxy;
	}
	return NULL;
}

int Daemon::setAudioStreamId(AudioStream *audio_stream) {
	++sProxyIds;
	mAudioStreams.insert(std::pair<int, AudioStream*>(sProxyIds, audio_stream));
	return sProxyIds;
}

AudioStream *Daemon::findAudioStream(int id) {
	std::map<int, AudioStream*>::iterator it = mAudioStreams.find(id);
	if (it != mAudioStreams.end())
		return it->second;
	return NULL;
}

void Daemon::initCommands() {
	mCommands.push_back(new RegisterCommand());
	mCommands.push_back(new RegisterStatusCommand());
	mCommands.push_back(new UnregisterCommand());
	mCommands.push_back(new CallCommand());
	mCommands.push_back(new TerminateCommand());
	mCommands.push_back(new PopEventCommand());
	mCommands.push_back(new AnswerCommand());
	mCommands.push_back(new CallStatusCommand());
	mCommands.push_back(new AudioCodecGetCommand());
	mCommands.push_back(new AudioCodecEnableCommand());
	mCommands.push_back(new AudioCodecDisableCommand());
	mCommands.push_back(new AudioCodecMoveCommand());
	mCommands.push_back(new AudioStreamStartCommand());
	mCommands.push_back(new AudioStreamStopCommand());
	mCommands.push_back(new PtimeCommand());
	mCommands.push_back(new QuitCommand());
	mCommands.push_back(new HelpCommand());

}

void Daemon::uninitCommands() {
	while (!mCommands.empty()) {
		delete mCommands.front();
		mCommands.pop_front();
	}
}

bool Daemon::pullEvent() {
	bool status = false;
	ostringstream ostr;
	if (!mEventQueue.empty()) {
		Response *r = mEventQueue.front();
		mEventQueue.pop();
		ostr << r->getBody() << "\n";
		delete r;
		status = true;
	}
	ostr << "Size: " << mEventQueue.size() << "\n";
	sendResponse(Response(ostr.str().c_str(), Response::Ok));
	return status;
}

void Daemon::callStateChanged(LinphoneCall *call, LinphoneCallState state, const char *msg) {
	switch (state) {
	case LinphoneCallOutgoingProgress:
	case LinphoneCallIncomingReceived:
	case LinphoneCallIncomingEarlyMedia:
	case LinphoneCallConnected:
	case LinphoneCallStreamsRunning:
	case LinphoneCallError:
	case LinphoneCallEnd:
		mEventQueue.push(new EventResponse(call, state));
		break;
	default:
		break;
	}
}

void Daemon::callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg) {
	Daemon *app = (Daemon*) linphone_core_get_user_data(lc);
	app->callStateChanged(call, state, msg);
}

void Daemon::iterate() {
	linphone_core_iterate(mLc);
	if (mChildFd == -1) {
		if (!mEventQueue.empty()) {
			Response *r = mEventQueue.front();
			mEventQueue.pop();
			fprintf(stdout, "%s\n", r->getBody().c_str());
			fflush(stdout);
			delete r;
		}
	}
}

int Daemon::readlineHook() {
	sZis->iterate();
	return 0;
}

void Daemon::initReadline() {
	const char *homedir = getenv("HOME");
	rl_readline_name = "daemon";

	rl_set_keyboard_input_timeout(20000);
	rl_event_hook = readlineHook;

	if (homedir == NULL)
		homedir = ".";
	mHistfile = string(homedir) + string("/.linphone_history");
	read_history(mHistfile.c_str());
	setlinebuf(stdout);
}

void Daemon::execCommand(const char *cl) {
	char args[sLineSize] = { 0 };
	char name[sLineSize] = { 0 };
	sscanf(cl, "%511s %511[^\n]", name, args); //Read the rest of line in args
	list<DaemonCommand*>::iterator it = find_if(mCommands.begin(), mCommands.end(), bind2nd(mem_fun(&DaemonCommand::matches), name));
	if (it != mCommands.end()) {
		(*it)->exec(this, args);
	} else {
		sendResponse(Response("Unknown command."));
	}
}

void Daemon::sendResponse(const Response &resp) {
	char buf[4096] = { 0 };
	int size;
	size = resp.toBuf(buf, sizeof(buf));
	if (mChildFd != -1) {
		if (write(mChildFd, buf, size) == -1) {
			ms_error("Fail to write to pipe: %s", strerror(errno));
		}
	} else {
		fprintf(stdout, "%s", buf);
		fflush(stdout);
	}
}

char *Daemon::readPipe(char *buffer, int buflen) {
	struct pollfd pfd[2] = { { 0 }, { 0 } };
	int nfds = 1;
	if (mServerFd != -1) {
		pfd[0].events = POLLIN;
		pfd[0].fd = mServerFd;
	}
	if (mChildFd != -1) {
		pfd[1].events = POLLIN;
		pfd[1].fd = mChildFd;
		nfds++;
	}
	iterate();
	int err = poll(pfd, nfds, 50);
	if (err > 0) {
		if (mServerFd != -1 && (pfd[0].revents & POLLIN)) {
			struct sockaddr_storage addr;
			socklen_t addrlen = sizeof(addr);
			int childfd = accept(mServerFd, (struct sockaddr*) &addr, &addrlen);
			if (childfd != -1) {
				if (mChildFd != -1) {
					ms_error("Cannot accept two client at the same time");
					close(childfd);
				} else {
					mChildFd = childfd;
					return NULL;
				}
			}
		}
		if (mChildFd != -1 && (pfd[1].revents & POLLIN)) {
			int ret;
			if ((ret = read(mChildFd, buffer, buflen)) == -1) {
				ms_error("Fail to read from pipe: %s", strerror(errno));
			} else {
				if (ret == 0) {
					ms_message("Client disconnected");
					close(mChildFd);
					mChildFd = -1;
					return NULL;
				}
				buffer[ret] = 0;
				return buffer;
			}
		}
	}
	return NULL;
}

static void printHelp() {
	fprintf(stdout, "daemon-linphone [<options>]\n"
			"where options are :\n"
			"\t--help\t\tPrint this notice.\n"
			"\t--pipe\t\tCreate an unix server socket to receive commands.\n"
			"\t--config <path>\tSupply a linphonerc style config file to start with.\n"
			"\t-C\t\tenable video capture.\n"
			"\t-D\t\tenable video display.\n");
}

int Daemon::run() {
	char line[sLineSize] = "daemon-linphone>";
	char *ret;
	mRunning = true;
	while (mRunning) {
		if (mServerFd == -1) {
			ret = readline(line);
			if (ret && ret[0] != '\0') {
				add_history(ret);
			}
		} else {
			ret = readPipe(line, sLineSize);
		}
		if (ret && ret[0] != '\0') {
			execCommand(ret);
		}
		if (mServerFd == -1) {
			free(ret);
		}
	}
	return 0;
}

void Daemon::quit() {
	mRunning = false;
}

Daemon::~Daemon() {
	uninitCommands();

	for (std::map<int, AudioStream *>::iterator it = mAudioStreams.begin(); it != mAudioStreams.end();) {
		audio_stream_stop(it->second);
	}

	linphone_core_destroy(mLc);
	if (mChildFd != -1) {
		close(mChildFd);
	}
	if (mServerFd != -1) {
		ortp_server_pipe_close(mServerFd);
	}
	stifle_history(30);
	write_history(mHistfile.c_str());
}

int main(int argc, char *argv[]) {
	const char *config_path = NULL;
	bool using_pipes = false;
	bool capture_video = false;
	bool display_video = false;
	int i;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0) {
			printHelp();
			return 0;
		} else if (strcmp(argv[i], "--pipe") == 0) {
			using_pipes = true;
		} else if (strcmp(argv[i], "--config") == 0) {
			config_path = argv[i + 1];
		} else if (strcmp(argv[i], "-C") == 0) {
			capture_video = true;
		} else if (strcmp(argv[i], "-D") == 0) {
			display_video = true;
		}
	}
	Daemon app(config_path, using_pipes, display_video, capture_video);
	return app.run();
}
;

