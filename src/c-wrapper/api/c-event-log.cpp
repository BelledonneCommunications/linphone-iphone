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

#include "c-wrapper/c-tools.h"

#include "c-event-log.h"

#include "event-log/call-event.h"
#include "event-log/conference-participant-event.h"
#include "event-log/message-event.h"

// =============================================================================

using namespace std;

extern "C" {
// -----------------------------------------------------------------------------
// Event log.
// -----------------------------------------------------------------------------

L_DECLARE_C_STRUCT_IMPL(EventLog, event_log);
L_DECLARE_C_STRUCT_NEW_DEFAULT(EventLog, event_log);

LinphoneEventLogType linphone_event_log_get_type (const LinphoneEventLog *eventLog) {
	return static_cast<LinphoneEventLogType>(eventLog->cppPtr->getType());
}

// -----------------------------------------------------------------------------
// Message event.
// -----------------------------------------------------------------------------

L_DECLARE_C_STRUCT_IMPL(MessageEvent, message_event);

LinphoneMessageEvent *linphone_message_event_new (LinphoneMessage *message) {
	LinphoneMessageEvent *object = _linphone_message_event_init();
	// TODO: call make_shared with cppPtr.
	object->cppPtr = make_shared<LINPHONE_NAMESPACE::MessageEvent>(nullptr);
	return object;
}

LinphoneMessage *linphone_message_event_get_message (const LinphoneMessageEvent *messageEvent) {
	// TODO.
	return nullptr;
}

// -----------------------------------------------------------------------------
// Call event.
// -----------------------------------------------------------------------------

// L_DECLARE_C_STRUCT_IMPL(CallEvent, call_event);

LinphoneCallEvent *linphone_call_event_new (LinphoneEventLogType type, LinphoneCall *call) {
	// TODO.
	return nullptr;
}

LinphoneCall *linphone_call_event_get_call (const LinphoneCallEvent *call_event) {
	// TODO.
	return nullptr;
}

// -----------------------------------------------------------------------------
// Conference event.
// -----------------------------------------------------------------------------

// L_DECLARE_C_STRUCT_IMPL(ConferenceEvent, conference_event);

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

// L_DECLARE_C_STRUCT_IMPL(ConferenceParticipantEvent, conference_participant_event);

LinphoneConferenceParticipantEvent *linphone_conference_participant_event_new (
	LinphoneEventLogType type,
	const LinphoneAddress *conference_address,
	const LinphoneAddress *participant_address
) {
	// TODO.
	return nullptr;
}

const LinphoneAddress *linphone_conference_participant_event_get_participant_address (const LinphoneConferenceParticipantEvent *conference_participant_event) {
	// TODO.
	return nullptr;
}
}
