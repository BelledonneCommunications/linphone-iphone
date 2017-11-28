/*
 * call.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#include "c-wrapper/c-wrapper.h"
#include "call-p.h"
#include "conference/local-conference.h"
#include "conference/participant-p.h"
#include "conference/remote-conference.h"
#include "conference/session/media-session-p.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

CallPrivate::CallPrivate (
	LinphoneCall *call,
	LinphoneCore *core,
	LinphoneCallDir direction,
	const Address &from,
	const Address &to,
	LinphoneProxyConfig *cfg,
	SalOp *op,
	const MediaSessionParams *msp
) : lcall(call), core(core) {
	nextVideoFrameDecoded._func = nullptr;
	nextVideoFrameDecoded._user_data = nullptr;
}

CallPrivate::~CallPrivate () {
	if (conference)
		delete conference;
}

// -----------------------------------------------------------------------------

shared_ptr<CallSession> CallPrivate::getActiveSession () const {
	return conference->getActiveParticipant()->getPrivate()->getSession();
}

bool CallPrivate::getAudioMuted () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getAudioMuted();
}

LinphoneProxyConfig *CallPrivate::getDestProxy () const {
	return getActiveSession()->getPrivate()->getDestProxy();
}

IceSession *CallPrivate::getIceSession () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getIceSession();
}

MediaStream *CallPrivate::getMediaStream (LinphoneStreamType type) const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getMediaStream(type);
}

SalCallOp * CallPrivate::getOp () const {
	return getActiveSession()->getPrivate()->getOp();
}

void CallPrivate::setAudioMuted (bool value) {
	static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->setAudioMuted(value);
}

// -----------------------------------------------------------------------------

void CallPrivate::initiateIncoming () {
	getActiveSession()->initiateIncoming();
}

bool CallPrivate::initiateOutgoing () {
	return getActiveSession()->initiateOutgoing();
}

void CallPrivate::iterate (time_t currentRealTime, bool oneSecondElapsed) {
	getActiveSession()->iterate(currentRealTime, oneSecondElapsed);
}

void CallPrivate::startIncomingNotification () {
	getActiveSession()->startIncomingNotification();
}

int CallPrivate::startInvite (const Address *destination) {
	return getActiveSession()->startInvite(destination, "");
}

// -----------------------------------------------------------------------------

void CallPrivate::createPlayer () const {
	L_Q();
	player = linphone_call_build_player(L_GET_C_BACK_PTR(q));
}

// -----------------------------------------------------------------------------

void CallPrivate::onAckBeingSent (LinphoneHeaders *headers) {
	if (lcall)
		linphone_call_notify_ack_processing(lcall, headers, false);
}

void CallPrivate::onAckReceived (LinphoneHeaders *headers) {
	if (lcall)
		linphone_call_notify_ack_processing(lcall, headers, true);
}

void CallPrivate::onCallSetReleased () {
	if (lcall)
		linphone_call_unref(lcall);
}

void CallPrivate::onCallSetTerminated () {
	if (lcall) {
		if (lcall == core->current_call) {
			lInfo() << "Resetting the current call";
			core->current_call = nullptr;
		}
		if (linphone_core_del_call(core, lcall) != 0)
			lError() << "Could not remove the call from the list!!!";
		#if 0
			if (core->conf_ctx)
				linphone_conference_on_call_terminating(core->conf_ctx, lcall);
			if (lcall->ringing_beep) {
				linphone_core_stop_dtmf(core);
				lcall->ringing_beep = false;
			}
			if (lcall->chat_room)
				linphone_chat_room_set_call(lcall->chat_room, nullptr);
		#endif // if 0
		if (!core->calls)
			ms_bandwidth_controller_reset_state(core->bw_controller);
	}
}

void CallPrivate::onCallStateChanged (LinphoneCallState state, const string &message) {
	if (lcall)
		linphone_call_notify_state_changed(lcall, state, message.c_str());
}

void CallPrivate::onCheckForAcceptation () {
	bctbx_list_t *copy = bctbx_list_copy(linphone_core_get_calls(core));
	for (bctbx_list_t *it = copy; it != nullptr; it = bctbx_list_next(it)) {
		LinphoneCall *call = reinterpret_cast<LinphoneCall *>(bctbx_list_get_data(it));
		if (call == lcall) continue;
		switch (linphone_call_get_state(call)) {
			case LinphoneCallOutgoingInit:
			case LinphoneCallOutgoingProgress:
			case LinphoneCallOutgoingRinging:
			case LinphoneCallOutgoingEarlyMedia:
				lInfo() << "Already existing call [" << call << "] in state [" << linphone_call_state_to_string(linphone_call_get_state(call)) <<
					"], canceling it before accepting new call [" << lcall << "]";
				linphone_call_terminate(call);
				break;
			default:
				break; /* Nothing to do */
		}
	}
	bctbx_list_free(copy);
}

