#ifndef DAEMON_H_
#define DAEMON_H_

#include <linphonecore.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/mscommon.h>

#include <string>
#include <list>
#include <queue>
#include <map>
#include <sstream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_READLINE
#ifdef HAVE_READLINE_H
#include <readline.h>
#else
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#endif
#endif
#ifdef HAVE_HISTORY_H
#include <history.h>
#else
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#endif


class Daemon;

class DaemonCommandExample {
public:
	DaemonCommandExample(const char *command, const char *output);
	~DaemonCommandExample() {};
	const std::string &getCommand() const {
		return mCommand;
	}
	const std::string &getOutput() const {
		return mOutput;
	}
private:
	const std::string mCommand;
	const std::string mOutput;
};

class DaemonCommand {
public:
	virtual ~DaemonCommand() {

	};
	virtual void exec(Daemon *app, const char *args)=0;
	bool matches(const char *name) const;
	const std::string getHelp() const;
	const std::string &getProto() const {
		return mProto;
	}
	const std::string &getDescription() const {
		return mDescription;
	}
	const std::list<const DaemonCommandExample*> &getExamples() const {
		return mExamples;
	}
	void addExample(const DaemonCommandExample *example);
protected:
	DaemonCommand(const char *name, const char *proto, const char *description);
	const std::string mName;
	const std::string mProto;
	const std::string mDescription;
	std::list<const DaemonCommandExample*> mExamples;
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

class PayloadTypeParser {
public:
	PayloadTypeParser(LinphoneCore *core, const std::string &mime_type, bool accept_all = false);
	inline bool all() { return mAll; };
	inline bool successful() { return mSuccesful; };
	inline int payloadTypeNumber() { return mPayloadTypeNumber; };
private:
	bool mAll;
	bool mSuccesful;
	int mPayloadTypeNumber;
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
	void removeAudioStream(int id);
	bool pullEvent();
	int updateCallId(LinphoneCall *call);
	int updateProxyId(LinphoneProxyConfig *proxy);
	inline int maxProxyId() { return mProxyIds; };
	int updateAudioStreamId(AudioStream *audio_stream);
	void dumpCommandsHelp();
	void dumpCommandsHelpHtml();
private:
	static void* iterateThread(void *arg);
	static void callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg);
	static void callStatsUpdated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);
	static void dtmfReceived(LinphoneCore *lc, LinphoneCall *call, int dtmf);
	void callStateChanged(LinphoneCall *call, LinphoneCallState state, const char *msg);
	void callStatsUpdated(LinphoneCall *call, const LinphoneCallStats *stats);
	void dtmfReceived(LinphoneCall *call, int dtmf);
	void execCommand(const char *cl);
	char *readLine(const char *);
	char *readPipe(char *buffer, int buflen);
	void iterate();
	void startThread();
	void stopThread();
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
	int mCallIds;
	int mProxyIds;
	int mAudioStreamIds;
	ms_thread_t mThread;
	ms_mutex_t mMutex;
	static const int sLineSize = 512;
	std::map<int, AudioStream*> mAudioStreams;
};

#endif //DAEMON_H_
