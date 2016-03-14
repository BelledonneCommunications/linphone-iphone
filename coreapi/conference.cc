/*******************************************************************************
 *            conference.cc
 *
 *  Thu Nov 26, 2015
 *  Copyright  2015  Belledonne Communications
 *  Author: Linphone's team
 *  Email info@belledonne-communications.com
 ******************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "private.h"
#include "conference_private.h"
#include <mediastreamer2/msvolume.h>
#include <typeinfo>
#include <list>
#include <algorithm>

namespace Linphone {

class Conference {
public:
	class Participant {
	public:
		Participant(LinphoneCall *call);
		Participant(const Participant &src);
		~Participant();
		bool operator==(const Participant &src) const;
		const LinphoneAddress *getUri() const {return m_uri;}
		LinphoneCall *getCall() const {return m_call;}
		
	private:
		LinphoneAddress *m_uri;
		LinphoneCall *m_call;
	};
	
	class Params {
	public:
		Params(const LinphoneCore *core = NULL);
		void enableVideo(bool enable) {m_enableVideo = enable;}
		bool videoRequested() const {return m_enableVideo;}
		void setStateChangedCallback(LinphoneConferenceStateChangedCb cb, void *user_data) {
			m_state_changed_cb=cb;
			m_user_data=user_data;
		}
		
	private:
		bool m_enableVideo;
		LinphoneConferenceStateChangedCb m_state_changed_cb;
		void *m_user_data;
		
		friend class Conference;
	};
	
	Conference(LinphoneCore *core, const Params *params = NULL);
	virtual ~Conference() {};
	
	const Params &getCurrentParams() const {return m_currentParams;}
	
	virtual int addParticipant(LinphoneCall *call) = 0;
	virtual int removeParticipant(LinphoneCall *call) = 0;
	virtual int removeParticipant(const LinphoneAddress *uri) = 0;
	virtual int terminate() = 0;
	
	virtual int enter() = 0;
	virtual int leave() = 0;
	virtual bool isIn() const = 0;
	
	AudioStream *getAudioStream() const {return m_localParticipantStream;}
	int muteMicrophone(bool val);
	bool microphoneIsMuted() const {return m_isMuted;}
	float getInputVolume() const;
	
	virtual int getSize() const {return m_participants.size() + (isIn()?1:0);}
	const std::list<Participant> &getParticipants() const {return m_participants;}
	
	virtual int startRecording(const char *path) = 0;
	virtual int stopRecording() = 0;
	
	virtual void onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote) {};
	virtual void onCallStreamStopping(LinphoneCall *call) {};
	virtual void onCallTerminating(LinphoneCall *call) {};
	
	LinphoneConferenceState getState() const {return m_state;}
	static const char *stateToString(LinphoneConferenceState state);
	
protected:
	const Participant *findParticipant(const LinphoneAddress* uri) const;
	void setState(LinphoneConferenceState state);
	
	LinphoneCore *m_core;
	AudioStream *m_localParticipantStream;
	bool m_isMuted;
	std::list<Participant> m_participants;
	Params m_currentParams;
	LinphoneConferenceState m_state;
};

class LocalConference: public Conference {
public:
	LocalConference(LinphoneCore *core, const Params *params = NULL);
	virtual ~LocalConference();
	
	virtual int addParticipant(LinphoneCall *call);
	virtual int removeParticipant(LinphoneCall *call);
	virtual int removeParticipant(const LinphoneAddress *uri);
	virtual int terminate();
	
	virtual int enter();
	virtual int leave();
	virtual bool isIn() const {return m_localParticipantStream!=NULL;}
	virtual int getSize() const;
	
	virtual int startRecording(const char *path);
	virtual int stopRecording();
	
	virtual void onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote);
	virtual void onCallStreamStopping(LinphoneCall *call);
	virtual void onCallTerminating(LinphoneCall *call);
	
private:
	void addLocalEndpoint();
	int remoteParticipantsCount();
	void removeLocalEndpoint();
	int removeFromConference(LinphoneCall *call, bool_t active);
	int convertConferenceToCall();
	static RtpProfile *sMakeDummyProfile(int samplerate);

	MSAudioConference *m_conf;
	MSAudioEndpoint *m_localEndpoint;
	MSAudioEndpoint *m_recordEndpoint;
	RtpProfile *m_localDummyProfile;
	bool_t m_terminated;
};

class RemoteConference: public Conference {
public:
	RemoteConference(LinphoneCore *core, const Params *params = NULL);
	virtual ~RemoteConference();
	
	virtual int addParticipant(LinphoneCall *call);
	virtual int removeParticipant(LinphoneCall *call) {return -1;}
	virtual int removeParticipant(const LinphoneAddress *uri);
	virtual int terminate();
	
	virtual int enter();
	virtual int leave();
	virtual bool isIn() const;
	
	virtual int startRecording(const char *path) {return 0;}
	virtual int stopRecording() {return 0;}

private:
	bool focusIsReady() const;
	bool transferToFocus(LinphoneCall *call);
	void reset();
	
	void onFocusCallSateChanged(LinphoneCallState state);
	void onPendingCallStateChanged(LinphoneCall *call, LinphoneCallState state);
	void onTransferingCallStateChanged(LinphoneCall *transfered, LinphoneCallState newCallState);
	
	static void callStateChangedCb(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message);
	static void transferStateChanged(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);
	
	const char *m_focusAddr;
	char *m_focusContact;
	LinphoneCall *m_focusCall;
	LinphoneCoreVTable *m_vtable;
	MSList *m_pendingCalls;
	MSList *m_transferingCalls;
	bool m_isTerminating;
};

};


using namespace Linphone;
using namespace std;


Conference::Participant::Participant(LinphoneCall *call) {
	m_uri = linphone_address_clone(linphone_call_get_remote_address(call));
	m_call = linphone_call_ref(call);
}

Conference::Participant::Participant(const Participant &src) {
	m_uri = linphone_address_clone(src.m_uri);
	m_call = src.m_call ? linphone_call_ref(src.m_call) : NULL;
}

Conference::Participant::~Participant() {
	linphone_address_unref(m_uri);
	if(m_call) linphone_call_unref(m_call);
}

bool Conference::Participant::operator==(const Participant &src) const {
	return linphone_address_equal(m_uri, src.m_uri);
}



Conference::Params::Params(const LinphoneCore *core): m_enableVideo(false) {
	if(core) {
		const LinphoneVideoPolicy *policy = linphone_core_get_video_policy(core);
		if(policy->automatically_initiate) m_enableVideo = true;
	}
}


Conference::Conference(LinphoneCore *core, const Conference::Params *params):
	m_core(core),
	m_localParticipantStream(NULL),
	m_isMuted(false),
	m_currentParams(),
	m_state(LinphoneConferenceStopped) {
	if(params) m_currentParams = *params;
}

int Conference::addParticipant(LinphoneCall *call) {
	Participant participant(call);
	m_participants.push_back(participant);
	call->conf_ref = (LinphoneConference *)this;
	return 0;
}

int Conference::removeParticipant(LinphoneCall *call) {
	Participant participant(call);
	m_participants.remove(participant);
	call->conf_ref = NULL;
	return 0;
}

int Conference::terminate() {
	for(list<Participant>::iterator it = m_participants.begin(); it!=m_participants.end(); it++) {
		it->getCall()->conf_ref = NULL;
	}
	m_participants.clear();
	return 0;
}

int Conference::removeParticipant(const LinphoneAddress *uri) {
	const Participant *participant = findParticipant(uri);
	if(participant == NULL) return -1;
	LinphoneCall *call = participant->getCall();
	if(call) call->conf_ref = NULL;
	m_participants.remove(Participant(*participant));
	return 0;
}

int Conference::muteMicrophone(bool val)  {
	if (val) {
		audio_stream_set_mic_gain(m_localParticipantStream, 0);
	} else {
		audio_stream_set_mic_gain_db(m_localParticipantStream, m_core->sound_conf.soft_mic_lev);
	}
	if ( linphone_core_get_rtp_no_xmit_on_audio_mute(m_core) ){
		audio_stream_mute_rtp(m_localParticipantStream, val);
	}
	m_isMuted=val;
	return 0;
}

float Conference::getInputVolume() const {
	AudioStream *st=m_localParticipantStream;
	if (st && st->volsend && !m_isMuted){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET,&vol);
		return vol;

	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

const char *Conference::stateToString(LinphoneConferenceState state) {
	switch(state) {
		case LinphoneConferenceStopped: return "Stopped";
		case LinphoneConferenceStarting: return "Starting";
		case LinphoneConferenceReady: return "Ready";
		case LinphoneConferenceStartingFailed: return "Startig failed";
		default: return "Invalid state";
	}
}



const Conference::Participant *Conference::findParticipant(const LinphoneAddress *uri) const {
	list<Participant>::const_iterator it = m_participants.begin();
	while(it!=m_participants.end()) {
		if(linphone_address_equal(uri, it->getUri())) break;
		it++;
	}
	if(it == m_participants.end()) return NULL;
	else return &*it;
}

void Conference::setState(LinphoneConferenceState state) {
	if(m_state != state) {
		ms_message("Switching conference [%p] into state '%s'", this, stateToString(state));
		m_state = state;
		if(m_currentParams.m_state_changed_cb) {
			m_currentParams.m_state_changed_cb((LinphoneConference *)this, state, m_currentParams.m_user_data);
		}
	}
}




LocalConference::LocalConference(LinphoneCore *core, const Conference::Params *params):
	Conference(core, params),
	m_conf(NULL),
	m_localEndpoint(NULL),
	m_recordEndpoint(NULL),
	m_localDummyProfile(NULL),
	m_terminated(FALSE) {

	MSAudioConferenceParams ms_conf_params;
	ms_conf_params.samplerate = lp_config_get_int(m_core->config, "sound","conference_rate",16000);
	m_conf=ms_audio_conference_new(&ms_conf_params, core->factory);
	m_state=LinphoneConferenceReady;
}

LocalConference::~LocalConference() {
	terminate();
	ms_audio_conference_destroy(m_conf);
}

RtpProfile *LocalConference::sMakeDummyProfile(int samplerate){
	RtpProfile *prof=rtp_profile_new("dummy");
	PayloadType *pt=payload_type_clone(&payload_type_l16_mono);
	pt->clock_rate=samplerate;
	rtp_profile_set_payload(prof,0,pt);
	return prof;
}

void LocalConference::addLocalEndpoint() {
	/*create a dummy audiostream in order to extract the local part of it */
	/* network address and ports have no meaning and are not used here. */
	AudioStream *st=audio_stream_new(m_core->factory, 65000,65001,FALSE);
	MSSndCard *playcard=m_core->sound_conf.lsd_card ?
			m_core->sound_conf.lsd_card : m_core->sound_conf.play_sndcard;
	MSSndCard *captcard=m_core->sound_conf.capt_sndcard;
	const MSAudioConferenceParams *params=ms_audio_conference_get_params(m_conf);
	m_localDummyProfile=sMakeDummyProfile(params->samplerate);

	audio_stream_start_full(st, m_localDummyProfile,
				"127.0.0.1",
				65000,
				"127.0.0.1",
				65001,
				0,
				40,
				NULL,
				NULL,
				playcard,
				captcard,
				linphone_core_echo_cancellation_enabled(m_core)
				);
	_post_configure_audio_stream(st,m_core,FALSE);
	m_localParticipantStream=st;
	m_localEndpoint=ms_audio_endpoint_get_from_stream(st,FALSE);
	ms_audio_conference_add_member(m_conf,m_localEndpoint);
}

