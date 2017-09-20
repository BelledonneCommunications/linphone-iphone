/*
 * c-event-log.cpp
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

#include "linphone/api/c-chat-message.h"
#include "linphone/api/c-event-log.h"

#include "c-wrapper/c-tools.h"
#include "call/call.h"
#include "chat/chat-message.h"
#include "event-log/call-event.h"
#include "event-log/chat-message-event.h"
#include "event-log/conference-participant-event.h"

// =============================================================================

L_DECLARE_C_CLONABLE_STRUCT_IMPL(EventLog, EventLog, event_log);
L_DECLARE_C_CLONABLE_STRUCT_IMPL(CallEvent, CallEvent, call_event);
L_DECLARE_C_CLONABLE_STRUCT_IMPL(ConferenceEvent, ConferenceEvent, conference_event);
L_DECLARE_C_CLONABLE_STRUCT_IMPL(ConferenceParticipantEvent, ConferenceParticipantEvent, conference_participant_event);
L_DECLARE_C_CLONABLE_STRUCT_IMPL(ChatMessageEvent, ChatMessageEvent, chat_message_event);

using namespace std;

// -----------------------------------------------------------------------------
// Event log.
// -----------------------------------------------------------------------------

LinphoneEventLog *linphone_event_log_new () {
	LinphoneEventLog *event_log = _linphone_event_log_init();
	L_SET_CPP_PTR_FROM_C_STRUCT(event_log, new LINPHONE_NAMESPACE::EventLog());
	return event_log;
}

LinphoneEventLog *linphone_event_log_ref (LinphoneEventLog *event_log) {
	belle_sip_object_ref(event_log);
	return event_log;
}

LinphoneEventLogType linphone_event_log_get_type (const LinphoneEventLog *event_log) {
	return static_cast<LinphoneEventLogType>(
		L_GET_CPP_PTR_FROM_C_STRUCT(event_log, EventLog, EventLog)->getType()
	);
}

// -----------------------------------------------------------------------------
// Call event.
// -----------------------------------------------------------------------------

LinphoneCallEvent *linphone_call_event_new (LinphoneEventLogType type, LinphoneCall *call) {
	LinphoneCallEvent *call_event = _linphone_call_event_init();
	L_SET_CPP_PTR_FROM_C_STRUCT(
		call_event,
		new LINPHONE_NAMESPACE::CallEvent(
			static_cast<LINPHONE_NAMESPACE::EventLog::Type>(type),
			L_GET_CPP_PTR_FROM_C_STRUCT(call, Call, Call)
		)
	);
	return call_event;
}

// TODO: REMOVE ME.
extern LinphoneCall *_linphone_call_init ();

LinphoneCall *linphone_call_event_get_call (const LinphoneCallEvent *call_event) {
	return L_GET_C_BACK_PTR(
		L_GET_CPP_PTR_FROM_C_STRUCT(
			call_event, CallEvent, CallEvent
		)->getCall(),
		Call,
		call
	);
}

// -----------------------------------------------------------------------------
// Conference event.
// -----------------------------------------------------------------------------

LinphoneConferenceEvent *linphone_conference_event_new (
	LinphoneEventLogType type,
	const LinphoneAddress *address
) {
	// TODO.
	return nullptr;
}

const LinphoneAddress *linphone_conference_event_get_address (const LinphoneConferenceEvent *conference_event) {
	// TODO.
	return nullptr;
}

// -----------------------------------------------------------------------------
// Conference participant event.
// -----------------------------------------------------------------------------

LinphoneConferenceParticipantEvent *linphone_conference_participant_event_new (
	LinphoneEventLogType type,
	const LinphoneAddress *conferenceAddress,
	const LinphoneAddress *participantAddress
) {
	// TODO.
	return nullptr;
}

const LinphoneAddress *linphone_conference_participant_event_get_participant_address (
	const LinphoneConferenceParticipantEvent *conference_participant_event
) {
	// TODO.
	return nullptr;
}

// -----------------------------------------------------------------------------
// Message event.
// -----------------------------------------------------------------------------

LinphoneChatMessageEvent *linphone_chat_message_event_new (LinphoneChatMessage *chat_message) {
	LinphoneChatMessageEvent *chat_message_event = _linphone_chat_message_event_init();
	L_SET_CPP_PTR_FROM_C_STRUCT(
		chat_message_event,
		new LINPHONE_NAMESPACE::ChatMessageEvent(
			L_GET_CPP_PTR_FROM_C_STRUCT(chat_message, ChatMessage, ChatMessage)
		)
	);
	return chat_message_event;
}

// TODO: REMOVE ME.
extern LinphoneChatMessage *_linphone_chat_message_init ();

LinphoneChatMessage *linphone_chat_message_event_get_chat_message (const LinphoneChatMessageEvent *chat_message_event) {
	return L_GET_C_BACK_PTR(
		L_GET_CPP_PTR_FROM_C_STRUCT(
			chat_message_event, ChatMessageEvent, ChatMessageEvent
		)->getChatMessage(),
		ChatMessage,
		chat_message
	);
}
