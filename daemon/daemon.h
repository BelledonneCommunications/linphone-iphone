#ifndef DAEMON_H_
#define DAEMON_H_

#include <linphonecore.h>
#include <mediastreamer2/mediastream.h>

#include <string>
#include <list>
#include <queue>
#include <map>

class Daemon;

class DaemonCommand {
public:
	virtual ~DaemonCommand() {

	};
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
	std::string mReason;
	std::string mBody;
};

class EventResponse: public Response {
public:
	EventResponse(Daemon *daemon, LinphoneCall *call, LinphoneCallState state);
private:
};

class CallStatsResponse: public Response {
public:
	CallStatsResponse(Daemon *daemon, LinphoneCall *call, const LinphoneCallStats *stats, bool unique);
private:
};

class DtmfResponse: public Response {
public:
	DtmfResponse(Daemon *daemon, LinphoneCall *call, int dtmf);
private:
};

class PayloadTypeResponse: public Response {
public:
	PayloadTypeResponse(LinphoneCore *core, const PayloadType *payloadType, int index = -1, const std::string &prefix = std::string(), bool enabled_status = true);
private:
};

class Daemon {
	friend class DaemonCommand;
public:
	typedef Response::Status Status;
	Daemon(const char *config_path, const char *factory_config_path, const char *log_file, const char *pipe_name, bool display_video, bool capture_video);
	~Daemon();
	int run();
	void quit();
	void sendResponse(const Response &resp);
	LinphoneCore *getCore();
	const std::list<DaemonCommand*> &getCommandList() const;
	LinphoneCall *findCall(int id);
	LinphoneProxyConfig *findProxy(int id);
	AudioStream *findAudioStream(int id);
	bool pullEvent();
	int updateCallId(LinphoneCall *call);
	int updateProxyId(LinphoneProxyConfig *proxy);
	int updateAudioStreamId(AudioStream *audio_stream);
private:
	static void callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg);
	static void callStatsUpdated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);
	static void dtmfReceived(LinphoneCore *lc, LinphoneCall *call, int dtmf);
	static int readlineHook();
	void callStateChanged(LinphoneCall *call, LinphoneCallState state, const char *msg);
	void callStatsUpdated(LinphoneCall *call, const LinphoneCallStats *stats);
	void dtmfReceived(LinphoneCall *call, int dtmf);
	void execCommand(const char *cl);
	void initReadline();
	char *readPipe(char *buffer, int buflen);
	void iterate();
	void initCommands();
	void uninitCommands();
	LinphoneCore *mLc;
	std::list<DaemonCommand*> mCommands;
	std::queue<Response*> mEventQueue;
	int mServerFd;
	int mChildFd;
	std::string mHistfile;
	bool mRunning;
	FILE *mLogFile;
	static Daemon *sZis;
	static int sCallIds;
	static int sProxyIds;
	static int sAudioStreamIds;
	static const int sLineSize = 512;
	std::map<int, AudioStream*> mAudioStreams;
};

#endif //DAEMON_H_