int LocalConference::addParticipant(LinphoneCall *call) {
	if (call->current_params->in_conference){
		ms_error("Already in conference");
		return -1;
	}

	if (call->state==LinphoneCallPaused){
		call->params->in_conference=TRUE;
		call->params->has_video=FALSE;
		linphone_core_resume_call(m_core,call);
	}else if (call->state==LinphoneCallStreamsRunning){
		LinphoneCallParams *params=linphone_call_params_copy(linphone_call_get_current_params(call));
		params->in_conference=TRUE;
		params->has_video=FALSE;

		if (call->audiostream || call->videostream){
			linphone_call_stop_media_streams(call); /*free the audio & video local resources*/
			linphone_call_init_media_streams(call);
		}
		if (call==m_core->current_call){
			m_core->current_call=NULL;
		}
		/*this will trigger a reINVITE that will later redraw the streams */
		/*FIXME probably a bit too much to just redraw streams !*/
		linphone_core_update_call(m_core,call,params);
		linphone_call_params_destroy(params);
		addLocalEndpoint();
	}else{
		ms_error("Call is in state %s, it cannot be added to the conference.",linphone_call_state_to_string(call->state));
		return -1;
	}
	Conference::addParticipant(call);
	return 0;
}

int LocalConference::removeFromConference(LinphoneCall *call, bool_t active){
	int err=0;
	char *str;

	if (!call->current_params->in_conference){
		if (call->params->in_conference){
			ms_warning("Not (yet) in conference, be patient");
			return -1;
		}else{
			ms_error("Not in a conference.");
			return -1;
		}
	}
	call->params->in_conference=FALSE;
	Conference::removeParticipant(call);

	str=linphone_call_get_remote_address_as_string(call);
	ms_message("%s will be removed from conference", str);
	ms_free(str);
	if (active){
		LinphoneCallParams *params=linphone_call_params_copy(linphone_call_get_current_params(call));
		params->in_conference=FALSE;
		// reconnect local audio with this call
		if (isIn()){
			ms_message("Leaving conference for reconnecting with unique call.");
			leave();
		}
		ms_message("Updating call to actually remove from conference");
		err=linphone_core_update_call(m_core,call,params);
		linphone_call_params_destroy(params);
	} else{
		ms_message("Pausing call to actually remove from conference");
		err=_linphone_core_pause_call(m_core,call);
	}
	return err;
}

