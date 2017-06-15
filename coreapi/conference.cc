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

#include "linphone/core.h"
#include "private.h"
#include "conference_private.h"
#include <mediastreamer2/msvolume.h>
#include <typeinfo>
#include <list>
#include <algorithm>

namespace Linphone {

template <typename _type>
inline std::list<_type> toStd(const bctbx_list_t *l){
	std::list<_type> ret;
	for(; l != NULL; l = l->next){
		ret.push_back(static_cast<_type>(l->data));
	}
	return ret;
}
	
class Conference {
public:
	class Participant {
	public:
		Participant(LinphoneCall *call) {
			m_uri = linphone_address_clone(linphone_call_get_remote_address(call));
			m_call = call;
		}
		
		~Participant() {
			linphone_address_unref(m_uri);
			if(m_call) m_call->conf_ref = NULL;
			
		}
		
		const LinphoneAddress *getUri() const {
			return m_uri;
		}
		
		LinphoneCall *getCall() const {
			return m_call;
		}
		
	private:
		Participant(const Participant &src);
		Participant &operator=(const Participant &src);
		
	private:
		LinphoneAddress *m_uri;
		LinphoneCall *m_call;
		
	friend class RemoteConference;
	};
	
	
	class Params {
	public:
		Params(const LinphoneCore *core = NULL) {
			m_enableVideo = false;
			if(core) {
				const LinphoneVideoPolicy *policy = linphone_core_get_video_policy(core);
				if(policy->automatically_initiate) m_enableVideo = true;
			}
			m_stateChangedCb = NULL;
			m_userData = NULL;
		}
		void enableVideo(bool enable) {m_enableVideo = enable;}
		bool videoRequested() const {return m_enableVideo;}
		void setStateChangedCallback(LinphoneConferenceStateChangedCb cb, void *userData) {
			m_stateChangedCb = cb;
			m_userData = userData;
		}
		
	private:
		bool m_enableVideo;
		LinphoneConferenceStateChangedCb m_stateChangedCb;
		void *m_userData;
		
		friend class Conference;
	};
	
	Conference(LinphoneCore *core, LinphoneConference *conf, const Params *params = NULL);
	virtual ~Conference() {};
	
	const Params &getCurrentParams() const {return m_currentParams;}
	
	virtual int inviteAddresses(const std::list<const LinphoneAddress*> &addresses, const LinphoneCallParams *params) = 0;
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
	
	virtual int getSize() const {return (int)m_participants.size() + (isIn()?1:0);}
	const std::list<Participant *> &getParticipants() const {return m_participants;}
	
	virtual int startRecording(const char *path) = 0;
	virtual int stopRecording() = 0;
	
	virtual void onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote) {};
	virtual void onCallStreamStopping(LinphoneCall *call) {};
	virtual void onCallTerminating(LinphoneCall *call) {};
	
	LinphoneConferenceState getState() const {return m_state;}
	LinphoneCore *getCore()const{
		return m_core;
	}
	static const char *stateToString(LinphoneConferenceState state);
	
protected:
	void setState(LinphoneConferenceState state);
	Participant *findParticipant(const LinphoneCall *call) const;
	Participant *findParticipant(const LinphoneAddress *uri) const;
	
protected:
	LinphoneCore *m_core;
	AudioStream *m_localParticipantStream;
	bool m_isMuted;
	std::list<Participant *> m_participants;
	Params m_currentParams;
	LinphoneConferenceState m_state;
	LinphoneConference *m_conference;
};

class LocalConference: public Conference {
public:
	LocalConference(LinphoneCore *core, LinphoneConference *conf, const Params *params = NULL);
	virtual ~LocalConference();
	
	virtual int inviteAddresses(const std::list<const LinphoneAddress*> &addresses, const LinphoneCallParams *params);
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
	bool_t m_terminating;
};

class RemoteConference: public Conference {
public:
	RemoteConference(LinphoneCore *core, LinphoneConference *conf, const Params *params = NULL);
	virtual ~RemoteConference();
	
	virtual int inviteAddresses(const std::list<const LinphoneAddress*> &addresses, const LinphoneCallParams *params);
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
	LinphoneCoreCbs *m_coreCbs;
	std::list<LinphoneCall *> m_pendingCalls;
	std::list<LinphoneCall *> m_transferingCalls;
};

};


