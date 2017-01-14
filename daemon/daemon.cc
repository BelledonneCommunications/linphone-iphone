/*
daemon.cc
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

#include <cstdio>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include <limits>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifndef _WIN32
#include <poll.h>
#endif

#include "daemon.h"
#include "commands/adaptive-jitter-compensation.h"
#include "commands/jitterbuffer.h"
#include "commands/answer.h"
#include "commands/audio-codec-get.h"
#include "commands/audio-codec-move.h"
#include "commands/audio-codec-set.h"
#include "commands/audio-codec-toggle.h"
#include "commands/audio-stream-start.h"
#include "commands/audio-stream-stop.h"
#include "commands/audio-stream-stats.h"
#include "commands/auth-infos-clear.h"
#include "commands/call.h"
#include "commands/call-stats.h"
#include "commands/call-status.h"
#include "commands/call-pause.h"
#include "commands/call-mute.h"
#include "commands/call-resume.h"
#include "commands/video.h"
#include "commands/call-transfer.h"
#include "commands/conference.h"
#include "commands/contact.h"
#include "commands/dtmf.h"
#include "commands/firewall-policy.h"
#include "commands/help.h"
#include "commands/ipv6.h"
#include "commands/media-encryption.h"
#include "commands/msfilter-add-fmtp.h"
#include "commands/play-wav.h"
#include "commands/pop-event.h"
#include "commands/port.h"
#include "commands/ptime.h"
#include "commands/register.h"
#include "commands/register-status.h"
#include "commands/terminate.h"
#include "commands/unregister.h"
#include "commands/quit.h"
#include "commands/configcommand.h"
#include "commands/netsim.h"
#include "commands/cn.h"
#include "commands/version.h"

#include "private.h"
using namespace std;

#ifndef WIN32
#else
#include <windows.h>
void usleep(int waitTime) {
	Sleep(waitTime/1000);
}
#endif

#ifdef HAVE_READLINE
#define LICENCE_GPL
#else
#define LICENCE_COMMERCIAL
#endif

const char * const ice_state_str[] = {
	"Not activated",	/* LinphoneIceStateNotActivated */
	"Failed",	/* LinphoneIceStateFailed */
	"In progress",	/* LinphoneIceStateInProgress */
	"Host connection",	/* LinphoneIceStateHostConnection */
	"Reflexive connection",	/* LinphoneIceStateReflexiveConnection */
	"Relayed connection"	/* LinphoneIceStateRelayConnection */
};

void *Daemon::iterateThread(void *arg) {
	Daemon *daemon = (Daemon *) arg;
	while (daemon->mRunning) {
		ms_mutex_lock(&daemon->mMutex);
		daemon->iterate();
		ms_mutex_unlock(&daemon->mMutex);
		usleep(20000);
	}
	return 0;
}

EventResponse::EventResponse(Daemon *daemon, LinphoneCall *call, LinphoneCallState state) {
	ostringstream ostr;
	char *remote = linphone_call_get_remote_address_as_string(call);
	ostr << "Event-type: call-state-changed\nEvent: " << linphone_call_state_to_string(state) << "\n";
	ostr << "From: " << remote << "\n";
	ostr << "Id: " << daemon->updateCallId(call) << "\n";
	setBody(ostr.str().c_str());
	ms_free(remote);
}

DtmfResponse::DtmfResponse(Daemon *daemon, LinphoneCall *call, int dtmf) {
	ostringstream ostr;
	char *remote = linphone_call_get_remote_address_as_string(call);
	ostr << "Event-type: receiving-tone\nTone: " << (char) dtmf << "\n";
	ostr << "From: " << remote << "\n";
	ostr << "Id: " << daemon->updateCallId(call) << "\n";
	setBody(ostr.str().c_str());
	ms_free(remote);
}