int LocalConference::remoteParticipantsCount() {
	int count=getSize();
	if (count==0) return 0;
	if (!m_localParticipantStream) return count;
	return count -1;
}

int LocalConference::convertConferenceToCall(){
	int err=0;
	MSList *calls=m_core->calls;

	if (remoteParticipantsCount()!=1){
		ms_error("No unique call remaining in conference.");
		return -1;
	}

	while (calls) {
		LinphoneCall *rc=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (rc->params->in_conference) { // not using current_param
			bool_t active_after_removed=isIn();
			err=removeFromConference(rc, active_after_removed);
			break;
		}
	}
	return err;
}

int LocalConference::removeParticipant(LinphoneCall *call) {
	int err;
	char * str=linphone_call_get_remote_address_as_string(call);
	ms_message("Removing call %s from the conference", str);
	ms_free(str);
	err=removeFromConference(call, FALSE);
	if (err){
		ms_error("Error removing participant from conference.");
		return err;
	}

	if (remoteParticipantsCount()==1){
		ms_message("conference size is 1: need to be converted to plain call");
		err=convertConferenceToCall();
	} else {
		ms_message("the conference need not to be converted as size is %i", remoteParticipantsCount());
	}
	return err;
}

int LocalConference::removeParticipant(const LinphoneAddress *uri) {
	const Participant *participant = findParticipant(uri);
	if(participant == NULL) return -1;
	LinphoneCall *call = participant->getCall();
	if(call == NULL) return -1;
	return removeParticipant(call);
}

