/*
 * call-enums.h
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

#ifndef _CALL_ENUMS_H_
#define _CALL_ENUMS_H_

// =============================================================================

#define L_ENUM_VALUES_CALL_SESSION_STATE(F) \
	F(Idle) \
	F(IncomingReceived) \
	F(OutgoingInit) \
	F(OutgoingProgress) \
	F(OutgoingRinging) \
	F(OutgoingEarlyMedia) \
	F(Connected) \
	F(StreamsRunning) \
	F(Pausing) \
	F(Paused) \
	F(Resuming) \
	F(Referred) \
	F(Error) \
	F(End) \
	F(PausedByRemote) \
	F(UpdatedByRemote) \
	F(IncomingEarlyMedia) \
	F(Updating) \
	F(Released) \
	F(EarlyUpdatedByRemote) \
	F(EarlyUpdating)

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

#endif // ifndef _CALL_ENUMS_H_