static ostream &printCallStatsHelper(ostream &ostr, const LinphoneCallStats *stats, const string &prefix) {
	ostr << prefix << "ICE state: " << ice_state_str[stats->ice_state] << "\n";
	ostr << prefix << "RoundTripDelay: " << stats->round_trip_delay << "\n";
	ostr << prefix << "Jitter: " << stats->jitter_stats.jitter << "\n";
//	ostr << prefix << "MaxJitter: " << stats->jitter_stats.max_jitter << "\n";
//	ostr << prefix << "SumJitter: " << stats->jitter_stats.sum_jitter << "\n";
//	ostr << prefix << "MaxJitterTs: " << stats->jitter_stats.max_jitter_ts << "\n";
	ostr << prefix << "JitterBufferSizeMs: " << stats->jitter_stats.jitter_buffer_size_ms << "\n";

	const report_block_t *rrb = NULL;
	if (stats->received_rtcp != NULL) {
		if (stats->received_rtcp->b_cont != NULL)
			msgpullup(stats->received_rtcp, -1);
		if (rtcp_is_SR(stats->received_rtcp)) {
			rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
		} else if (rtcp_is_RR(stats->received_rtcp)) {
			rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
		}
	}
	if (rrb) {
		unsigned int ij;
		float flost;
		ij = report_block_get_interarrival_jitter(rrb);
		flost = (float) (100.0 * report_block_get_fraction_lost(rrb) / 256.0);
		ostr << prefix << "Received-InterarrivalJitter: " << ij << "\n";
		ostr << prefix << "Received-FractionLost: " << flost << "\n";
	}

	const report_block_t *srb = NULL;
	if (stats->sent_rtcp != NULL) {
		if (stats->sent_rtcp->b_cont != NULL)
			msgpullup(stats->sent_rtcp, -1);
		if (rtcp_is_SR(stats->sent_rtcp)) {
			srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
		} else if (rtcp_is_RR(stats->sent_rtcp)) {
			srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
		}
	}
	if (srb) {
		unsigned int ij;
		float flost;
		ij = report_block_get_interarrival_jitter(srb);
		flost = (float) (100.0 * report_block_get_fraction_lost(srb) / 256.0);
		ostr << prefix << "Sent-InterarrivalJitter: " << ij << "\n";
		ostr << prefix << "Sent-FractionLost: " << flost << "\n";
	}
	return ostr;
}

CallStatsResponse::CallStatsResponse(Daemon *daemon, LinphoneCall *call, const LinphoneCallStats *stats, bool event) {
	const LinphoneCallParams *callParams = linphone_call_get_current_params(call);
	const char *prefix = "";

	ostringstream ostr;
	if (event) {
		ostr << "Event-type: call-stats\n";
		ostr << "Id: " << daemon->updateCallId(call) << "\n";
		ostr << "Type: ";
		if (stats->type == LINPHONE_CALL_STATS_AUDIO) {
			ostr << "Audio";
		} else {
			ostr << "Video";
		}
		ostr << "\n";
	} else {
		prefix = ((stats->type == LINPHONE_CALL_STATS_AUDIO) ? "Audio-" : "Video-");
	}

	printCallStatsHelper(ostr, stats, prefix);

	if (stats->type == LINPHONE_CALL_STATS_AUDIO) {
		const PayloadType *audioCodec = linphone_call_params_get_used_audio_codec(callParams);
		ostr << PayloadTypeResponse(linphone_call_get_core(call), audioCodec, -1, prefix, false).getBody() << "\n";
	} else {
		const PayloadType *videoCodec = linphone_call_params_get_used_video_codec(callParams);
		ostr << PayloadTypeResponse(linphone_call_get_core(call), videoCodec, -1, prefix, false).getBody() << "\n";
	}

	setBody(ostr.str().c_str());
}