int LocalConference::terminate() {
	MSList *calls=m_core->calls;
	m_terminated=TRUE;

	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (call->current_params->in_conference) {
			linphone_core_terminate_call(m_core, call);
		}
	}
	
	Conference::terminate();
	
	return 0;
}

int LocalConference::enter() {
	if (linphone_core_sound_resources_locked(m_core)) {
		return -1;
	}
	if (m_core->current_call != NULL) {
		_linphone_core_pause_call(m_core, m_core->current_call);
	}
	if (m_localParticipantStream==NULL) addLocalEndpoint();
	return 0;
}

void LocalConference::removeLocalEndpoint(){
	if (m_localEndpoint){
		ms_audio_conference_remove_member(m_conf,m_localEndpoint);
		ms_audio_endpoint_release_from_stream(m_localEndpoint);
		m_localEndpoint=NULL;
		audio_stream_stop(m_localParticipantStream);
		m_localParticipantStream=NULL;
		rtp_profile_destroy(m_localDummyProfile);
	}
}

int LocalConference::leave() {
	if (isIn())
		removeLocalEndpoint();
	return 0;
}

int LocalConference::getSize() const {
	if (m_conf == NULL) {
		return 0;
	}
	return ms_audio_conference_get_size(m_conf) - (m_recordEndpoint ? 1 : 0);
}