using namespace Linphone;
using namespace std;


Conference::Conference(LinphoneCore *core, LinphoneConference *conf, const Conference::Params *params):
	m_core(core),
	m_localParticipantStream(NULL),
	m_isMuted(false),
	m_currentParams(),
	m_state(LinphoneConferenceStopped),
	m_conference(conf) {
	if(params) m_currentParams = *params;
}

int Conference::addParticipant(LinphoneCall *call) {
	Participant *p =new Participant(call);
	m_participants.push_back(p);
	call->conf_ref = m_conference;
	return 0;
}

int Conference::removeParticipant(LinphoneCall *call) {
	Participant *p = findParticipant(call);
	if(p == NULL) return -1;
	delete p;
	m_participants.remove(p);
	return 0;
}

int Conference::removeParticipant(const LinphoneAddress *uri) {
	Participant *p = findParticipant(uri);
	if(p == NULL) return -1;
	delete p;
	m_participants.remove(p);
	return 0;
}

int Conference::terminate() {
	for(list<Participant *>::iterator it = m_participants.begin(); it!=m_participants.end(); it++) {
		delete *it;
	}
	m_participants.clear();
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
		case LinphoneConferenceRunning: return "Ready";
		case LinphoneConferenceStartingFailed: return "Startig failed";
		default: return "Invalid state";
	}
}



void Conference::setState(LinphoneConferenceState state) {
	if(m_state != state) {
		ms_message("Switching conference [%p] into state '%s'", this, stateToString(state));
		m_state = state;
		if(m_currentParams.m_stateChangedCb) {
			m_currentParams.m_stateChangedCb(m_conference, state, m_currentParams.m_userData);
		}
	}
}

Conference::Participant *Conference::findParticipant(const LinphoneCall *call) const {
	for(list<Participant *>::const_iterator it=m_participants.begin(); it!=m_participants.end(); it++) {
		if((*it)->getCall() == call) return *it;
	}
	return NULL;
}

Conference::Participant *Conference::findParticipant(const LinphoneAddress *uri) const {
	for(list<Participant *>::const_iterator it=m_participants.begin(); it!=m_participants.end(); it++) {
		if(linphone_address_equal((*it)->getUri(), uri)) return *it;
	}
	return NULL;
}




LocalConference::LocalConference(LinphoneCore *core, LinphoneConference *conf, const Conference::Params *params):
	Conference(core, conf, params),
	m_conf(NULL),
	m_localEndpoint(NULL),
	m_recordEndpoint(NULL),
	m_localDummyProfile(NULL),
	m_terminating(FALSE) {

	MSAudioConferenceParams ms_conf_params;
	ms_conf_params.samplerate = lp_config_get_int(m_core->config, "sound","conference_rate",16000);
	m_conf=ms_audio_conference_new(&ms_conf_params, core->factory);
	m_state= LinphoneConferenceRunning;
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
	ms_message("conference: adding local endpoint");
	ms_audio_conference_add_member(m_conf,m_localEndpoint);
}

int LocalConference::inviteAddresses(const std::list<const LinphoneAddress*> &addresses, const LinphoneCallParams *params){
	
	for (std::list<const LinphoneAddress*>::const_iterator it = addresses.begin(); it != addresses.end(); ++it){
		const LinphoneAddress *addr = *it;
		LinphoneCall * call = linphone_core_get_call_by_remote_address2(m_core, addr);
		if (!call){
			/*start a new call by indicating that it has to be put into the conference directlly*/
			LinphoneCallParams * new_params = params ? linphone_call_params_copy(params) : linphone_core_create_call_params(m_core, NULL);
			LinphoneCall *call;
			/*toggle this flag so the call is immediately added to the conference upon acceptance*/
			new_params->in_conference = TRUE;
			linphone_call_params_enable_video(new_params, FALSE); /*turn off video as it is not supported for conferencing at this time*/
			call = linphone_core_invite_address_with_params(m_core, addr, new_params);
			if (!call){
				ms_error("LocalConference::inviteAddresses(): could not invite participant");
			}
			linphone_call_params_unref(new_params);
		}else{
			/*there is already a call to this address, so simply join it to the local conference if not already done*/
			if (!call->current_params->in_conference)
				addParticipant(call);
		}
		/*if the local participant is not yet created, created it and it to the conference */
		if (!m_localEndpoint) addLocalEndpoint();
	}
	return 0;
}

