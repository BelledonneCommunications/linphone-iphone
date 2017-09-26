/*
 * c-wrapper.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _C_WRAPPER_H_
#define _C_WRAPPER_H_

#include "linphone/api/c-types.h"

#include "internal/c-tools.h"

// =============================================================================
// Declare exported C types.
// =============================================================================

#define L_REGISTER_TYPES(F) \
	F(Address, Address) \
	F(Call, Call) \
	F(CallEvent, CallEvent) \
	F(ChatMessage, ChatMessage) \
	F(ChatMessageEvent, ChatMessageEvent) \
	F(ChatRoom, ChatRoom) \
	F(ConferenceEvent, ConferenceEvent) \
	F(ConferenceParticipantEvent, ConferenceParticipantEvent) \
	F(EventLog, EventLog) \
	F(MediaSessionParams, CallParams) \
	F(Participant, Participant)

#define L_REGISTER_SUBTYPES(F) \
	F(ChatRoom, BasicChatRoom) \
	F(ChatRoom, ClientGroupChatRoom) \
	F(ChatRoom, RealTimeTextChatRoom)

// =============================================================================
// Register belle-sip ID.
// =============================================================================

#define L_REGISTER_ID(CPP_TYPE, C_TYPE) BELLE_SIP_TYPE_ID(Linphone ## C_TYPE),

BELLE_SIP_DECLARE_TYPES_BEGIN(linphone, 10000)
L_REGISTER_TYPES(L_REGISTER_ID)
BELLE_SIP_TYPE_ID(LinphoneAccountCreator),
BELLE_SIP_TYPE_ID(LinphoneAccountCreatorCbs),
BELLE_SIP_TYPE_ID(LinphoneAccountCreatorService),
BELLE_SIP_TYPE_ID(LinphoneAuthInfo),
BELLE_SIP_TYPE_ID(LinphoneBuffer),
BELLE_SIP_TYPE_ID(LinphoneCallCbs),
BELLE_SIP_TYPE_ID(LinphoneCallLog),
BELLE_SIP_TYPE_ID(LinphoneCallStats),
BELLE_SIP_TYPE_ID(LinphoneChatMessageCbs),
BELLE_SIP_TYPE_ID(LinphoneChatRoomCbs),
BELLE_SIP_TYPE_ID(LinphoneConference),
BELLE_SIP_TYPE_ID(LinphoneConferenceParams),
BELLE_SIP_TYPE_ID(LinphoneConfig),
BELLE_SIP_TYPE_ID(LinphoneContactProvider),
BELLE_SIP_TYPE_ID(LinphoneContactSearch),
BELLE_SIP_TYPE_ID(LinphoneContent),
BELLE_SIP_TYPE_ID(LinphoneCore),
BELLE_SIP_TYPE_ID(LinphoneCoreCbs),
BELLE_SIP_TYPE_ID(LinphoneErrorInfo),
BELLE_SIP_TYPE_ID(LinphoneEvent),
BELLE_SIP_TYPE_ID(LinphoneFactory),
BELLE_SIP_TYPE_ID(LinphoneFriend),
BELLE_SIP_TYPE_ID(LinphoneFriendList),
BELLE_SIP_TYPE_ID(LinphoneFriendListCbs),
BELLE_SIP_TYPE_ID(LinphoneImEncryptionEngine),
BELLE_SIP_TYPE_ID(LinphoneImEncryptionEngineCbs),
BELLE_SIP_TYPE_ID(LinphoneImNotifPolicy),
BELLE_SIP_TYPE_ID(LinphoneInfoMessage),
BELLE_SIP_TYPE_ID(LinphoneLDAPContactProvider),
BELLE_SIP_TYPE_ID(LinphoneLDAPContactSearch),
BELLE_SIP_TYPE_ID(LinphoneNatPolicy),
BELLE_SIP_TYPE_ID(LinphonePayloadType),
BELLE_SIP_TYPE_ID(LinphonePlayer),
BELLE_SIP_TYPE_ID(LinphonePlayerCbs),
BELLE_SIP_TYPE_ID(LinphonePresenceActivity),
BELLE_SIP_TYPE_ID(LinphonePresenceModel),
BELLE_SIP_TYPE_ID(LinphonePresenceNote),
BELLE_SIP_TYPE_ID(LinphonePresencePerson),
BELLE_SIP_TYPE_ID(LinphonePresenceService),
BELLE_SIP_TYPE_ID(LinphoneProxyConfig),
BELLE_SIP_TYPE_ID(LinphoneRange),
BELLE_SIP_TYPE_ID(LinphoneTransports),
BELLE_SIP_TYPE_ID(LinphoneTunnel),
BELLE_SIP_TYPE_ID(LinphoneTunnelConfig),
BELLE_SIP_TYPE_ID(LinphoneVcard),
BELLE_SIP_TYPE_ID(LinphoneVideoActivationPolicy),
BELLE_SIP_TYPE_ID(LinphoneVideoDefinition),
BELLE_SIP_TYPE_ID(LinphoneXmlRpcRequest),
BELLE_SIP_TYPE_ID(LinphoneXmlRpcRequestCbs),
BELLE_SIP_TYPE_ID(LinphoneXmlRpcSession)
BELLE_SIP_DECLARE_TYPES_END

#undef L_REGISTER_ID

// =============================================================================
// Register C types.
// =============================================================================

L_REGISTER_TYPES(L_REGISTER_TYPE);
L_REGISTER_SUBTYPES(L_REGISTER_SUBTYPE);

#undef L_REGISTER_SUBTYPES
#undef L_REGISTER_TYPES

#endif // ifndef _C_WRAPPER_H_