int LocalConference::startRecording(const char *path) {
	if (m_conf == NULL) {
		ms_warning("linphone_core_start_conference_recording(): no conference now.");
		return -1;
	}
	if (m_recordEndpoint==NULL){
		m_recordEndpoint=ms_audio_endpoint_new_recorder(m_core->factory);
		ms_audio_conference_add_member(m_conf,m_recordEndpoint);
	}
	ms_audio_recorder_endpoint_start(m_recordEndpoint,path);
	return 0;
}

int LocalConference::stopRecording() {
	if (m_conf == NULL) {
		ms_warning("linphone_core_stop_conference_recording(): no conference now.");
		return -1;
	}
	if (m_recordEndpoint==NULL){
		ms_warning("linphone_core_stop_conference_recording(): no record active.");
		return -1;
	}
	ms_audio_recorder_endpoint_stop(m_recordEndpoint);
	return 0;
}

void LocalConference::onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote) {
	call->params->has_video = FALSE;
	call->camera_enabled = FALSE;
	MSAudioEndpoint *ep=ms_audio_endpoint_get_from_stream(call->audiostream,TRUE);
	ms_audio_conference_add_member(m_conf,ep);
	ms_audio_conference_mute_member(m_conf,ep,isPausedByRemote);
	call->endpoint=ep;
}

void LocalConference::onCallStreamStopping(LinphoneCall *call) {
	ms_audio_conference_remove_member(m_conf,call->endpoint);
	ms_audio_endpoint_release_from_stream(call->endpoint);
	call->endpoint=NULL;
}

void LocalConference::onCallTerminating(LinphoneCall *call) {
	int remote_count=remoteParticipantsCount();
	ms_message("conference_check_uninit(): size=%i", getSize());
	if (remote_count==1 && !m_terminated){
		convertConferenceToCall();
	}
	if (remote_count==0){
		if (m_localParticipantStream)
			removeLocalEndpoint();
		if (m_recordEndpoint){
			ms_audio_conference_remove_member(m_conf, m_recordEndpoint);
			ms_audio_endpoint_destroy(m_recordEndpoint);
		}
	}
}



RemoteConference::RemoteConference(LinphoneCore *core, const Conference::Params *params):
	Conference(core, params),
	m_focusAddr(NULL),
	m_focusContact(NULL),
	m_focusCall(NULL),
	m_vtable(NULL),
	m_pendingCalls(NULL),
	m_transferingCalls(NULL),
	m_isTerminating(false) {
	m_focusAddr = lp_config_get_string(m_core->config, "misc", "conference_focus_addr", "");
	m_vtable = linphone_core_v_table_new();
	m_vtable->call_state_changed = callStateChangedCb;
	m_vtable->transfer_state_changed = transferStateChanged;
	linphone_core_v_table_set_user_data(m_vtable, this);
	_linphone_core_add_listener(m_core, m_vtable, FALSE, TRUE);
}

RemoteConference::~RemoteConference() {
	terminate();
	linphone_core_remove_listener(m_core, m_vtable);
	linphone_core_v_table_destroy(m_vtable);
}

int RemoteConference::addParticipant(LinphoneCall *call) {
	LinphoneAddress *addr;
	LinphoneCallParams *params;
	
	switch(m_state) {
		case LinphoneConferenceStopped:
		case LinphoneConferenceStartingFailed:
			Conference::addParticipant(call);
			ms_message("Calling the conference focus (%s)", m_focusAddr);
			addr = linphone_address_new(m_focusAddr);
			if(addr) {
				params = linphone_core_create_call_params(m_core, NULL);
				linphone_call_params_enable_video(params, m_currentParams.videoRequested());
				m_focusCall = linphone_core_invite_address_with_params(m_core, addr, params);
				m_focusCall->conf_ref = (LinphoneConference *)this;
				m_localParticipantStream = m_focusCall->audiostream;
				m_pendingCalls = ms_list_append(m_pendingCalls, call);
				LinphoneCallLog *callLog = linphone_call_get_call_log(m_focusCall);
				callLog->was_conference = TRUE;
				linphone_address_unref(addr);
				linphone_call_params_unref(params);
				setState(LinphoneConferenceStarting);
				return 0;
			} else return -1;

		case LinphoneConferenceStarting:
			Conference::addParticipant(call);
			if(focusIsReady()) {
				transferToFocus(call);
			} else {
				m_pendingCalls = ms_list_append(m_pendingCalls, call);
			}
			return 0;
			
		case LinphoneConferenceReady:
			Conference::addParticipant(call);
			transferToFocus(call);
			return 0;

		default:
			ms_error("Could not add call %p to the conference. Bad conference state (%s)", call, stateToString(m_state));
			return -1;
	}
}