void CallPrivate::onIncomingCallStarted () {
	if (lcall)
		linphone_core_notify_incoming_call(core, lcall);
}

void CallPrivate::onIncomingCallToBeAdded () {
	if (lcall) /* The call is acceptable so we can now add it to our list */
		linphone_core_add_call(core, lcall);
}

void CallPrivate::onInfoReceived (const LinphoneInfoMessage *im) {
	if (lcall)
		linphone_call_notify_info_message_received(lcall, im);
}

void CallPrivate::onEncryptionChanged (bool activated, const string &authToken) {
	if (lcall)
		linphone_call_notify_encryption_changed(lcall, activated, authToken.empty() ? nullptr : authToken.c_str());
}

void CallPrivate::onStatsUpdated (const LinphoneCallStats *stats) {
	if (lcall)
		linphone_call_notify_stats_updated(lcall, stats);
}

void CallPrivate::onResetCurrentCall () {
	core->current_call = nullptr;
}

void CallPrivate::onSetCurrentCall () {
	if (lcall)
		core->current_call = lcall;
}

void CallPrivate::onFirstVideoFrameDecoded () {
	if (lcall && nextVideoFrameDecoded._func) {
		nextVideoFrameDecoded._func(lcall, nextVideoFrameDecoded._user_data);
		nextVideoFrameDecoded._func = nullptr;
		nextVideoFrameDecoded._user_data = nullptr;
	}
}

void CallPrivate::onResetFirstVideoFrameDecoded () {
	#ifdef VIDEO_ENABLED
		if (lcall && nextVideoFrameDecoded._func)
			static_cast<MediaSession *>(getActiveSession().get())->resetFirstVideoFrameDecoded();
	#endif // ifdef VIDEO_ENABLED
}

// =============================================================================

Call::Call (
	LinphoneCall *call,
	LinphoneCore *core,
	LinphoneCallDir direction,
	const Address &from,
	const Address &to,
	LinphoneProxyConfig *cfg,
	SalCallOp *op,
	const MediaSessionParams *msp
) : Object(*new CallPrivate(call, core, direction, from, to, cfg, op, msp)) {
	L_D();
	const Address *myAddress = (direction == LinphoneCallIncoming) ? &to : &from;
	string confType = lp_config_get_string(linphone_core_get_config(core), "misc", "conference_type", "local");
	if (confType == "remote") {
		d->conference = new RemoteConference(core->cppCore, *myAddress, d);
	} else {
		d->conference = new LocalConference(core->cppCore, *myAddress, d);
	}
	const Address *remoteAddress = (direction == LinphoneCallIncoming) ? &from : &to;
	d->conference->addParticipant(*remoteAddress, msp, true);
	shared_ptr<Participant> participant = d->conference->getParticipants().front();
	participant->getPrivate()->getSession()->configure(direction, cfg, op, from, to);
}

// -----------------------------------------------------------------------------

LinphoneStatus Call::accept (const MediaSessionParams *msp) {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->accept(msp);
}

LinphoneStatus Call::acceptEarlyMedia (const MediaSessionParams *msp) {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->acceptEarlyMedia(msp);
}

LinphoneStatus Call::acceptUpdate (const MediaSessionParams *msp) {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->acceptUpdate(msp);
}

LinphoneStatus Call::decline (LinphoneReason reason) {
	L_D();
	return d->getActiveSession()->decline(reason);
}

LinphoneStatus Call::decline (const LinphoneErrorInfo *ei) {
	L_D();
	return d->getActiveSession()->decline(ei);
}

void Call::oglRender () const {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->getPrivate()->oglRender();
}

LinphoneStatus Call::pause () {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->pause();
}

LinphoneStatus Call::redirect (const std::string &redirectUri) {
	L_D();
	return d->getActiveSession()->redirect(redirectUri);
}

LinphoneStatus Call::resume () {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->resume();
}

void Call::sendVfuRequest () {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->sendVfuRequest();
}

void Call::startRecording () {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->startRecording();
}

void Call::stopRecording () {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->stopRecording();
}

LinphoneStatus Call::takePreviewSnapshot (const string &file) {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->takePreviewSnapshot(file);
}

LinphoneStatus Call::takeVideoSnapshot (const string &file) {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->takeVideoSnapshot(file);
}

LinphoneStatus Call::terminate (const LinphoneErrorInfo *ei) {
	L_D();
	return d->getActiveSession()->terminate(ei);
}

LinphoneStatus Call::update (const MediaSessionParams *msp) {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->update(msp);
}

void Call::zoomVideo (float zoomFactor, float *cx, float *cy) {
	zoomVideo(zoomFactor, *cx, *cy);
}

void Call::zoomVideo (float zoomFactor, float cx, float cy) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->zoomVideo(zoomFactor, cx, cy);
}

