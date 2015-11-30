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
#include "conference.h"
#include <mediastreamer2/msvolume.h>

namespace Linphone {
class Conference {
public:
	enum Method {
		Unknown = 0,
		Media = 1,
		Transport = 2
	};
	
	Conference(LinphoneCore *core, Method method);
	virtual ~Conference() {};
	
	virtual int addCall(LinphoneCall *call) = 0;
	virtual int removeCall(LinphoneCall *call) = 0;
	virtual int terminate() = 0;
	
	virtual int addLocalParticipant() = 0;
	virtual int removeLocalParticipant() = 0;
	virtual bool localParticipantIsIn() const = 0;
	virtual AudioStream *getAudioStream() const = 0;
	
	virtual int muteMicrophone(bool val) = 0;
	virtual bool microphoneIsMuted() const = 0;
	virtual float getInputVolume() const = 0;
	virtual int getParticipantCount() const = 0;
	Method getMethod() const {return m_method;}
	
	virtual int startRecording(const char *path) = 0;
	virtual int stopRecording() = 0;
	
	virtual void onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote) {};
	virtual void onCallStreamStopping(LinphoneCall *call) {};
	virtual void onCallTerminating(LinphoneCall *call) {};
	
	static Conference *make(LinphoneCore *core, Conference::Method method);
	
protected:
	LinphoneCore *m_core;
	Method m_method;
};
};

using namespace Linphone;

Conference::Conference(LinphoneCore *core, Conference::Method type) :
	m_core(core),
	m_method(type) {
}


namespace Linphone {
class MediaConference: public Conference {
public:
	MediaConference(LinphoneCore *core);
	virtual ~MediaConference();
	
	virtual int addCall(LinphoneCall *call);
	virtual int addAllCalls(LinphoneCall &call);
	virtual int removeCall(LinphoneCall *call);
	virtual int terminate();
	
	virtual int addLocalParticipant();
	virtual int removeLocalParticipant();
	virtual bool localParticipantIsIn() const {return m_localParticipant!=NULL;}
	virtual AudioStream *getAudioStream() const {return m_localParticipant;}
	
	virtual int muteMicrophone(bool val);
	virtual bool microphoneIsMuted() const {return m_localMuted;}
	virtual float getInputVolume() const;
	virtual int getParticipantCount() const;
	
	virtual int startRecording(const char *path);
	virtual int stopRecording();
	
	void onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote);
	void onCallStreamStopping(LinphoneCall *call);
	void onCallTerminating(LinphoneCall *call);
	
private:
	void addLocalEndpoint();
	int remoteParticipantsCount();
	void removeLocalEndpoint();
	int removeFromConference(LinphoneCall *call, bool_t active);
	int convertConferenceToCall();
	static RtpProfile *sMakeDummyProfile(int samplerate);

	MSAudioConference *m_conf;
	AudioStream *m_localParticipant;
	MSAudioEndpoint *m_localEndpoint;
	MSAudioEndpoint *m_recordEndpoint;
	RtpProfile *m_localDummyProfile;
	bool_t m_localMuted;
	bool_t m_terminated;
};
};

MediaConference::MediaConference(LinphoneCore *core): Conference(core, Media),
	m_conf(NULL),
	m_localParticipant(NULL),
	m_localEndpoint(NULL),
	m_recordEndpoint(NULL),
	m_localDummyProfile(NULL),
	m_localMuted(FALSE),
	m_terminated(FALSE) {
	MSAudioConferenceParams params;
	params.samplerate = lp_config_get_int(m_core->config, "sound","conference_rate",16000);
	m_conf=ms_audio_conference_new(&params);
}

MediaConference::~MediaConference() {
	if(m_conf) terminate();
}

RtpProfile *MediaConference::sMakeDummyProfile(int samplerate){
	RtpProfile *prof=rtp_profile_new("dummy");
	PayloadType *pt=payload_type_clone(&payload_type_l16_mono);
	pt->clock_rate=samplerate;
	rtp_profile_set_payload(prof,0,pt);
	return prof;
}

void MediaConference::addLocalEndpoint() {
	/*create a dummy audiostream in order to extract the local part of it */
	/* network address and ports have no meaning and are not used here. */
	AudioStream *st=audio_stream_new(65000,65001,FALSE);
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
	m_localParticipant=st;
	m_localEndpoint=ms_audio_endpoint_get_from_stream(st,FALSE);
	ms_audio_conference_add_member(m_conf,m_localEndpoint);
}

int MediaConference::addCall(LinphoneCall *call) {
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
	return 0;
}

int MediaConference::addAllCalls(LinphoneCall &call) {
	MSList *calls=m_core->calls;
	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (!call->current_params->in_conference) addCall(call);
	}
	addLocalParticipant();
	return 0;
}

int MediaConference::removeFromConference(LinphoneCall *call, bool_t active){
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
		if (localParticipantIsIn()){
			ms_message("Leaving conference for reconnecting with unique call.");
			removeLocalParticipant();
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

int MediaConference::remoteParticipantsCount() {
	int count=getParticipantCount();
	if (count==0) return 0;
	if (!m_localParticipant) return count;
	return count -1;
}

int MediaConference::convertConferenceToCall(){
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
			bool_t active_after_removed=localParticipantIsIn();
			err=removeFromConference(rc, active_after_removed);
			break;
		}
	}
	return err;
}