AudioStreamStatsResponse::AudioStreamStatsResponse(Daemon* daemon, AudioStream* stream,
		const LinphoneCallStats *stats, bool event) {
	const char *prefix = "";

	ostringstream ostr;
	if (event) {
		ostr << "Event-type: audio-stream-stats\n";
		ostr << "Id: " << daemon->updateAudioStreamId(stream) << "\n";
		ostr << "Type: ";
		if (stats->type == LINPHONE_CALL_STATS_AUDIO) {
			ostr << "Audio";
		} else {
			ostr << "Video";
		}
		ostr << "\n";
	} else {
		prefix = ((stats->type == LINPHONE_CALL_STATS_AUDIO) ? "Audio-" : "Video-");
	}

	printCallStatsHelper(ostr, stats, prefix);

	setBody(ostr.str().c_str());
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

PayloadTypeParser::PayloadTypeParser(LinphoneCore *core, const string &mime_type, bool accept_all) : mAll(false), mSuccesful(true), mPayloadType(NULL),mPosition(-1){
	int number=-1;
	if (accept_all && (mime_type.compare("ALL") == 0)) {
		mAll = true;
		return;
	}
	istringstream ist(mime_type);
	ist >> number;
	if (ist.fail()) {
		char type[64]={0};
		int rate, channels;
		if (sscanf(mime_type.c_str(), "%63[^/]/%u/%u", type, &rate, &channels) != 3) {
			mSuccesful = false;
			return;
		}
		mPayloadType = linphone_core_find_payload_type(core, type, rate, channels);
		if (mPayloadType) mPosition=bctbx_list_index(linphone_core_get_audio_codecs(core), mPayloadType);
	}else if (number!=-1){
		const bctbx_list_t *elem;
		for(elem=linphone_core_get_audio_codecs(core);elem!=NULL;elem=elem->next){
			if (number==linphone_core_get_payload_type_number(core,(PayloadType*)elem->data)){
				mPayloadType=(PayloadType*)elem->data;
				break;
			}
		}
	}
}

DaemonCommandExample::DaemonCommandExample(const string& command, const string& output)
	: mCommand(command), mOutput(output) {}

DaemonCommand::DaemonCommand(const string& name, const string& proto, const string& description) :
		mName(name), mProto(proto), mDescription(description) {
}

void DaemonCommand::addExample(const DaemonCommandExample *example) {
	mExamples.push_back(example);
}

const string DaemonCommand::getHelp() const {
	ostringstream ost;
	ost << getProto() << endl << endl;
	ost << "Description:" << endl << getDescription() << endl << endl;
	list<const DaemonCommandExample*> examples = getExamples();
	int c = 1;
	for (list<const DaemonCommandExample*>::iterator it = examples.begin(); it != examples.end(); ++it, ++c) {
		ost << "Example " << c << ":" << endl;
		ost << ">" << (*it)->getCommand() << endl;
		ost << (*it)->getOutput() << endl;
		ost << endl;
	}
	return ost.str();
}

bool DaemonCommand::matches(const string& name) const {
	return mName.compare(name) == 0;
}

Daemon::Daemon(const char *config_path, const char *factory_config_path, const char *log_file, const char *pipe_name, bool display_video, bool capture_video) :
		mLSD(0), mLogFile(NULL), mAutoVideo(0), mCallIds(0), mProxyIds(0), mAudioStreamIds(0) {
	ms_mutex_init(&mMutex, NULL);
	mServerFd = (ortp_pipe_t)-1;
	mChildFd = (ortp_pipe_t)-1;
	if (pipe_name == NULL) {
#ifdef HAVE_READLINE
		const char *homedir = getenv("HOME");
		rl_readline_name = (char*)"daemon";
		if (homedir == NULL)
			homedir = ".";
		mHistfile = string(homedir) + string("/.linphone_history");
		read_history(mHistfile.c_str());
		setlinebuf(stdout);
#endif
	} else {
		mServerFd = ortp_server_pipe_create(pipe_name);
#ifndef _WIN32
		listen(mServerFd, 2);
		fprintf(stdout, "Server unix socket created, name=%s fd=%i\n", pipe_name, (int)mServerFd);
#else
		fprintf(stdout, "Named pipe  created, name=%s fd=%i\n", pipe_name, (int)mServerFd);
#endif
	}

	if (log_file != NULL) {
		mLogFile = fopen(log_file, "a+");
		linphone_core_enable_logs(mLogFile);
	} else {
		linphone_core_disable_logs();
	}

	LinphoneCoreVTable vtable;
	memset(&vtable, 0, sizeof(vtable));
	vtable.call_state_changed = callStateChanged;
	vtable.call_stats_updated = callStatsUpdated;
	vtable.dtmf_received = dtmfReceived;
	mLc = linphone_core_new(&vtable, config_path, factory_config_path, this);
	linphone_core_set_user_data(mLc, this);
	linphone_core_enable_video_capture(mLc,capture_video);
	linphone_core_enable_video_display(mLc,display_video);
	initCommands();
	mUseStatsEvents=true;
}

const list<DaemonCommand*> &Daemon::getCommandList() const {
	return mCommands;
}

LinphoneCore *Daemon::getCore() {
	return mLc;
}

LinphoneSoundDaemon *Daemon::getLSD() {
	return mLSD;
}

int Daemon::updateCallId(LinphoneCall *call) {
	int val = (int) (long) linphone_call_get_user_pointer(call);
	if (val == 0) {
		linphone_call_set_user_pointer(call, (void*) (long) ++mCallIds);
		return mCallIds;
	}
	return val;
}

LinphoneCall *Daemon::findCall(int id) {
	const bctbx_list_t *elem = linphone_core_get_calls(mLc);
	for (; elem != NULL; elem = elem->next) {
		LinphoneCall *call = (LinphoneCall *) elem->data;
		if (linphone_call_get_user_pointer(call) == (void*) (long) id)
			return call;
	}
	return NULL;
}

int Daemon::updateProxyId(LinphoneProxyConfig *cfg) {
	int val = (int) (long) linphone_proxy_config_get_user_data(cfg);
	if (val == 0) {
		linphone_proxy_config_set_user_data(cfg, (void*) (long) ++mProxyIds);
		return mProxyIds;
	}
	return val;
}

LinphoneProxyConfig *Daemon::findProxy(int id) {
	const bctbx_list_t *elem = linphone_core_get_proxy_config_list(mLc);
	for (; elem != NULL; elem = elem->next) {
		LinphoneProxyConfig *proxy = (LinphoneProxyConfig *) elem->data;
		if (linphone_proxy_config_get_user_data(proxy) == (void*) (long) id)
			return proxy;
	}
	return NULL;
}

LinphoneAuthInfo *Daemon::findAuthInfo(int id)  {
	const bctbx_list_t *elem = linphone_core_get_auth_info_list(mLc);
	if (elem == NULL || id < 1 || (unsigned int)id > bctbx_list_size(elem)) {
		return NULL;
	}
	while (id > 1) {
		elem = elem->next;
		--id;
	}
	return (LinphoneAuthInfo *) elem->data;
}

int Daemon::updateAudioStreamId(AudioStream *audio_stream) {
	for (std::map<int, AudioStreamAndOther*>::iterator it = mAudioStreams.begin(); it != mAudioStreams.end(); ++it) {
		if (it->second->stream == audio_stream)
			return it->first;
	}

	++mAudioStreamIds;
	mAudioStreams.insert(make_pair(mAudioStreamIds, new AudioStreamAndOther(audio_stream)));
	return mAudioStreamIds;
}

AudioStreamAndOther *Daemon::findAudioStreamAndOther(int id) {
	std::map<int, AudioStreamAndOther*>::iterator it = mAudioStreams.find(id);
	if (it != mAudioStreams.end())
		return it->second;
	return NULL;
}

AudioStream *Daemon::findAudioStream(int id) {
	std::map<int, AudioStreamAndOther*>::iterator it = mAudioStreams.find(id);
	if (it != mAudioStreams.end())
		return it->second->stream;
	return NULL;
}

void Daemon::removeAudioStream(int id) {
	std::map<int, AudioStreamAndOther*>::iterator it = mAudioStreams.find(id);
	if (it != mAudioStreams.end()) {
		mAudioStreams.erase(it);
		delete(it->second);
	}
}

static bool compareCommands(const DaemonCommand *command1, const DaemonCommand *command2) {
	return (command1->getProto() < command2->getProto());
}

void Daemon::initCommands() {
	mCommands.push_back(new RegisterCommand());
	mCommands.push_back(new ContactCommand());
	mCommands.push_back(new RegisterStatusCommand());
	mCommands.push_back(new UnregisterCommand());
	mCommands.push_back(new AuthInfosClearCommand());
	mCommands.push_back(new CallCommand());
	mCommands.push_back(new TerminateCommand());
	mCommands.push_back(new DtmfCommand());
	mCommands.push_back(new PlayWavCommand());
	mCommands.push_back(new PopEventCommand());
	mCommands.push_back(new AnswerCommand());
	mCommands.push_back(new CallStatusCommand());
	mCommands.push_back(new CallStatsCommand());
	mCommands.push_back(new CallPauseCommand());
	mCommands.push_back(new CallMuteCommand());
	mCommands.push_back(new CallResumeCommand());
	mCommands.push_back(new CallTransferCommand());
	mCommands.push_back(new Video());
	mCommands.push_back(new VideoSource());
	mCommands.push_back(new AutoVideo());
	mCommands.push_back(new ConferenceCommand());
	mCommands.push_back(new AudioCodecGetCommand());
	mCommands.push_back(new AudioCodecEnableCommand());
	mCommands.push_back(new AudioCodecDisableCommand());
	mCommands.push_back(new AudioCodecMoveCommand());
	mCommands.push_back(new AudioCodecSetCommand());
	mCommands.push_back(new AudioStreamStartCommand());
	mCommands.push_back(new AudioStreamStopCommand());
	mCommands.push_back(new AudioStreamStatsCommand());
	mCommands.push_back(new MSFilterAddFmtpCommand());
	mCommands.push_back(new PtimeCommand());
	mCommands.push_back(new IPv6Command());
	mCommands.push_back(new FirewallPolicyCommand());
	mCommands.push_back(new MediaEncryptionCommand());
	mCommands.push_back(new PortCommand());
	mCommands.push_back(new AdaptiveBufferCompensationCommand());
	mCommands.push_back(new JitterBufferCommand());
	mCommands.push_back(new JitterBufferResetCommand());
	mCommands.push_back(new VersionCommand());
	mCommands.push_back(new QuitCommand());
	mCommands.push_back(new HelpCommand());
	mCommands.push_back(new ConfigGetCommand());
	mCommands.push_back(new ConfigSetCommand());
	mCommands.push_back(new NetsimCommand());
	mCommands.push_back(new CNCommand());
	mCommands.sort(compareCommands);
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
	mEventQueue.push(new EventResponse(this, call, state));
}

void Daemon::callStatsUpdated(LinphoneCall *call, const LinphoneCallStats *stats) {
	if (mUseStatsEvents) {
		/* don't queue periodical updates (3 per seconds for just bandwidth updates) */
		if (!(stats->updated & LINPHONE_CALL_STATS_PERIODICAL_UPDATE)){
			mEventQueue.push(new CallStatsResponse(this, call, stats, true));
		}
	}
}

void Daemon::dtmfReceived(LinphoneCall *call, int dtmf) {
	mEventQueue.push(new DtmfResponse(this, call, dtmf));
}

void Daemon::callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *msg) {
	Daemon *app = (Daemon*) linphone_core_get_user_data(lc);
	app->callStateChanged(call, state, msg);
}
void Daemon::callStatsUpdated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats) {
	Daemon *app = (Daemon*) linphone_core_get_user_data(lc);
	app->callStatsUpdated(call, stats);
}
void Daemon::dtmfReceived(LinphoneCore *lc, LinphoneCall *call, int dtmf) {
	Daemon *app = (Daemon*) linphone_core_get_user_data(lc);
	app->dtmfReceived(call, dtmf);
}