// -----------------------------------------------------------------------------

bool Call::cameraEnabled () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->cameraEnabled();
}

bool Call::echoCancellationEnabled () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->echoCancellationEnabled();
}

bool Call::echoLimiterEnabled () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->echoLimiterEnabled();
}

void Call::enableCamera (bool value) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->enableCamera(value);
}

void Call::enableEchoCancellation (bool value) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->enableEchoCancellation(value);
}

void Call::enableEchoLimiter (bool value) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->enableEchoLimiter(value);
}

bool Call::getAllMuted () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getAllMuted();
}

LinphoneCallStats *Call::getAudioStats () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getAudioStats();
}

string Call::getAuthenticationToken () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getAuthenticationToken();
}

bool Call::getAuthenticationTokenVerified () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getAuthenticationTokenVerified();
}

float Call::getAverageQuality () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getAverageQuality();
}

LinphoneCore *Call::getCore () const {
	L_D();
	return d->core;
}

const MediaSessionParams *Call::getCurrentParams () const {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->getCurrentParams();
}

float Call::getCurrentQuality () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getCurrentQuality();
}

LinphoneCallDir Call::getDirection () const {
	L_D();
	return d->getActiveSession()->getDirection();
}

int Call::getDuration () const {
	L_D();
	return d->getActiveSession()->getDuration();
}

const LinphoneErrorInfo *Call::getErrorInfo () const {
	L_D();
	return d->getActiveSession()->getErrorInfo();
}

LinphoneCallLog *Call::getLog () const {
	L_D();
	return d->getActiveSession()->getLog();
}

RtpTransport *Call::getMetaRtcpTransport (int streamIndex) const {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->getMetaRtcpTransport(streamIndex);
}

RtpTransport *Call::getMetaRtpTransport (int streamIndex) const {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->getMetaRtpTransport(streamIndex);
}

float Call::getMicrophoneVolumeGain () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getMicrophoneVolumeGain();
}

void *Call::getNativeVideoWindowId () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getNativeVideoWindowId();
}

const MediaSessionParams *Call::getParams () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getMediaParams();
}

LinphonePlayer *Call::getPlayer () const {
	L_D();
	if (!d->player)
		d->createPlayer();
	return d->player;
}

float Call::getPlayVolume () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getPlayVolume();
}

LinphoneReason Call::getReason () const {
	L_D();
	return d->getActiveSession()->getReason();
}

float Call::getRecordVolume () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getRecordVolume();
}

const Address &Call::getRemoteAddress () const {
	L_D();
	return d->getActiveSession()->getRemoteAddress();
}

string Call::getRemoteAddressAsString () const {
	L_D();
	return d->getActiveSession()->getRemoteAddressAsString();
}

string Call::getRemoteContact () const {
	L_D();
	return d->getActiveSession()->getRemoteContact();
}

const MediaSessionParams *Call::getRemoteParams () const {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->getRemoteParams();
}

string Call::getRemoteUserAgent () const {
	L_D();
	return d->getActiveSession()->getRemoteUserAgent();
}

float Call::getSpeakerVolumeGain () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getSpeakerVolumeGain();
}

LinphoneCallState Call::getState () const {
	L_D();
	return d->getActiveSession()->getState();
}

LinphoneCallStats *Call::getStats (LinphoneStreamType type) const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getStats(type);
}

int Call::getStreamCount () const {
	L_D();
	return static_cast<MediaSession *>(d->getActiveSession().get())->getStreamCount();
}

MSFormatType Call::getStreamType (int streamIndex) const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getStreamType(streamIndex);
}

LinphoneCallStats *Call::getTextStats () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getTextStats();
}

LinphoneCallStats *Call::getVideoStats () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->getVideoStats();
}

bool Call::mediaInProgress () const {
	L_D();
	return static_cast<const MediaSession *>(d->getActiveSession().get())->mediaInProgress();
}

void Call::setAuthenticationTokenVerified (bool value) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->setAuthenticationTokenVerified(value);
}

void Call::setMicrophoneVolumeGain (float value) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->setMicrophoneVolumeGain(value);
}

void Call::setNativeVideoWindowId (void *id) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->setNativeVideoWindowId(id);
}

void Call::setNextVideoFrameDecodedCallback (LinphoneCallCbFunc cb, void *user_data) {
	L_D();
	d->nextVideoFrameDecoded._func = cb;
	d->nextVideoFrameDecoded._user_data = user_data;
	d->onResetFirstVideoFrameDecoded();
}

void Call::setSpeakerVolumeGain (float value) {
	L_D();
	static_cast<MediaSession *>(d->getActiveSession().get())->setSpeakerVolumeGain(value);
}

LINPHONE_END_NAMESPACE
