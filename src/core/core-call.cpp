/*
 * core-call.cpp
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

#include <algorithm>

#include "core-p.h"
#include "call/call-p.h"
#include "logger/logger.h"

// TODO: Remove me later.
#include "c-wrapper/c-wrapper.h"

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

bool CorePrivate::isAlreadyInCallWithAddress (const Address &addr) const {
	for (const auto &call : calls) {
		if (call->getRemoteAddress().weakEqual(addr))
			return true;
	}
	return false;
}

void CorePrivate::iterateCalls (time_t currentRealTime, bool oneSecondElapsed) const {
	for (const auto &call : calls) {
		call->getPrivate()->iterate(currentRealTime, oneSecondElapsed);
	}
}

bool CorePrivate::canWeAddCall () const {
	L_Q();
	if (q->getCallCount() < static_cast<unsigned int>(q->getCCore()->max_calls))
		return true;
	lInfo() << "Maximum amount of simultaneous calls reached!";
	return false;
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

// =============================================================================

bool Core::areSoundResourcesLocked () const {
	L_D();
	for (const auto &call : d->calls) {
		if (call->mediaInProgress())
			return true;
		switch (call->getState()) {
			case LinphoneCallOutgoingInit:
			case LinphoneCallOutgoingProgress:
			case LinphoneCallOutgoingRinging:
			case LinphoneCallOutgoingEarlyMedia:
			case LinphoneCallConnected:
			case LinphoneCallRefered:
			case LinphoneCallIncomingEarlyMedia:
			case LinphoneCallUpdating:
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
		if ((call->getState() == LinphoneCallStreamsRunning) || (call->getState() == LinphoneCallPausedByRemote))
			call->pause();
	}
	return 0;
}

void Core::soundcardHintCheck () {
	L_D();
	bool noNeedForSound = true;
	// Check if the remaining calls are paused
	for (const auto &call : d->calls) {
		if ((call->getState() != LinphoneCallPausing)
			&& (call->getState() != LinphoneCallPaused)
			&& (call->getState() != LinphoneCallEnd)
			&& (call->getState() != LinphoneCallError)) {
			noNeedForSound = false;
			break;
		}
	}
	// If no more calls or all calls are paused, we can free the soundcard
	LinphoneConfig *config = linphone_core_get_config(L_GET_C_BACK_PTR(this));
	bool useRtpIo = !!lp_config_get_int(config, "sound", "rtp_io", FALSE);
	bool useRtpIoEnableLocalOutput = !!lp_config_get_int(config, "sound", "rtp_io_enable_local_output", FALSE);
	bool useFiles = L_GET_C_BACK_PTR(getSharedFromThis())->use_files;
	if ((!d->hasCalls() || noNeedForSound)
		&& (!useFiles && (!useRtpIo || (useRtpIo && useRtpIoEnableLocalOutput)))) {
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