void Daemon::iterateStreamStats() {
	for (std::map<int, AudioStreamAndOther*>::iterator it = mAudioStreams.begin(); it != mAudioStreams.end(); ++it) {
		OrtpEvent *ev;
		while (it->second->queue && (NULL != (ev=ortp_ev_queue_get(it->second->queue)))){
			OrtpEventType evt=ortp_event_get_type(ev);
			if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED || evt == ORTP_EVENT_RTCP_PACKET_EMITTED) {
				linphone_call_stats_fill(&it->second->stats, &it->second->stream->ms, ev);
				if (mUseStatsEvents) mEventQueue.push(new AudioStreamStatsResponse(this,
					it->second->stream, &it->second->stats, true));
			}
			ortp_event_destroy(ev);
		}
	}
}

void Daemon::iterate() {
	linphone_core_iterate(mLc);
	iterateStreamStats();
	if (mChildFd == (ortp_pipe_t)-1) {
		if (!mEventQueue.empty()) {
			Response *r = mEventQueue.front();
			mEventQueue.pop();
			fprintf(stdout, "%s\n", r->getBody().c_str());
			fflush(stdout);
			delete r;
		}
	}
}

void Daemon::execCommand(const string &command) {
	istringstream ist(command);
	string name;
	ist >> name;
	stringbuf argsbuf;
	ist.get(argsbuf);
	string args = argsbuf.str();
	if (!args.empty() && (args[0] == ' ')) args.erase(0, 1);
	list<DaemonCommand*>::iterator it = find_if(mCommands.begin(), mCommands.end(), bind2nd(mem_fun(&DaemonCommand::matches), name));
	if (it != mCommands.end()) {
		ms_mutex_lock(&mMutex);
		(*it)->exec(this, args);
		ms_mutex_unlock(&mMutex);
	} else {
		sendResponse(Response("Unknown command."));
	}
}

