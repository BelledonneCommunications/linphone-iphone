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

L_DECLARE_C_OBJECT_IMPL(EventLog);

using namespace std;

// -----------------------------------------------------------------------------
// Helpers.
// -----------------------------------------------------------------------------

static bool isConferenceType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceCallEnd:
		case LinphoneEventLogTypeConferenceCallStart:
		case LinphoneEventLogTypeConferenceChatMessage:
		case LinphoneEventLogTypeConferenceCreated:
		case LinphoneEventLogTypeConferenceDestroyed:
		case LinphoneEventLogTypeConferenceParticipantAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceRemoved:
		case LinphoneEventLogTypeConferenceParticipantRemoved:
		case LinphoneEventLogTypeConferenceParticipantSetAdmin:
		case LinphoneEventLogTypeConferenceParticipantUnsetAdmin:
		case LinphoneEventLogTypeConferenceSubjectChanged:
			return true;

		default:
			break;
	}

	return false;
}

static bool isConferenceCallType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceCallEnd:
		case LinphoneEventLogTypeConferenceCallStart:
			return true;

		default:
			break;
	}

	return false;
}

static bool isConferenceChatMessageType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceChatMessage:
			return true;

		default:
			break;
	}

	return false;
}

static bool isConferenceNotifiedType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceParticipantAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceRemoved:
		case LinphoneEventLogTypeConferenceParticipantRemoved:
		case LinphoneEventLogTypeConferenceParticipantSetAdmin:
		case LinphoneEventLogTypeConferenceParticipantUnsetAdmin:
		case LinphoneEventLogTypeConferenceSubjectChanged:
			return true;

		default:
			break;
	}

	return false;
}

static bool isConferenceParticipantType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceParticipantAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceRemoved:
		case LinphoneEventLogTypeConferenceParticipantRemoved:
		case LinphoneEventLogTypeConferenceParticipantSetAdmin:
		case LinphoneEventLogTypeConferenceParticipantUnsetAdmin:
			return true;

		default:
			break;
	}

	return false;
}

static bool isConferenceParticipantDeviceType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceParticipantDeviceAdded:
		case LinphoneEventLogTypeConferenceParticipantDeviceRemoved:
			return true;

		default:
			break;
	}

	return false;
}

static bool isConferenceSubjectType (LinphoneEventLogType type) {
	switch (type) {
		case LinphoneEventLogTypeConferenceSubjectChanged:
			return true;

		default:
			break;
	}

	return false;
}

// -----------------------------------------------------------------------------
// EventLog.
// -----------------------------------------------------------------------------

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

const LinphoneAddress *linphone_event_log_get_conference_address (const LinphoneEventLog *event_log) {
	if (!isConferenceType(linphone_event_log_get_type(event_log)))
		return nullptr;

	return L_GET_C_BACK_PTR(
		&static_pointer_cast<const LinphonePrivate::ConferenceEvent>(
			L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
		)->getConferenceAddress()
	);
}

// -----------------------------------------------------------------------------
// ConferenceNotifiedEvent.
// -----------------------------------------------------------------------------

unsigned int linphone_event_log_get_notify_id (const LinphoneEventLog *event_log) {
	if (!isConferenceNotifiedType(linphone_event_log_get_type(event_log)))
		return 0;

	return static_pointer_cast<const LinphonePrivate::ConferenceNotifiedEvent>(
		L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
	)->getNotifyId();
}

// -----------------------------------------------------------------------------
// ConferenceCallEvent.
// -----------------------------------------------------------------------------

LinphoneCall *linphone_event_log_get_call (const LinphoneEventLog *event_log) {
	if (!isConferenceCallType(linphone_event_log_get_type(event_log)))
		return nullptr;

	return L_GET_C_BACK_PTR(
		static_pointer_cast<const LinphonePrivate::ConferenceCallEvent>(
			L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
		)->getCall()
	);
}

// -----------------------------------------------------------------------------
// ConferenceChatMessageEvent.
// -----------------------------------------------------------------------------

LinphoneChatMessage *linphone_event_log_get_chat_message (const LinphoneEventLog *event_log) {
	if (!isConferenceChatMessageType(linphone_event_log_get_type(event_log)))
		return nullptr;

	return L_GET_C_BACK_PTR(
		static_pointer_cast<const LinphonePrivate::ConferenceChatMessageEvent>(
			L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
		)->getChatMessage()
	);
}

// -----------------------------------------------------------------------------
// ConferenceParticipantEvent.
// -----------------------------------------------------------------------------

const LinphoneAddress *linphone_event_log_get_participant_address (const LinphoneEventLog *event_log) {
	if (!isConferenceParticipantType(linphone_event_log_get_type(event_log)))
		return nullptr;

	return L_GET_C_BACK_PTR(
		&static_pointer_cast<const LinphonePrivate::ConferenceParticipantEvent>(
			L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
		)->getParticipantAddress()
	);
}

// -----------------------------------------------------------------------------
// ConferenceParticipantDeviceEvent.
// -----------------------------------------------------------------------------

const LinphoneAddress *linphone_event_log_get_gruu_address (const LinphoneEventLog *event_log) {
	if (!isConferenceParticipantDeviceType(linphone_event_log_get_type(event_log)))
		return nullptr;

	return L_GET_C_BACK_PTR(
		&static_pointer_cast<const LinphonePrivate::ConferenceParticipantDeviceEvent>(
			L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
		)->getGruuAddress()
	);
}

// -----------------------------------------------------------------------------
// ConferenceSubjectEvent.
// -----------------------------------------------------------------------------

LINPHONE_PUBLIC const char *linphone_event_log_get_subject (const LinphoneEventLog *event_log) {
	if (!isConferenceSubjectType(linphone_event_log_get_type(event_log)))
		return nullptr;

	return L_STRING_TO_C(
		static_pointer_cast<const LinphonePrivate::ConferenceSubjectEvent>(
			L_GET_CPP_PTR_FROM_C_OBJECT(event_log)
		)->getSubject()
	);
}