int LocalConference::addParticipant(LinphoneCall *call) {
	if (call->current_params->in_conference){
		ms_error("Already in conference");
		return -1;
	}

	if (call->state==LinphoneCallPaused){
		call->params->in_conference=TRUE;
		call->params->has_video=FALSE;
		linphone_call_resume(call);
	}else if (call->state==LinphoneCallStreamsRunning){
		LinphoneCallParams *params = linphone_core_create_call_params(m_core, call);
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
		linphone_call_update(call,params);
		linphone_call_params_unref(params);
		addLocalEndpoint();
	}else{
		ms_error("Call is in state %s, it cannot be added to the conference.",linphone_call_state_to_string(call->state));
		return -1;
	}
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
		err=linphone_call_update(call,params);
		linphone_call_params_unref(params);
	} else{
		ms_message("Pausing call to actually remove from conference");
		err=_linphone_call_pause(call);
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
	bctbx_list_t *calls=m_core->calls;

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
	bctbx_list_t *calls=m_core->calls;
	m_terminating =TRUE;

	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (call->current_params->in_conference) {
			linphone_call_terminate(call);
		}
	}
	
	Conference::terminate();
	m_terminating = FALSE;
	
	return 0;
}

int LocalConference::enter() {
	if (linphone_core_sound_resources_locked(m_core)) {
		return -1;
	}
	if (m_core->current_call != NULL) {
		_linphone_call_pause(m_core->current_call);
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
	ms_message("LocalConference::onCallStreamStarting(): joining AudioStream [%p] of call [%p] into conference.", call->audiostream, call);
	MSAudioEndpoint *ep=ms_audio_endpoint_get_from_stream(call->audiostream,TRUE);
	ms_audio_conference_add_member(m_conf,ep);
	ms_audio_conference_mute_member(m_conf,ep,isPausedByRemote);
	call->endpoint=ep;
	setState(LinphoneConferenceRunning);
	Conference::addParticipant(call);
}

void LocalConference::onCallStreamStopping(LinphoneCall *call) {
	ms_audio_conference_remove_member(m_conf,call->endpoint);
	ms_audio_endpoint_release_from_stream(call->endpoint);
	call->endpoint=NULL;
	Conference::removeParticipant(call);
}

void LocalConference::onCallTerminating(LinphoneCall *call) {
	int remote_count=remoteParticipantsCount();
	ms_message("conference_check_uninit(): size=%i", getSize());
	if (remote_count==1 && !m_terminating){
		convertConferenceToCall();
	}
	if (remote_count==0){
		if (m_localParticipantStream)
			removeLocalEndpoint();
		if (m_recordEndpoint){
			ms_audio_conference_remove_member(m_conf, m_recordEndpoint);
			ms_audio_endpoint_destroy(m_recordEndpoint);
		}
		setState(LinphoneConferenceStopped);
	}
}



RemoteConference::RemoteConference(LinphoneCore *core, LinphoneConference *conf, const Conference::Params *params): Conference(core, conf, params) {
	m_focusAddr = NULL;
	m_focusContact = NULL;
	m_focusCall = NULL;
	m_coreCbs = NULL;
	m_focusAddr = lp_config_get_string(m_core->config, "misc", "conference_focus_addr", "");
	m_coreCbs = linphone_factory_create_core_cbs(linphone_factory_get());
	linphone_core_cbs_set_call_state_changed(m_coreCbs, callStateChangedCb);
	linphone_core_cbs_set_transfer_state_changed(m_coreCbs, transferStateChanged);
	linphone_core_cbs_set_user_data(m_coreCbs, this);
	_linphone_core_add_callbacks(m_core, m_coreCbs, TRUE);
}

RemoteConference::~RemoteConference() {
	terminate();
	linphone_core_remove_callbacks(m_core, m_coreCbs);
	linphone_core_cbs_unref(m_coreCbs);
}

int RemoteConference::inviteAddresses(const std::list<const LinphoneAddress *> &addresses, const LinphoneCallParams *params){
	ms_error("RemoteConference::inviteAddresses() not implemented");
	return -1;
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
				m_localParticipantStream = m_focusCall->audiostream;
				m_pendingCalls.push_back(call);
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
				m_pendingCalls.push_back(call);
			}
			return 0;
			
		case LinphoneConferenceRunning:
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
		case LinphoneConferenceRunning:
			if(findParticipant(uri) == NULL) {
				char *tmp = linphone_address_as_string(uri);
				ms_error("Conference: could not remove participant '%s': not in the participants list", tmp);
				ms_free(tmp);
				return -1;
			}
			
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
	switch(m_state) {
		case LinphoneConferenceRunning:
		case LinphoneConferenceStarting:
			linphone_call_terminate(m_focusCall);
			break;
			
		default: break;
	}
	return 0;
}