void Daemon::sendResponse(const Response &resp) {
	string buf = resp.toBuf();
	if (mChildFd != (ortp_pipe_t)-1) {
		if (ortp_pipe_write(mChildFd, (uint8_t *)buf.c_str(), buf.size()) == -1) {
			ms_error("Fail to write to pipe: %s", strerror(errno));
		}
	} else {
		cout << buf << flush;
	}
}

string Daemon::readPipe() {
	char buffer[32768];
	memset(buffer, '\0', sizeof(buffer));
#ifdef _WIN32
	if (mChildFd == (ortp_pipe_t)-1) {
		mChildFd = ortp_server_pipe_accept_client(mServerFd);
		ms_message("Client accepted");
	}
	if (mChildFd != (ortp_pipe_t)-1) {
		int ret = ortp_pipe_read(mChildFd, (uint8_t *)buffer, sizeof(buffer));
		if (ret == -1) {
			ms_error("Fail to read from pipe: %s", strerror(errno));
			mChildFd = (ortp_pipe_t)-1;
		} else {
			if (ret == 0) {
				ms_message("Client disconnected");
				mChildFd = (ortp_pipe_t)-1;
				return "";
			}
			buffer[ret] = '\0';
			return buffer;
		}
	}
#else
	struct pollfd pfd[2];
	int nfds = 1;
	memset(&pfd[0], 0, sizeof(pfd));
	if (mServerFd != (ortp_pipe_t)-1) {
		pfd[0].events = POLLIN;
		pfd[0].fd = mServerFd;
	}
	if (mChildFd != (ortp_pipe_t)-1) {
		pfd[1].events = POLLIN;
		pfd[1].fd = mChildFd;
		nfds++;
	}
	int err = poll(pfd, nfds, 50);
	if (err > 0) {
		if (mServerFd != (ortp_pipe_t)-1 && (pfd[0].revents & POLLIN)) {
			struct sockaddr_storage addr;
			socklen_t addrlen = sizeof(addr);
			int childfd = accept(mServerFd, (struct sockaddr*) &addr, &addrlen);
			if (childfd != -1) {
				if (mChildFd != (ortp_pipe_t)-1) {
					ms_error("Cannot accept two client at the same time");
					close(childfd);
				} else {
					mChildFd = (ortp_pipe_t)childfd;
					return NULL;
				}
			}
		}
		if (mChildFd != (ortp_pipe_t)-1 && (pfd[1].revents & POLLIN)) {
			int ret;
			if ((ret = ortp_pipe_read(mChildFd, (uint8_t *)buffer, sizeof(buffer))) == -1) {
				ms_error("Fail to read from pipe: %s", strerror(errno));
			} else {
				if (ret == 0) {
					ms_message("Client disconnected");
					ortp_server_pipe_close_client(mChildFd);
					mChildFd = (ortp_pipe_t)-1;
					return NULL;
				}
				buffer[ret] = '\0';
				return buffer;
			}
		}
	}
#endif
	return NULL;
}