int RemoteConference::removeParticipant(const LinphoneAddress *uri) {
	char *refer_to;
	LinphoneAddress *refer_to_addr;
	int res;
	
	switch(m_state) {
		case LinphoneConferenceReady:
			if(findParticipant(uri) == NULL) {
				char *tmp = linphone_address_as_string(uri);
				ms_error("Conference: could not remove participant '%s': not in the participants list", tmp);
				ms_free(tmp);
				return -1;
			}
			
// 			refer_to = ms_strdup_printf("%s;method=BYE", tmp);
			refer_to_addr = linphone_address_clone(uri);
			linphone_address_set_method_param(refer_to_addr, "BYE");
			refer_to = linphone_address_as_string(refer_to_addr);
			linphone_address_unref(refer_to_addr);
			res = sal_call_refer(m_focusCall->op, refer_to);
			ms_free(refer_to);
			
			if(res == 0) {
				return Conference::removeParticipant(uri);
			} else {
				char *tmp = linphone_address_as_string(uri);
				ms_error("Conference: could not remove participant '%s': REFER with BYE has failed", tmp);
				ms_free(tmp);
				return -1;
			}
			
		default:
			ms_error("Cannot remove %s from conference: Bad conference state (%s)", linphone_address_as_string(uri), stateToString(m_state));
			return -1;
	}
}

int RemoteConference::terminate() {
	m_isTerminating = true;
	switch(m_state) {
		case LinphoneConferenceReady:
		case LinphoneConferenceStarting:
			linphone_core_terminate_call(m_core, m_focusCall);
			reset();
			Conference::terminate();
			m_isTerminating = false;
			setState(LinphoneConferenceStopped);
			break;
			
		default:
			m_isTerminating = false;
			break;
	}
	
	return 0;
}

int RemoteConference::enter() {
	if(m_state != LinphoneConferenceReady) {
		ms_error("Could not enter in the conference: bad conference state (%s)", stateToString(m_state));
		return -1;
	}
	LinphoneCallState callState = linphone_call_get_state(m_focusCall);
	switch(callState) {
		case LinphoneCallStreamsRunning: break;
		case LinphoneCallPaused:
			linphone_core_resume_call(m_core, m_focusCall);
			break;
		default:
			ms_error("Could not join the conference: bad focus call state (%s)", linphone_call_state_to_string(callState));
			return -1;
	}
	return 0;
}

int RemoteConference::leave() {
	if(m_state != LinphoneConferenceReady) {
		ms_error("Could not leave the conference: bad conference state (%s)", stateToString(m_state));
		return -1;
	}
	LinphoneCallState callState = linphone_call_get_state(m_focusCall);
	switch(callState) {
		case LinphoneCallPaused: break;
		case LinphoneCallStreamsRunning:
			linphone_core_pause_call(m_core, m_focusCall);
			break;
		default:
			ms_error("Could not leave the conference: bad focus call state (%s)", linphone_call_state_to_string(callState));
			return -1;
	}
	return 0;
}

bool RemoteConference::isIn() const {
	if(m_state != LinphoneConferenceReady) return false;
	LinphoneCallState callState = linphone_call_get_state(m_focusCall);
	return callState == LinphoneCallStreamsRunning;
}

bool RemoteConference::focusIsReady() const {
	LinphoneCallState focusState;
	if(m_focusCall == NULL) return false;
	focusState = linphone_call_get_state(m_focusCall);
	return focusState == LinphoneCallStreamsRunning || focusState == LinphoneCallPaused;
}

bool RemoteConference::transferToFocus(LinphoneCall *call) {
	if(linphone_core_transfer_call(m_core, call, m_focusContact) == 0) {
		m_transferingCalls = ms_list_append(m_transferingCalls, call);
		return true;
	} else {
		ms_error("Conference: could not transfer call [%p] to %s", call, m_focusContact);
		return false;
	}
}