int RemoteConference::enter() {
	if(m_state != LinphoneConferenceRunning) {
		ms_error("Could not enter in the conference: bad conference state (%s)", stateToString(m_state));
		return -1;
	}
	LinphoneCallState callState = linphone_call_get_state(m_focusCall);
	switch(callState) {
		case LinphoneCallStreamsRunning: break;
		case LinphoneCallPaused:
			linphone_call_resume(m_focusCall);
			break;
		default:
			ms_error("Could not join the conference: bad focus call state (%s)", linphone_call_state_to_string(callState));
			return -1;
	}
	return 0;
}

int RemoteConference::leave() {
	if(m_state != LinphoneConferenceRunning) {
		ms_error("Could not leave the conference: bad conference state (%s)", stateToString(m_state));
		return -1;
	}
	LinphoneCallState callState = linphone_call_get_state(m_focusCall);
	switch(callState) {
		case LinphoneCallPaused: break;
		case LinphoneCallStreamsRunning:
			linphone_call_pause(m_focusCall);
			break;
		default:
			ms_error("Could not leave the conference: bad focus call state (%s)", linphone_call_state_to_string(callState));
			return -1;
	}
	return 0;
}

bool RemoteConference::isIn() const {
	if(m_state != LinphoneConferenceRunning) return false;
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
	if(linphone_call_transfer(call, m_focusContact) == 0) {
		m_transferingCalls.push_back(call);
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
	m_pendingCalls.clear();
	m_transferingCalls.clear();
}

void RemoteConference::onFocusCallSateChanged(LinphoneCallState state) {
	list<LinphoneCall *>::iterator it;
	
	switch (state) {
		case LinphoneCallConnected:
			m_focusContact = ms_strdup(linphone_call_get_remote_contact(m_focusCall));
			it = m_pendingCalls.begin();
			while (it != m_pendingCalls.end()) {
				LinphoneCall *pendingCall = *it;
				LinphoneCallState pendingCallState = linphone_call_get_state(pendingCall);
				if(pendingCallState == LinphoneCallStreamsRunning || pendingCallState == LinphoneCallPaused) {
					it = m_pendingCalls.erase(it);
					transferToFocus(pendingCall);
				} else {
					it++;
				}
			}
			setState(LinphoneConferenceRunning);
			break;

		case LinphoneCallError:
			reset();
			Conference::terminate();
			setState(LinphoneConferenceStartingFailed);
			break;
			
		case LinphoneCallEnd:
			reset();
			Conference::terminate();
			setState(LinphoneConferenceStopped);
			break;

		default: break;
	}
}

void RemoteConference::onPendingCallStateChanged(LinphoneCall *call, LinphoneCallState state) {
	switch(state) {
			case LinphoneCallStreamsRunning:
			case LinphoneCallPaused:
				if(m_state == LinphoneConferenceRunning) {
					m_pendingCalls.remove(call);
					m_transferingCalls.push_back(call);
					linphone_call_transfer(call, m_focusContact);
				}
				break;
				
			case LinphoneCallError:
			case LinphoneCallEnd:
				m_pendingCalls.remove(call);
				Conference::removeParticipant(call);
				if(m_participants.size() + m_pendingCalls.size() + m_transferingCalls.size() == 0) {
					terminate();
				}
				break;
				
			default: break;
		}
}

void RemoteConference::onTransferingCallStateChanged(LinphoneCall* transfered, LinphoneCallState newCallState) {
	switch (newCallState) {
		case LinphoneCallConnected:
			m_transferingCalls.push_back(transfered);
			findParticipant(transfered)->m_call = NULL;
			break;
			
		case LinphoneCallError:
			m_transferingCalls.remove(transfered);
			Conference::removeParticipant(transfered);
			if(m_participants.size() + m_pendingCalls.size() + m_transferingCalls.size() == 0) {
				terminate();
			}
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
	} else {
		list<LinphoneCall *>::iterator it = find(conf->m_pendingCalls.begin(), conf->m_pendingCalls.end(), call);
		if(it != conf->m_pendingCalls.end()) {
			conf->onPendingCallStateChanged(call, cstate);
		}
	}
}

void RemoteConference::transferStateChanged(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state) {
	LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(lc);
	RemoteConference *conf = (RemoteConference *)linphone_core_v_table_get_user_data(vtable);
	list<LinphoneCall *>::iterator it = find(conf->m_transferingCalls.begin(), conf->m_transferingCalls.end(), transfered);
	if (it != conf->m_transferingCalls.end()) {
		conf->onTransferingCallStateChanged(transfered, new_call_state);
	}
}



const char *linphone_conference_state_to_string(LinphoneConferenceState state) {
	return Conference::stateToString(state);
}


struct _LinphoneConferenceParams {
	::belle_sip_object_t base;
	Conference::Params *params;
};

static void _linphone_conference_params_uninit(LinphoneConferenceParams *params);
static void _linphone_conference_params_clone(LinphoneConferenceParams *params, const LinphoneConferenceParams *orig);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneConferenceParams);
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneConferenceParams);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneConferenceParams, belle_sip_object_t,
	_linphone_conference_params_uninit, // uninit
	_linphone_conference_params_clone, // clone
	NULL, // marshal
	FALSE // unown
);

