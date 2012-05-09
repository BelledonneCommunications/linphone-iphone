#include <iostream>
#include <sstream>
#include <algorithm>

#include <readline/readline.h>
#include <readline/history.h>

#include <poll.h>

#include "daemon.h"
#include "commands/answer.h"
#include "commands/audio-codec-get.h"
#include "commands/audio-codec-move.h"
#include "commands/audio-codec-enable.h"
#include "commands/audio-codec-disable.h"
#include "commands/audio-codec-set.h"
#include "commands/audio-stream-start.h"
#include "commands/audio-stream-stop.h"
#include "commands/call.h"
#include "commands/call-stats.h"
#include "commands/call-status.h"
#include "commands/help.h"
#include "commands/pop-event.h"
#include "commands/ptime.h"
#include "commands/register.h"
#include "commands/register-status.h"
#include "commands/terminate.h"
#include "commands/unregister.h"
#include "commands/quit.h"

using namespace std;

EventResponse::EventResponse(LinphoneCall *call, LinphoneCallState state) {
	ostringstream ostr;
	char *remote = linphone_call_get_remote_address_as_string(call);
	ostr << "Event-type: call-state-changed\nEvent: " << linphone_call_state_to_string(state) << "\n";
	ostr << "From: " << remote << "\n";
	ostr << "Id: " << Daemon::getCallId(call) << "\n";
	setBody(ostr.str().c_str());
	ms_free(remote);
}

DtmfResponse::DtmfResponse(LinphoneCall *call, int dtmf) {
	ostringstream ostr;
	char *remote = linphone_call_get_remote_address_as_string(call);
	ostr << "Event-type: receiving-tone\nTone: " << (char) dtmf << "\n";
	ostr << "From: " << remote << "\n";
	ostr << "Id: " << Daemon::getCallId(call) << "\n";
	setBody(ostr.str().c_str());
	ms_free(remote);
}

PayloadTypeResponse::PayloadTypeResponse(LinphoneCore *core, const PayloadType *payloadType, int index, const string &prefix, bool enabled_status) {
	ostringstream ostr;
	if (payloadType != NULL) {
		if (index >= 0)
			ostr << prefix << "Index: " << index << "\n";
		ostr << prefix << "Payload-type-number: " << linphone_core_get_payload_type_number(core, payloadType) << "\n";
		ostr << prefix << "Clock-rate: " << payloadType->clock_rate << "\n";
		ostr << prefix << "Bitrate: " << payloadType->normal_bitrate << "\n";
		ostr << prefix << "Mime: " << payloadType->mime_type << "\n";
		ostr << prefix << "Channels: " << payloadType->channels << "\n";
		ostr << prefix << "Recv-fmtp: " << ((payloadType->recv_fmtp) ? payloadType->recv_fmtp : "") << "\n";
		ostr << prefix << "Send-fmtp: " << ((payloadType->send_fmtp) ? payloadType->send_fmtp : "") << "\n";
		if (enabled_status)
			ostr << prefix << "Enabled: " << (linphone_core_payload_type_enabled(core, payloadType) == TRUE ? "true" : "false") << "\n";
		setBody(ostr.str().c_str());
	}
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

Daemon::Daemon(const char *config_path, const char *factory_config_path, const char *log_file, const char *pipe_name, bool display_video, bool capture_video) :
		mLogFile(NULL) {
	sZis = this;
	mServerFd = -1;
	mChildFd = -1;
	if (pipe_name == NULL) {
		initReadline();
	} else {
		mServerFd = ortp_server_pipe_create(pipe_name);
		listen(mServerFd, 2);
		fprintf(stdout, "Server unix socket created, name=%s fd=%i\n", pipe_name, mServerFd);
	}

	if (log_file != NULL) {
		mLogFile = fopen(log_file, "a+");
		linphone_core_enable_logs(mLogFile);
	} else {
		linphone_core_disable_logs();
	}

	LinphoneCoreVTable vtable = { 0 };
	vtable.call_state_changed = callStateChanged;
	vtable.dtmf_received = dtmfReceived;
	mLc = linphone_core_new(&vtable, config_path, factory_config_path, this);
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
	mCommands.push_back(new CallStatsCommand());
	mCommands.push_back(new AudioCodecGetCommand());
	mCommands.push_back(new AudioCodecEnableCommand());
	mCommands.push_back(new AudioCodecDisableCommand());
	mCommands.push_back(new AudioCodecMoveCommand());
	mCommands.push_back(new AudioCodecSetCommand());
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

void Daemon::dtmfReceived(LinphoneCall *call, int dtmf) {
	mEventQueue.push(new DtmfResponse(call, dtmf));
}

void Daemon::callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg) {
	Daemon *app = (Daemon*) linphone_core_get_user_data(lc);
	app->callStateChanged(call, state, msg);
}
void Daemon::dtmfReceived(LinphoneCore *lc, LinphoneCall *call, int dtmf) {
	Daemon *app = (Daemon*) linphone_core_get_user_data(lc);
	app->dtmfReceived(call, dtmf);
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
			"\t--help\t\t\tPrint this notice.\n"
			"\t--pipe <pipename>\tCreate an unix server socket to receive commands.\n"
			"\t--log <path>\t\tSupply a file where the log will be saved\n"
			"\t--factory-config <path>\tSupply a readonly linphonerc style config file to start with.\n"
			"\t--config <path>\t\tSupply a linphonerc style config file to start with.\n"
			"\t-C\t\t\tenable video capture.\n"
			"\t-D\t\t\tenable video display.\n");
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
	if (mLogFile != NULL) {
		linphone_core_enable_logs(NULL);
		fclose(mLogFile);
	}

	stifle_history(30);
	write_history(mHistfile.c_str());
}

int main(int argc, char *argv[]) {
	const char *config_path = NULL;
	const char *factory_config_path = NULL;
	const char *pipe_name = NULL;
	const char *log_file = NULL;
	bool capture_video = false;
	bool display_video = false;
	int i;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0) {
			printHelp();
			return 0;
		} else if (strcmp(argv[i], "--pipe") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no pipe name specify after --pipe");
				return -1;
			}
			pipe_name = argv[++i];
		} else if (strcmp(argv[i], "--factory-config") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no file specify after --factory-config");
				return -1;
			}
			factory_config_path = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "--config") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no file specify after  --config");
				return -1;
			}
			config_path = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "--log") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no file specify after --log");
				return -1;
			}
			log_file = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "-C") == 0) {
			capture_video = true;
		} else if (strcmp(argv[i], "-D") == 0) {
			display_video = true;
		}
	}
	Daemon app(config_path, factory_config_path, log_file, pipe_name, display_video, capture_video);
	return app.run();
}
;

