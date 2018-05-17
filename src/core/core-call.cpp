/*
 * core-call.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <algorithm>
#include <math.h>

#include "core-p.h"
#include "call/call-p.h"
#include "conference/session/call-session-p.h"
#include "logger/logger.h"

// TODO: Remove me later.
#include "c-wrapper/c-wrapper.h"
#include "conference_private.h"

#include <mediastreamer2/msvolume.h>

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

int CorePrivate::addCall (const shared_ptr<Call> &call) {
	L_Q();
	L_ASSERT(call);
	if (!canWeAddCall())
		return -1;
	if (!hasCalls())
		notifySoundcardUsage(true);
	calls.push_back(call);
	linphone_core_notify_call_created(q->getCCore(), L_GET_C_BACK_PTR(call));
	return 0;
}

bool CorePrivate::canWeAddCall () const {
	L_Q();
	if (q->getCallCount() < static_cast<unsigned int>(q->getCCore()->max_calls))
		return true;
	lInfo() << "Maximum amount of simultaneous calls reached!";
	return false;
}

bool CorePrivate::inviteReplacesABrokenCall (SalCallOp *op) {
	CallSession *replacedSession = nullptr;
	SalCallOp *replacedOp = op->getReplaces();
	if (replacedOp)
		replacedSession = reinterpret_cast<CallSession *>(replacedOp->getUserPointer());
	for (const auto &call : calls) {
		shared_ptr<CallSession> session = call->getPrivate()->getActiveSession();
		if (session
			&& ((session->getPrivate()->isBroken() && op->compareOp(session->getPrivate()->getOp()))
				|| (replacedSession == session.get() && op->getFrom() == replacedOp->getFrom() && op->getTo() == replacedOp->getTo())
		)) {
			session->getPrivate()->replaceOp(op);
			return true;
		}
	}

	return false;
}

bool CorePrivate::isAlreadyInCallWithAddress (const Address &addr) const {
	for (const auto &call : calls) {
		if (call->getRemoteAddress().weakEqual(addr))
			return true;
	}
	return false;
}

void CorePrivate::iterateCalls (time_t currentRealTime, bool oneSecondElapsed) const {
	// Make a copy of the list af calls because it may be altered during calls to the Call::iterate method
	list<shared_ptr<Call>> savedCalls(calls);
	for (const auto &call : savedCalls) {
		call->getPrivate()->iterate(currentRealTime, oneSecondElapsed);
	}
}

void CorePrivate::notifySoundcardUsage (bool used) {
	L_Q();
	MSSndCard *card = q->getCCore()->sound_conf.capt_sndcard;
	if (card && (ms_snd_card_get_capabilities(card) & MS_SND_CARD_CAP_IS_SLOW))
		ms_snd_card_set_usage_hint(card, used);
}

int CorePrivate::removeCall (const shared_ptr<Call> &call) {
	L_ASSERT(call);
	auto iter = find(calls.begin(), calls.end(), call);
	if (iter == calls.end()) {
		lWarning() << "Could not find the call to remove";
		return -1;
	}
	calls.erase(iter);
	return 0;
}

void CorePrivate::unsetVideoWindowId (bool preview, void *id) {
#ifdef VIDEO_ENABLED
	for (const auto &call : calls) {
		VideoStream *vstream = reinterpret_cast<VideoStream *>(call->getPrivate()->getMediaStream(LinphoneStreamTypeVideo));
		if (vstream) {
			if (preview)
				video_stream_set_native_preview_window_id(vstream, id);
			else
				video_stream_set_native_window_id(vstream, id);
		}
	}
#endif
}

// -----------------------------------------------------------------------------

void CorePrivate::parameterizeEqualizer (AudioStream *stream) {
	L_Q();
	LinphoneConfig *config = linphone_core_get_config(q->getCCore());
	const char *eqActive = lp_config_get_string(config, "sound", "eq_active", nullptr);
	if (eqActive)
		lWarning() << "'eq_active' linphonerc parameter has no effect anymore. Please use 'mic_eq_active' or 'spk_eq_active' instead";
	const char *eqGains = lp_config_get_string(config, "sound", "eq_gains", nullptr);
	if(eqGains)
		lWarning() << "'eq_gains' linphonerc parameter has no effect anymore. Please use 'mic_eq_gains' or 'spk_eq_gains' instead";
	if (stream->mic_equalizer) {
		MSFilter *f = stream->mic_equalizer;
		bool enabled = !!lp_config_get_int(config, "sound", "mic_eq_active", 0);
		ms_filter_call_method(f, MS_EQUALIZER_SET_ACTIVE, &enabled);
		const char *gains = lp_config_get_string(config, "sound", "mic_eq_gains", nullptr);
		if (enabled && gains) {
			bctbx_list_t *gainsList = ms_parse_equalizer_string(gains);
			for (bctbx_list_t *it = gainsList; it; it = bctbx_list_next(it)) {
				MSEqualizerGain *g = reinterpret_cast<MSEqualizerGain *>(bctbx_list_get_data(it));
				lInfo() << "Read microphone equalizer gains: " << g->frequency << "(~" << g->width << ") --> " << g->gain;
				ms_filter_call_method(f, MS_EQUALIZER_SET_GAIN, g);
			}
			if (gainsList)
				bctbx_list_free_with_data(gainsList, ms_free);
		}
	}
	if (stream->spk_equalizer) {
		MSFilter *f = stream->spk_equalizer;
		bool enabled = !!lp_config_get_int(config, "sound", "spk_eq_active", 0);
		ms_filter_call_method(f, MS_EQUALIZER_SET_ACTIVE, &enabled);
		const char *gains = lp_config_get_string(config, "sound", "spk_eq_gains", nullptr);
		if (enabled && gains) {
			bctbx_list_t *gainsList = ms_parse_equalizer_string(gains);
			for (bctbx_list_t *it = gainsList; it; it = bctbx_list_next(it)) {
				MSEqualizerGain *g = reinterpret_cast<MSEqualizerGain *>(bctbx_list_get_data(it));
				lInfo() << "Read speaker equalizer gains: " << g->frequency << "(~" << g->width << ") --> " << g->gain;
				ms_filter_call_method(f, MS_EQUALIZER_SET_GAIN, g);
			}
			if (gainsList)
				bctbx_list_free_with_data(gainsList, ms_free);
		}
	}
}

void CorePrivate::postConfigureAudioStream (AudioStream *stream, bool muted) {
	L_Q();
	float micGain = q->getCCore()->sound_conf.soft_mic_lev;
	if (muted)
		audio_stream_set_mic_gain(stream, 0);
	else
		audio_stream_set_mic_gain_db(stream, micGain);
	float recvGain = q->getCCore()->sound_conf.soft_play_lev;
	if (static_cast<int>(recvGain))
		setPlaybackGainDb(stream, recvGain);
	LinphoneConfig *config = linphone_core_get_config(q->getCCore());
	float ngThres = lp_config_get_float(config, "sound", "ng_thres", 0.05f);
	float ngFloorGain = lp_config_get_float(config, "sound", "ng_floorgain", 0);
	if (stream->volsend) {
		int dcRemoval = lp_config_get_int(config, "sound", "dc_removal", 0);
		ms_filter_call_method(stream->volsend, MS_VOLUME_REMOVE_DC, &dcRemoval);
		float speed = lp_config_get_float(config, "sound", "el_speed", -1);
		float thres = lp_config_get_float(config, "sound", "el_thres", -1);
		float force = lp_config_get_float(config, "sound", "el_force", -1);
		int sustain = lp_config_get_int(config, "sound", "el_sustain", -1);
		float transmitThres = lp_config_get_float(config, "sound", "el_transmit_thres", -1);
		if (static_cast<int>(speed) == -1)
			speed = 0.03f;
		if (static_cast<int>(force) == -1)
			force = 25;
		MSFilter *f = stream->volsend;
		ms_filter_call_method(f, MS_VOLUME_SET_EA_SPEED, &speed);
		ms_filter_call_method(f, MS_VOLUME_SET_EA_FORCE, &force);
		if (static_cast<int>(thres) != -1)
			ms_filter_call_method(f, MS_VOLUME_SET_EA_THRESHOLD, &thres);
		if (static_cast<int>(sustain) != -1)
			ms_filter_call_method(f, MS_VOLUME_SET_EA_SUSTAIN, &sustain);
		if (static_cast<int>(transmitThres) != -1)
			ms_filter_call_method(f, MS_VOLUME_SET_EA_TRANSMIT_THRESHOLD, &transmitThres);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_THRESHOLD, &ngThres);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_FLOORGAIN, &ngFloorGain);
	}
	if (stream->volrecv) {
		/* Parameters for a limited noise-gate effect, using echo limiter threshold */
		float floorGain = (float)(1 / pow(10, micGain / 10));
		int spkAgc = lp_config_get_int(config, "sound", "speaker_agc_enabled", 0);
		MSFilter *f = stream->volrecv;
		ms_filter_call_method(f, MS_VOLUME_ENABLE_AGC, &spkAgc);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_THRESHOLD, &ngThres);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_FLOORGAIN, &floorGain);
	}
	parameterizeEqualizer(stream);
}