LinphoneConferenceParams *linphone_conference_params_new(const LinphoneCore *core) {
	LinphoneConferenceParams *obj = belle_sip_object_new(LinphoneConferenceParams);
	obj->params = new Conference::Params(core);
	return obj;
}

static void _linphone_conference_params_uninit(LinphoneConferenceParams *params) {
	delete params->params;
}

LinphoneConferenceParams *linphone_conference_params_ref(LinphoneConferenceParams *params) {
	return (LinphoneConferenceParams *)belle_sip_object_ref(params);
}

void linphone_conference_params_unref(LinphoneConferenceParams *params) {
	belle_sip_object_unref(params);
}

void linphone_conference_params_free(LinphoneConferenceParams *params) {
	linphone_conference_params_unref(params);
}

static void _linphone_conference_params_clone(LinphoneConferenceParams *params, const LinphoneConferenceParams *orig) {
	params->params = new Conference::Params(*orig->params);
}

LinphoneConferenceParams *linphone_conference_params_clone(const LinphoneConferenceParams *params) {
	return (LinphoneConferenceParams *)belle_sip_object_clone((const belle_sip_object_t *)params);
}

void linphone_conference_params_enable_video(LinphoneConferenceParams *params, bool_t enable) {
	params->params->enableVideo(enable ? true : false);
}

bool_t linphone_conference_params_video_requested(const LinphoneConferenceParams *params) {
	return params->params->videoRequested() ? TRUE : FALSE;
}

void linphone_conference_params_set_state_changed_callback(LinphoneConferenceParams *params, LinphoneConferenceStateChangedCb cb, void *user_data) {
	params->params->setStateChangedCallback(cb, user_data);
}


struct _LinphoneConference {
	belle_sip_object_t base;
	Conference *conf;
};

static void _linphone_conference_uninit(LinphoneConference *conf);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneConference);
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneConference);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneConference, belle_sip_object_t,
	_linphone_conference_uninit, // uninit
	NULL, // clone
	NULL, // marshal
	FALSE // unown
);

LinphoneConference *linphone_local_conference_new(LinphoneCore *core) {
	LinphoneConference *conf = belle_sip_object_new(LinphoneConference);
	conf->conf = new LocalConference(core, conf);
	return conf;
}

LinphoneConference *linphone_local_conference_new_with_params(LinphoneCore *core, const LinphoneConferenceParams *params) {
	LinphoneConference *conf = belle_sip_object_new(LinphoneConference);
	conf->conf = new LocalConference(core, conf, params->params);
	return conf;
}

LinphoneConference *linphone_remote_conference_new(LinphoneCore *core) {
	LinphoneConference *conf = belle_sip_object_new(LinphoneConference);
	conf->conf = new RemoteConference(core, conf);
	return conf;
}