void RemoteConference::reset() {
	m_localParticipantStream = NULL;
	m_focusAddr = NULL;
	if(m_focusContact) {
		ms_free(m_focusContact);
		m_focusContact = NULL;
	}
	m_focusCall = NULL;
	m_pendingCalls = ms_list_free(m_pendingCalls);
	m_transferingCalls = ms_list_free(m_transferingCalls);
}

void RemoteConference::onFocusCallSateChanged(LinphoneCallState state) {
	MSList *it;
	
	switch (state) {
		case LinphoneCallConnected:
			m_focusContact = ms_strdup(linphone_call_get_remote_contact(m_focusCall));
			it = m_pendingCalls;
			while (it) {
				LinphoneCall *pendingCall = (LinphoneCall *)it->data;
				LinphoneCallState pendingCallState = linphone_call_get_state(pendingCall);
				if(pendingCallState == LinphoneCallStreamsRunning || pendingCallState == LinphoneCallPaused) {
					MSList *current_elem = it;
					it = it->next;
					m_pendingCalls = ms_list_remove_link(m_pendingCalls, current_elem);
					transferToFocus(pendingCall);
				} else {
					it = it->next;
				}
			}
			setState(LinphoneConferenceReady);
			break;

		case LinphoneCallError:
			reset();
			setState(LinphoneConferenceStartingFailed);
			break;
			
		case LinphoneCallEnd:
			if(!m_isTerminating) terminate();
			break;

		default: break;
	}
}

void RemoteConference::onPendingCallStateChanged(LinphoneCall *call, LinphoneCallState state) {
	switch(state) {
			case LinphoneCallStreamsRunning:
			case LinphoneCallPaused:
				if(m_state == LinphoneConferenceReady) {
					m_pendingCalls = ms_list_remove(m_pendingCalls, call);
					m_transferingCalls = ms_list_append(m_transferingCalls, call);
					linphone_core_transfer_call(m_core, call, m_focusContact);
				}
				break;
				
			case LinphoneCallError:
			case LinphoneCallEnd:
				m_pendingCalls = ms_list_remove(m_pendingCalls, call);
				Conference::removeParticipant(call);
				break;
				
			default: break;
		}
}

void RemoteConference::onTransferingCallStateChanged(LinphoneCall* transfered, LinphoneCallState newCallState) {
	switch (newCallState) {
		case LinphoneCallConnected:
		case LinphoneCallError:
			m_transferingCalls = ms_list_remove(m_transferingCalls, transfered);
			if(newCallState == LinphoneCallError) Conference::removeParticipant(transfered);
			break;

		default:
			break;
	}
}

void RemoteConference::callStateChangedCb(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message) {
	LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(lc);
	RemoteConference *conf = (RemoteConference *)linphone_core_v_table_get_user_data(vtable);
	if (call == conf->m_focusCall) {
		conf->onFocusCallSateChanged(cstate);
	} else if(ms_list_find(conf->m_pendingCalls, call)) {
		conf->onPendingCallStateChanged(call, cstate);
	}
}

void RemoteConference::transferStateChanged(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state) {
	LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(lc);
	RemoteConference *conf = (RemoteConference *)linphone_core_v_table_get_user_data(vtable);
	if (ms_list_find(conf->m_transferingCalls, transfered)) {
		conf->onTransferingCallStateChanged(transfered, new_call_state);
	}
}



const char *linphone_conference_state_to_string(LinphoneConferenceState state) {
	return Conference::stateToString(state);
}

LinphoneConferenceParams *linphone_conference_params_new(const LinphoneCore *core) {
	return (LinphoneConferenceParams *)new Conference::Params(core);
}

void linphone_conference_params_free(LinphoneConferenceParams *params) {
	delete (Conference::Params *)params;
}

LinphoneConferenceParams *linphone_conference_params_clone(const LinphoneConferenceParams *params) {
	return (LinphoneConferenceParams *)new Conference::Params(*(Conference::Params *)params);
}

void linphone_conference_params_enable_video(LinphoneConferenceParams *params, bool_t enable) {
	((Conference::Params *)params)->enableVideo(enable);
}

bool_t linphone_conference_params_video_requested(const LinphoneConferenceParams *params) {
	return ((Conference::Params *)params)->videoRequested();
}

void linphone_conference_params_set_state_changed_callback(LinphoneConferenceParams *params, LinphoneConferenceStateChangedCb cb, void *user_data) {
	((Conference::Params *)params)->setStateChangedCallback(cb, user_data);
}