void CorePrivate::setPlaybackGainDb (AudioStream *stream, float gain) {
	if (stream->volrecv)
		ms_filter_call_method(stream->volrecv, MS_VOLUME_SET_DB_GAIN, &gain);
	else
		lWarning() << "Could not apply playback gain: gain control wasn't activated";
}

// =============================================================================

bool Core::areSoundResourcesLocked () const {
	L_D();
	for (const auto &call : d->calls) {
		if (call->mediaInProgress())
			return true;
		switch (call->getState()) {
			case CallSession::State::OutgoingInit:
			case CallSession::State::OutgoingProgress:
			case CallSession::State::OutgoingRinging:
			case CallSession::State::OutgoingEarlyMedia:
			case CallSession::State::Connected:
			case CallSession::State::Referred:
			case CallSession::State::IncomingEarlyMedia:
			case CallSession::State::Updating:
				lInfo() << "Call " << call << " is locking sound resources";
				return true;
			default:
				break;
		}
	}
	return false;
}

shared_ptr<Call> Core::getCallByRemoteAddress (const Address &addr) const {
	L_D();
	for (const auto &call : d->calls) {
		if (call->getRemoteAddress().weakEqual(addr))
			return call;
	}
	return nullptr;
}

const list<shared_ptr<Call>> &Core::getCalls () const {
	L_D();
	return d->calls;
}

