/*
 * c-event-log.cpp
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

#include "linphone/api/c-event-log.h"

#include "c-wrapper/c-wrapper.h"
#include "call/call.h"
#include "chat/chat-message/chat-message.h"
#include "event-log/events.h"

// =============================================================================

L_DECLARE_C_OBJECT_IMPL(ConferenceCallEvent);
L_DECLARE_C_OBJECT_IMPL(ConferenceChatMessageEvent);
L_DECLARE_C_OBJECT_IMPL(ConferenceEvent);
L_DECLARE_C_OBJECT_IMPL(ConferenceNotifiedEvent);
L_DECLARE_C_OBJECT_IMPL(ConferenceParticipantDeviceEvent);
L_DECLARE_C_OBJECT_IMPL(ConferenceParticipantEvent);
L_DECLARE_C_OBJECT_IMPL(ConferenceSubjectEvent);
L_DECLARE_C_OBJECT_IMPL(EventLog);

using namespace std;

// -----------------------------------------------------------------------------
// EventLog.
// -----------------------------------------------------------------------------

LinphoneEventLog *linphone_event_log_new () {
	LinphoneEventLog *event_log = L_INIT(EventLog);
	L_SET_CPP_PTR_FROM_C_OBJECT(event_log, make_shared<LinphonePrivate::EventLog>());
	return event_log;
}

LinphoneEventLog *linphone_event_log_ref (LinphoneEventLog *event_log) {
	belle_sip_object_ref(event_log);
	return event_log;
}

void linphone_event_log_unref (LinphoneEventLog *event_log) {
	belle_sip_object_unref(event_log);
}

LinphoneEventLogType linphone_event_log_get_type (const LinphoneEventLog *event_log) {
	return static_cast<LinphoneEventLogType>(
		L_GET_CPP_PTR_FROM_C_OBJECT(event_log)->getType()
	);
}

time_t linphone_event_log_get_time (const LinphoneEventLog *event_log) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(event_log)->getTime();
}

// -----------------------------------------------------------------------------
// ConferenceEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceEvent *linphone_conference_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address
) {
	LinphoneConferenceEvent *conference_event = L_INIT(ConferenceEvent);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_event,
		make_shared<LinphonePrivate::ConferenceEvent>(
			static_cast<LinphonePrivate::EventLog::Type>(type),
			time,
			*L_GET_CPP_PTR_FROM_C_OBJECT(conference_address)
		)
	);
	return conference_event;
}

const LinphoneAddress *linphone_conference_event_get_conference_address (
	const LinphoneConferenceEvent *conference_event
) {
	return L_GET_C_BACK_PTR(
		&L_GET_CPP_PTR_FROM_C_OBJECT(conference_event)->getConferenceAddress()
	);
}

// -----------------------------------------------------------------------------
// ConferenceNotifiedEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceNotifiedEvent *linphone_conference_notified_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id
) {
	LinphoneConferenceNotifiedEvent *conference_notified_event = L_INIT(ConferenceNotifiedEvent);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_notified_event,
		make_shared<LinphonePrivate::ConferenceNotifiedEvent>(
			static_cast<LinphonePrivate::EventLog::Type>(type),
			time,
			*L_GET_CPP_PTR_FROM_C_OBJECT(conference_address),
			notify_id
		)
	);
	return conference_notified_event;
}

unsigned int linphone_conference_notified_event_get_notify_id (
	const LinphoneConferenceNotifiedEvent *conference_notified_event
) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(conference_notified_event)->getNotifyId();
}

// -----------------------------------------------------------------------------
// ConferenceCallEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceCallEvent *linphone_conference_call_event_new (
	LinphoneEventLogType type,
	time_t time,
	LinphoneCall *call
) {
	LinphoneConferenceCallEvent *conference_call_event = L_INIT(ConferenceCallEvent);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_call_event,
		make_shared<LinphonePrivate::ConferenceCallEvent>(
			static_cast<LinphonePrivate::EventLog::Type>(type),
			time,
			L_GET_CPP_PTR_FROM_C_OBJECT(call)
		)
	);
	return conference_call_event;
}

LinphoneCall *linphone_conference_call_event_get_call (const LinphoneConferenceCallEvent *conference_call_event) {
	return L_GET_C_BACK_PTR(
		L_GET_CPP_PTR_FROM_C_OBJECT(conference_call_event)->getCall()
	);
}

// -----------------------------------------------------------------------------
// ConferenceChatMessageEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceChatMessageEvent *linphone_conference_chat_message_event_new (
	time_t time,
	LinphoneChatMessage *chat_message
) {
	LinphoneConferenceChatMessageEvent *conference_chat_message_event = L_INIT(ConferenceChatMessageEvent);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_chat_message_event,
		make_shared<LinphonePrivate::ConferenceChatMessageEvent>(
			time,
			L_GET_CPP_PTR_FROM_C_OBJECT(chat_message)
		)
	);
	return conference_chat_message_event;
}

LinphoneChatMessage *linphone_conference_chat_message_event_get_chat_message (
	const LinphoneConferenceChatMessageEvent *conference_chat_message_event
) {
	return L_GET_C_BACK_PTR(
		L_GET_CPP_PTR_FROM_C_OBJECT(conference_chat_message_event)->getChatMessage()
	);
}

// -----------------------------------------------------------------------------
// ConferenceParticipantEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceParticipantEvent *linphone_conference_participant_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id,
	const LinphoneAddress *participant_address
) {
	LinphoneConferenceParticipantEvent *conference_participant_event = L_INIT(ConferenceParticipantEvent);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_participant_event,
		make_shared<LinphonePrivate::ConferenceParticipantEvent>(
			static_cast<LinphonePrivate::EventLog::Type>(type),
			time,
			*L_GET_CPP_PTR_FROM_C_OBJECT(conference_address),
			notify_id,
			*L_GET_CPP_PTR_FROM_C_OBJECT(participant_address)
		)
	);
	return conference_participant_event;
}

const LinphoneAddress *linphone_conference_participant_event_get_participant_address (
	const LinphoneConferenceParticipantEvent *conference_participant_event
) {
	return L_GET_C_BACK_PTR(
		&L_GET_CPP_PTR_FROM_C_OBJECT(conference_participant_event)->getParticipantAddress()
	);
}

// -----------------------------------------------------------------------------
// ConferenceParticipantDeviceEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceParticipantDeviceEvent *linphone_conference_participant_device_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id,
	const LinphoneAddress *participant_address,
	const LinphoneAddress *gruu_address
) {
	LinphoneConferenceParticipantDeviceEvent *conference_participant_device_event = L_INIT(
		ConferenceParticipantDeviceEvent
	);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_participant_device_event,
		make_shared<LinphonePrivate::ConferenceParticipantDeviceEvent>(
			static_cast<LinphonePrivate::EventLog::Type>(type),
			time,
			*L_GET_CPP_PTR_FROM_C_OBJECT(conference_address),
			notify_id,
			*L_GET_CPP_PTR_FROM_C_OBJECT(participant_address),
			*L_GET_CPP_PTR_FROM_C_OBJECT(gruu_address)
		)
	);
	return conference_participant_device_event;
}

const LinphoneAddress *linphone_conference_participant_device_event_get_gruu_address (
	const LinphoneConferenceParticipantDeviceEvent *conference_participant_device_event
) {
	return L_GET_C_BACK_PTR(
		&L_GET_CPP_PTR_FROM_C_OBJECT(conference_participant_device_event)->getGruuAddress()
	);
}

// -----------------------------------------------------------------------------
// ConferenceSubjectEvent.
// -----------------------------------------------------------------------------

LinphoneConferenceSubjectEvent *linphone_conference_subject_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id,
	const char *subject
) {
	LinphoneConferenceSubjectEvent *conference_subject_event = L_INIT(ConferenceSubjectEvent);
	L_SET_CPP_PTR_FROM_C_OBJECT(
		conference_subject_event,
		make_shared<LinphonePrivate::ConferenceSubjectEvent>(
			time,
			*L_GET_CPP_PTR_FROM_C_OBJECT(conference_address),
			notify_id,
			L_C_TO_STRING(subject)
		)
	);
	return conference_subject_event;
}

LINPHONE_PUBLIC const char *linphone_conference_subject_event_get_subject (
	const LinphoneConferenceSubjectEvent *conference_subject_event
) {
	return L_STRING_TO_C(
		L_GET_CPP_PTR_FROM_C_OBJECT(conference_subject_event)->getSubject()
	);
}