int MediaConference::removeCall(LinphoneCall *call) {
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

int MediaConference::terminate() {
	MSList *calls=m_core->calls;
	m_terminated=TRUE;

	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (call->current_params->in_conference) {
			linphone_core_terminate_call(m_core, call);
		}
	}
	return 0;
}

int MediaConference::addLocalParticipant() {
	if (linphone_core_sound_resources_locked(m_core)) {
		return -1;
	}
	if (m_core->current_call != NULL) {
		_linphone_core_pause_call(m_core, m_core->current_call);
	}
	if (m_localParticipant==NULL) addLocalEndpoint();
	return 0;
}

void MediaConference::removeLocalEndpoint(){
	if (m_localEndpoint){
		ms_audio_conference_remove_member(m_conf,m_localEndpoint);
		ms_audio_endpoint_release_from_stream(m_localEndpoint);
		m_localEndpoint=NULL;
		audio_stream_stop(m_localParticipant);
		m_localParticipant=NULL;
		rtp_profile_destroy(m_localDummyProfile);
	}
}

int MediaConference::removeLocalParticipant() {
	if (localParticipantIsIn())
		removeLocalEndpoint();
	return 0;
}

int MediaConference::muteMicrophone(bool val)  {
	if (val) {
		audio_stream_set_mic_gain(m_localParticipant, 0);
	} else {
		audio_stream_set_mic_gain_db(m_localParticipant, m_core->sound_conf.soft_mic_lev);
	}
	if ( linphone_core_get_rtp_no_xmit_on_audio_mute(m_core) ){
		audio_stream_mute_rtp(m_localParticipant, val);
	}
	m_localMuted=val;
	return 0;
}

float MediaConference::getInputVolume() const {
	AudioStream *st=m_localParticipant;
	if (st && st->volsend && !m_localMuted){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET,&vol);
		return vol;

	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

int MediaConference::getParticipantCount() const {
	if (m_conf == NULL) {
		return 0;
	}
	return ms_audio_conference_get_size(m_conf) - (m_recordEndpoint ? 1 : 0);
}

int MediaConference::startRecording(const char *path) {
	if (m_conf == NULL) {
		ms_warning("linphone_core_start_conference_recording(): no conference now.");
		return -1;
	}
	if (m_recordEndpoint==NULL){
		m_recordEndpoint=ms_audio_endpoint_new_recorder();
		ms_audio_conference_add_member(m_conf,m_recordEndpoint);
	}
	ms_audio_recorder_endpoint_start(m_recordEndpoint,path);
	return 0;
}

int MediaConference::stopRecording() {
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

Conference *Conference::make(LinphoneCore *core, Conference::Method type) {
	switch(type) {
		case Media:
			return new MediaConference(core);
		default:
			return NULL;
	}
}

void MediaConference::onCallStreamStarting(LinphoneCall *call, bool isPausedByRemote) {
	call->params->has_video = FALSE;
	call->camera_enabled = FALSE;
	MSAudioEndpoint *ep=ms_audio_endpoint_get_from_stream(call->audiostream,TRUE);
	ms_audio_conference_add_member(m_conf,ep);
	ms_audio_conference_mute_member(m_conf,ep,isPausedByRemote);
	call->endpoint=ep;
}

void MediaConference::onCallStreamStopping(LinphoneCall *call) {
	ms_audio_conference_remove_member(m_conf,call->endpoint);
	ms_audio_endpoint_release_from_stream(call->endpoint);
	call->endpoint=NULL;
}

void MediaConference::onCallTerminating(LinphoneCall *call) {
	int remote_count=remoteParticipantsCount();
	ms_message("conference_check_uninit(): size=%i", getParticipantCount());
	if (remote_count==1 && !m_terminated){
		convertConferenceToCall();
	}
	if (remote_count==0){
		if (m_localParticipant)
			removeLocalEndpoint();
		if (m_recordEndpoint){
			ms_audio_conference_remove_member(m_conf, m_recordEndpoint);
			ms_audio_endpoint_destroy(m_recordEndpoint);
		}
	}
	if (ms_audio_conference_get_size(m_conf)==0){
		ms_audio_conference_destroy(m_conf);
		m_core->conf_ctx = NULL;
		delete this;
	}
}

LinphoneConference *linphone_conference_make(LinphoneCore *core, LinphoneConferenceType type) {
	return Conference::make(core, (Conference::Method)type);
}

void linphone_conference_free(LinphoneConference *obj) {
	delete (Conference *)obj;
}

int linphone_conference_add_call(LinphoneConference *obj, LinphoneCall *call) {
	return ((Conference *)obj)->addCall(call);
}

int linphone_conference_remove_call(LinphoneConference *obj, LinphoneCall *call) {
	return ((Conference *)obj)->removeCall(call);
}

int linphone_conference_terminate(LinphoneConference *obj) {
	return ((Conference *)obj)->terminate();
}

int linphone_conference_add_local_participant(LinphoneConference *obj) {
	return ((Conference *)obj)->addLocalParticipant();
}

int linphone_conference_remove_local_participant(LinphoneConference *obj) {
	return ((Conference *)obj)->removeLocalParticipant();
}

bool_t linphone_conference_local_participant_is_in(const LinphoneConference *obj) {
	return ((Conference *)obj)->localParticipantIsIn() ? TRUE : FALSE;
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

int linphone_conference_get_participant_count(const LinphoneConference *obj) {
	return ((Conference *)obj)->getParticipantCount();
}

LinphoneConferenceType linphone_conference_get_method(const LinphoneConference *obj) {
	return (LinphoneConferenceType)((Conference *)obj)->getMethod();
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