unsigned int Core::getCallCount () const {
	L_D();
	return static_cast<unsigned int>(d->calls.size());
}

shared_ptr<Call> Core::getCurrentCall () const {
	L_D();
	return d->currentCall;
}

LinphoneStatus Core::pauseAllCalls () {
	L_D();
	for (const auto &call : d->calls) {
		if ((call->getState() == CallSession::State::StreamsRunning) || (call->getState() == CallSession::State::PausedByRemote))
			call->pause();
	}
	return 0;
}

void Core::soundcardHintCheck () {
	L_D();
	bool noNeedForSound = true;
	// Check if the remaining calls are paused
	for (const auto &call : d->calls) {
		if ((call->getState() != CallSession::State::Pausing)
			&& (call->getState() != CallSession::State::Paused)
			&& (call->getState() != CallSession::State::End)
			&& (call->getState() != CallSession::State::Error)) {
			noNeedForSound = false;
			break;
		}
	}
	// If no more calls or all calls are paused, we can free the soundcard
	LinphoneConfig *config = linphone_core_get_config(L_GET_C_BACK_PTR(this));
	bool useRtpIo = !!lp_config_get_int(config, "sound", "rtp_io", FALSE);
	bool useRtpIoEnableLocalOutput = !!lp_config_get_int(config, "sound", "rtp_io_enable_local_output", FALSE);
	
	LinphoneConference *conf_ctx = getCCore()->conf_ctx;
	if (conf_ctx && linphone_conference_get_size(conf_ctx) >= 1) return;
	
	if ((!d->hasCalls() || noNeedForSound)
		&& (!L_GET_C_BACK_PTR(getSharedFromThis())->use_files && (!useRtpIo || (useRtpIo && useRtpIoEnableLocalOutput)))) {
		lInfo() << "Notifying soundcard that we don't need it anymore for calls";
		d->notifySoundcardUsage(false);
	}
}

LinphoneStatus Core::terminateAllCalls () {
	L_D();
	while (!d->calls.empty()) {
		d->calls.front()->terminate();
	}
	return 0;
}

LINPHONE_END_NAMESPACE
