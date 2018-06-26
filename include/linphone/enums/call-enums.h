/*
 * call-enums.h
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

#ifndef _L_CALL_ENUMS_H_
#define _L_CALL_ENUMS_H_

// =============================================================================

#define L_ENUM_VALUES_CALL_SESSION_STATE(F) \
	F(Idle /**< Initial state */) \
	F(IncomingReceived /**< Incoming call received */) \
	F(OutgoingInit /**< Outgoing call initialized */) \
	F(OutgoingProgress /**< Outgoing call in progress */) \
	F(OutgoingRinging /**< Outgoing call ringing */) \
	F(OutgoingEarlyMedia /**< Outgoing call early media */) \
	F(Connected /**< Connected */) \
	F(StreamsRunning /**< Streams running */) \
	F(Pausing /**< Pausing */) \
	F(Paused /**< Paused */) \
	F(Resuming /**< Resuming */) \
	F(Referred /**< Referred */) \
	F(Error /**< Error */) \
	F(End /**< Call end */) \
	F(PausedByRemote /**< Paused by remote */) \
	F(UpdatedByRemote /**< The call&apos;s parameters are updated for example when video is asked by remote */) \
	F(IncomingEarlyMedia /**< We are proposing early media to an incoming call */) \
	F(Updating /**< We have initiated a call update */) \
	F(Released /**< The call object is now released */) \
	F(EarlyUpdatedByRemote /**< The call is updated by remote while not yet answered (SIP UPDATE in early dialog received) */) \
	F(EarlyUpdating /**< We are updating the call while not yet answered (SIP UPDATE in early dialog sent) */)

// =============================================================================
// DEPRECATED
// =============================================================================

#define LinphoneCallIdle LinphoneCallStateIdle
#define LinphoneCallIncomingReceived LinphoneCallStateIncomingReceived
#define LinphoneCallOutgoingInit LinphoneCallStateOutgoingInit
#define LinphoneCallOutgoingProgress LinphoneCallStateOutgoingProgress
#define LinphoneCallOutgoingRinging LinphoneCallStateOutgoingRinging
#define LinphoneCallOutgoingEarlyMedia LinphoneCallStateOutgoingEarlyMedia
#define LinphoneCallConnected LinphoneCallStateConnected
#define LinphoneCallStreamsRunning LinphoneCallStateStreamsRunning
#define LinphoneCallPausing LinphoneCallStatePausing
#define LinphoneCallPaused LinphoneCallStatePaused
#define LinphoneCallResuming LinphoneCallStateResuming
#define LinphoneCallRefered LinphoneCallStateReferred
#define LinphoneCallError LinphoneCallStateError
#define LinphoneCallEnd LinphoneCallStateEnd
#define LinphoneCallPausedByRemote LinphoneCallStatePausedByRemote
#define LinphoneCallUpdatedByRemote LinphoneCallStateUpdatedByRemote
#define LinphoneCallIncomingEarlyMedia LinphoneCallStateIncomingEarlyMedia
#define LinphoneCallUpdating LinphoneCallStateUpdating
#define LinphoneCallReleased LinphoneCallStateReleased
#define LinphoneCallEarlyUpdatedByRemote LinphoneCallStateEarlyUpdatedByRemote
#define LinphoneCallEarlyUpdating LinphoneCallStateEarlyUpdating

#endif // ifndef _L_CALL_ENUMS_H_