void Daemon::dumpCommandsHelp() {
	int cols = 80;
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	cols = ts.ts_cols;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	cols = ts.ws_col;
#endif

	cout << endl;
	for (list<DaemonCommand*>::iterator it = mCommands.begin(); it != mCommands.end(); ++it) {
		cout << setfill('-') << setw(cols) << "-" << endl << endl;
		cout << (*it)->getHelp();
	}
}

static string htmlEscape(const string &orig){
	string ret=orig;
	size_t pos;

	while(1){
		pos=ret.find('<');
		if (pos!=string::npos){
			ret.erase(pos,1);
			ret.insert(pos,"&lt");
			continue;
		}
		pos=ret.find('>');
		if (pos!=string::npos){
			ret.erase(pos,1);
			ret.insert(pos,"&gt");
			continue;
		}
		break;
	}
	while(1){
		pos=ret.find('\n');
		if (pos!=string::npos){
			ret.erase(pos,1);
			ret.insert(pos,"<br>");
			continue;
		}
		break;
	}
	return ret;
}

void Daemon::dumpCommandsHelpHtml(){
	cout << endl;
	cout << "<!DOCTYPE html><html><body>"<<endl;
	cout << "<h1>List of linphone-daemon commands.</h1>"<<endl;
	for (list<DaemonCommand*>::iterator it = mCommands.begin(); it != mCommands.end(); ++it) {
		cout<<"<h2>"<<htmlEscape((*it)->getProto())<<"</h2>"<<endl;
		cout<<"<h3>"<<"Description"<<"</h3>"<<endl;
		cout<<"<p>"<<htmlEscape((*it)->getDescription())<<"</p>"<<endl;
		cout<<"<h3>"<<"Examples"<<"</h3>"<<endl;
		const std::list<const DaemonCommandExample*> &examples=(*it)->getExamples();
		cout<<"<p><i>";
		for(list<const DaemonCommandExample*>::const_iterator ex_it=examples.begin();ex_it!=examples.end();++ex_it){
			cout<<"<b>"<<htmlEscape("Linphone-daemon>")<<htmlEscape((*ex_it)->getCommand())<<"</b><br>"<<endl;
			cout<<htmlEscape((*ex_it)->getOutput())<<"<br>"<<endl;
			cout<<"<br><br>";
		}
		cout<<"</i></p>"<<endl;
	}

	cout << "</body></html>"<<endl;
}