LinphoneConference *linphone_remote_conference_new_with_params(LinphoneCore *core, const LinphoneConferenceParams *params) {
	LinphoneConference *conf = belle_sip_object_new(LinphoneConference);
	conf->conf = new RemoteConference(core, conf, params->params);
	return conf;
}

static void _linphone_conference_uninit(LinphoneConference *conf) {
	delete conf->conf;
}

LinphoneConference *linphone_conference_ref(LinphoneConference *conf) {
	return (LinphoneConference *)belle_sip_object_ref(conf);
}

void linphone_conference_unref(LinphoneConference *conf) {
	belle_sip_object_unref(conf);
}

LinphoneConferenceState linphone_conference_get_state(const LinphoneConference *obj) {
	return obj->conf->getState();
}

int linphone_conference_add_participant(LinphoneConference *obj, LinphoneCall *call) {
	return obj->conf->addParticipant(call);
}

LinphoneStatus linphone_conference_remove_participant(LinphoneConference *obj, const LinphoneAddress *uri) {
	return obj->conf->removeParticipant(uri);
}

int linphone_conference_remove_participant_with_call(LinphoneConference *obj, LinphoneCall *call) {
	return obj->conf->removeParticipant(call);
}

int linphone_conference_terminate(LinphoneConference *obj) {
	return obj->conf->terminate();
}

int linphone_conference_enter(LinphoneConference *obj) {
	return obj->conf->enter();
}

int linphone_conference_leave(LinphoneConference *obj) {
	return obj->conf->leave();
}

bool_t linphone_conference_is_in(const LinphoneConference *obj) {
	return obj->conf->isIn();
}

AudioStream *linphone_conference_get_audio_stream(const LinphoneConference *obj) {
	return obj->conf->getAudioStream();
}

int linphone_conference_mute_microphone(LinphoneConference *obj, bool_t val) {
	return obj->conf->muteMicrophone(val ? true : false);
}

bool_t linphone_conference_microphone_is_muted(const LinphoneConference *obj) {
	return obj->conf->microphoneIsMuted() ? TRUE : FALSE;
}

float linphone_conference_get_input_volume(const LinphoneConference *obj) {
	return obj->conf->getInputVolume();
}

int linphone_conference_get_size(const LinphoneConference *obj) {
	return obj->conf->getSize();
}

bctbx_list_t *linphone_conference_get_participants(const LinphoneConference *obj) {
	const list<Conference::Participant *> &participants = obj->conf->getParticipants();
	bctbx_list_t *participants_list = NULL;
	for(list<Conference::Participant *>::const_iterator it=participants.begin();it!=participants.end();it++) {
		LinphoneAddress *uri = linphone_address_clone((*it)->getUri());
		participants_list = bctbx_list_append(participants_list, uri);
	}
	return participants_list;
}

int linphone_conference_start_recording(LinphoneConference *obj, const char *path) {
	return obj->conf->startRecording(path);
}

int linphone_conference_stop_recording(LinphoneConference *obj) {
	return obj->conf->stopRecording();
}

void linphone_conference_on_call_stream_starting(LinphoneConference *obj, LinphoneCall *call, bool_t is_paused_by_remote) {
	obj->conf->onCallStreamStarting(call, is_paused_by_remote ? true : false);
}

void linphone_conference_on_call_stream_stopping(LinphoneConference *obj, LinphoneCall *call) {
	obj->conf->onCallStreamStopping(call);
}

void linphone_conference_on_call_terminating(LinphoneConference *obj, LinphoneCall *call) {
	obj->conf->onCallTerminating(call);
}

bool_t linphone_conference_check_class(LinphoneConference *obj, LinphoneConferenceClass _class) {
	switch(_class) {
		case LinphoneConferenceClassLocal: return typeid(obj->conf) == typeid(LocalConference);
		case LinphoneConferenceClassRemote: return typeid(obj->conf) == typeid(RemoteConference);
		default: return FALSE;
	}
}

LinphoneStatus linphone_conference_invite_participants(LinphoneConference *obj, const bctbx_list_t *addresses, const LinphoneCallParams *params){
	return obj->conf->inviteAddresses(toStd<const LinphoneAddress*>(addresses), params);
}