LinphoneConference *linphone_local_conference_new(LinphoneCore *core) {
	return (LinphoneConference *) new LocalConference(core);
}

LinphoneConference *linphone_local_conference_new_with_params(LinphoneCore *core, const LinphoneConferenceParams *params) {
	return (LinphoneConference *) new LocalConference(core, (Conference::Params *)params);
}

LinphoneConference *linphone_remote_conference_new(LinphoneCore *core) {
	return (LinphoneConference *) new RemoteConference(core);
}

LinphoneConference *linphone_remote_conference_new_with_params(LinphoneCore *core, const LinphoneConferenceParams *params) {
	return (LinphoneConference *) new RemoteConference(core, (Conference::Params *)params);
}

void linphone_conference_free(LinphoneConference *obj) {
	delete (Conference *)obj;
}

LinphoneConferenceState linphone_conference_get_state(const LinphoneConference *obj) {
	return ((Conference *)obj)->getState();
}

int linphone_conference_add_participant(LinphoneConference *obj, LinphoneCall *call) {
	return ((Conference *)obj)->addParticipant(call);
}

int linphone_conference_remove_participant(LinphoneConference *obj, const LinphoneAddress *uri) {
	return ((Conference *)obj)->removeParticipant(uri);
}

int linphone_conference_remove_participant_with_call(LinphoneConference *obj, LinphoneCall *call) {
	return ((Conference *)obj)->removeParticipant(call);
}

int linphone_conference_terminate(LinphoneConference *obj) {
	return ((Conference *)obj)->terminate();
}

int linphone_conference_enter(LinphoneConference *obj) {
	return ((Conference *)obj)->enter();
}

int linphone_conference_leave(LinphoneConference *obj) {
	return ((Conference *)obj)->leave();
}

bool_t linphone_conference_is_in(const LinphoneConference *obj) {
	return ((Conference *)obj)->isIn();
}

AudioStream *linphone_conference_get_audio_stream(const LinphoneConference *obj) {
	return ((Conference *)obj)->getAudioStream();
}

int linphone_conference_mute_microphone(LinphoneConference *obj, bool_t val) {
	return ((Conference *)obj)->muteMicrophone(val);
}

bool_t linphone_conference_microphone_is_muted(const LinphoneConference *obj) {
	return ((Conference *)obj)->microphoneIsMuted();
}

float linphone_conference_get_input_volume(const LinphoneConference *obj) {
	return ((Conference *)obj)->getInputVolume();
}

int linphone_conference_get_size(const LinphoneConference *obj) {
	return ((Conference *)obj)->getSize();
}

MSList *linphone_conference_get_participants(const LinphoneConference *obj) {
	const list<Conference::Participant> &participants = ((Conference *)obj)->getParticipants();
	MSList *participants_list = NULL;
	for(list<Conference::Participant>::const_iterator it=participants.begin();it!=participants.end();it++) {
		LinphoneAddress *uri = linphone_address_clone(it->getUri());
		participants_list = ms_list_append(participants_list, uri);
	}
	return participants_list;
}

int linphone_conference_start_recording(LinphoneConference *obj, const char *path) {
	return ((Conference *)obj)->startRecording(path);
}

int linphone_conference_stop_recording(LinphoneConference *obj) {
	return ((Conference *)obj)->stopRecording();
}

void linphone_conference_on_call_stream_starting(LinphoneConference *obj, LinphoneCall *call, bool_t is_paused_by_remote) {
	((Conference *)obj)->onCallStreamStarting(call, is_paused_by_remote);
}

void linphone_conference_on_call_stream_stopping(LinphoneConference *obj, LinphoneCall *call) {
	((Conference *)obj)->onCallStreamStopping(call);
}

void linphone_conference_on_call_terminating(LinphoneConference *obj, LinphoneCall *call) {
	((Conference *)obj)->onCallTerminating(call);
}

bool_t linphone_conference_check_class(LinphoneConference *obj, LinphoneConferenceClass _class) {
	switch(_class) {
		case LinphoneConferenceClassLocal: return typeid(obj) == typeid(LocalConference);
		case LinphoneConferenceClassRemote: return typeid(obj) == typeid(RemoteConference);
		default: return FALSE;
	}
}