static void printHelp() {
	cout << "daemon-linphone [<options>]" << endl <<
#if defined(LICENCE_GPL) || defined(LICENCE_COMMERCIAL)
		"Licence: "
#ifdef LICENCE_GPL
		"GPL"
#endif
#ifdef LICENCE_COMMERCIAL
		"Commercial"
#endif
		<< endl <<
#endif

		"where options are :" << endl <<
		"\t--help                     Print this notice." << endl <<
		"\t--dump-commands-help       Dump the help of every available commands." << endl <<
		"\t--dump-commands-html-help  Dump the help of every available commands." << endl <<
		"\t--pipe <pipename>          Create an unix server socket to receive commands." << endl <<
		"\t--log <path>               Supply a file where the log will be saved." << endl <<
		"\t--factory-config <path>    Supply a readonly linphonerc style config file to start with." << endl <<
		"\t--config <path>            Supply a linphonerc style config file to start with." << endl <<
		"\t--disable-stats-events     Do not automatically raise RTP statistics events." << endl <<
		"\t--enable-lsd               Use the linphone sound daemon." << endl <<
		"\t-C                         Enable video capture." << endl <<
		"\t-D                         Enable video display." << endl;
}

void Daemon::startThread() {
	ms_thread_create(&this->mThread, NULL, Daemon::iterateThread, this);
}

#ifdef max
#undef max
#endif

string Daemon::readLine(const string& prompt, bool *eof) {
	*eof=false;
#ifdef HAVE_READLINE
	return readline(prompt.c_str());
#else
	if (cin.eof()) {
		*eof=true;
		return "";
	}
	cout << prompt;
	stringbuf outbuf;
	cin.get(outbuf);
	cin.clear();
	cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	return outbuf.str();
#endif
}

int Daemon::run() {
	const string prompt("daemon-linphone>");
	mRunning = true;
	startThread();
	while (mRunning) {
		string line;
		bool eof=false;
		if (mServerFd == (ortp_pipe_t)-1) {
			line = readLine(prompt, &eof);
			if (!line.empty()) {
#ifdef HAVE_READLINE
				add_history(line.c_str());
#endif
			}
		} else {
			line = readPipe();
		}
		if (!line.empty()) {
			execCommand(line);
		}
		if (eof && mRunning) {
			mRunning = false; // ctrl+d
			cout << "Quitting..." << endl;
		}
	}
	stopThread();
	return 0;
}

void Daemon::stopThread() {
	void *ret;
	ms_thread_join(mThread, &ret);
}

void Daemon::quit() {
	mRunning = false;
}

void Daemon::enableStatsEvents(bool enabled){
	mUseStatsEvents=enabled;
}

void Daemon::enableLSD(bool enabled) {
	if (mLSD) linphone_sound_daemon_destroy(mLSD);
	linphone_core_use_sound_daemon(mLc, NULL);
	if (enabled) {
		mLSD = linphone_sound_daemon_new(mLc->factory,NULL, 44100, 1);
		linphone_core_use_sound_daemon(mLc, mLSD);
	}
}

Daemon::~Daemon() {
	uninitCommands();

	for (std::map<int, AudioStreamAndOther *>::iterator it = mAudioStreams.begin(); it != mAudioStreams.end(); ++it) {
		audio_stream_stop(it->second->stream);
	}

	enableLSD(false);
	linphone_core_unref(mLc);
	if (mChildFd != (ortp_pipe_t)-1) {
		ortp_server_pipe_close_client(mChildFd);
	}
	if (mServerFd != (ortp_pipe_t)-1) {
		ortp_server_pipe_close(mServerFd);
	}
	if (mLogFile != NULL) {
		linphone_core_enable_logs(NULL);
		fclose(mLogFile);
	}

	ms_mutex_destroy(&mMutex);

#ifdef HAVE_READLINE
	stifle_history(30);
	write_history(mHistfile.c_str());
#endif
}

int main(int argc, char *argv[]) {
	const char *config_path = NULL;
	const char *factory_config_path = NULL;
	const char *pipe_name = NULL;
	const char *log_file = NULL;
	bool capture_video = false;
	bool display_video = false;
	bool stats_enabled = true;
	bool lsd_enabled = false;
	int i;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0) {
			printHelp();
			return 0;
		} else if (strcmp(argv[i], "--dump-commands-help") == 0) {
			Daemon app(NULL, NULL, NULL, NULL, false, false);
			app.dumpCommandsHelp();
			return 0;
		}else if (strcmp(argv[i], "--dump-commands-html-help") == 0) {
			Daemon app(NULL, NULL, NULL, NULL, false, false);
			app.dumpCommandsHelpHtml();
			return 0;
		} else if (strcmp(argv[i], "--pipe") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no pipe name specify after --pipe\n");
				return -1;
			}
			pipe_name = argv[++i];
			stats_enabled = false;
		} else if (strcmp(argv[i], "--factory-config") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no file specify after --factory-config\n");
				return -1;
			}
			factory_config_path = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "--config") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no file specify after --config\n");
				return -1;
			}
			config_path = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "--log") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "no file specify after --log\n");
				return -1;
			}
			log_file = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "-C") == 0) {
			capture_video = true;
		} else if (strcmp(argv[i], "-D") == 0) {
			display_video = true;
		}else if (strcmp(argv[i],"--disable-stats-events")==0){
			stats_enabled = false;
		} else if (strcmp(argv[i], "--enable-lsd") == 0) {
			lsd_enabled = true;
		}
		else{
			fprintf(stderr, "Unrecognized option : %s", argv[i]);
		}
	}
	Daemon app(config_path, factory_config_path, log_file, pipe_name, display_video, capture_video);
	app.enableStatsEvents(stats_enabled);
	app.enableLSD(lsd_enabled);
	return app.run();
}
