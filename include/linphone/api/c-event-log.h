/*
 * c-event-log.h
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

#ifndef _C_EVENT_LOG_H_
#define _C_EVENT_LOG_H_

#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup events
 * @{
 */

// -----------------------------------------------------------------------------
// EventLog.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneEventLog object.
 **/
LINPHONE_PUBLIC LinphoneEventLog *linphone_event_log_new (void);

LINPHONE_PUBLIC LinphoneEventLog *linphone_event_log_ref (LinphoneEventLog *event_log);
LINPHONE_PUBLIC void linphone_event_log_unref (LinphoneEventLog *event_log);

/**
 * Returns the type of a event log.
 * @param[in] event_log A #LinphoneEventLog object
 * @return The event type
 */
LINPHONE_PUBLIC LinphoneEventLogType linphone_event_log_get_type (const LinphoneEventLog *event_log);

/**
 * Returns the time of a event log.
 * @param[in] event_log A #LinphoneEventLog object
 * @return The event time
 */
LINPHONE_PUBLIC time_t linphone_event_log_get_time (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceEvent *linphone_conference_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address
);

/**
 * Returns the conference address of a conference event.
 * @param[in] conference_event A #LinphoneConferenceEvent object.
 * @return The conference address.
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_conference_event_get_conference_address (
	const LinphoneConferenceEvent *conference_event
);

// -----------------------------------------------------------------------------
// ConferenceNotifiedEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceNotifiedEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceNotifiedEvent *linphone_conference_notified_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id
);

/**
 * Returns the notify id of a conference notified event.
 * @param[in] conference_notified_event A #LinphoneConferenceNotifiedEvent object.
 * @return The conference notify id.
 */
LINPHONE_PUBLIC unsigned int linphone_conference_notified_event_get_notify_id (
	const LinphoneConferenceNotifiedEvent *conference_notified_event
);

// -----------------------------------------------------------------------------
// ConferenceCallEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceCallEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceCallEvent *linphone_conference_call_event_new (
	LinphoneEventLogType type,
	time_t time,
	LinphoneCall *call
);

/**
 * Returns the call of a conference call event.
 * @param[in] conference_conference_call_event A #LinphoneConferenceCallEvent object.
 * @return The conference call.
 */
LINPHONE_PUBLIC LinphoneCall *linphone_conference_call_event_get_call (
	const LinphoneConferenceCallEvent *conference_call_event
);

// -----------------------------------------------------------------------------
// ConferenceChatMessageEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceChatMessageEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceChatMessageEvent *linphone_conference_chat_message_event_new (
	time_t time,
	LinphoneChatMessage *chat_message
);

/**
 * Returns the chat message of a conference chat message event.
 * @param[in] conference_chat_message_event A #LinphoneConferenceChatMessageEvent object.
 * @return The conference chat message.
 */
LINPHONE_PUBLIC LinphoneChatMessage *linphone_conference_chat_message_event_get_chat_message (
	const LinphoneConferenceChatMessageEvent *conference_chat_message_event
);

// -----------------------------------------------------------------------------
// ConferenceParticipantEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceParticipantEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceParticipantEvent *linphone_conference_participant_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id,
	const LinphoneAddress *participant_address
);

/**
 * Returns the participant address of a conference participant event.
 * @param[in] conference_participant_event A ConferenceParticipantEvent object.
 * @return The conference participant address.
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_conference_participant_event_get_participant_address (
	const LinphoneConferenceParticipantEvent *conference_participant_event
);

// -----------------------------------------------------------------------------
// ConferenceParticipantDeviceEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceParticipantDeviceEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceParticipantDeviceEvent *linphone_conference_participant_device_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id,
	const LinphoneAddress *participant_address,
	const LinphoneAddress *gruu_address
);

/**
 * Returns the gruu address of a conference participant device event.
 * @param[in] conference_participant_device_event A #LinphoneConferenceParticipantDeviceEvent object.
 * @return The conference gruu address.
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_conference_participant_device_event_get_gruu_address (
	const LinphoneConferenceParticipantDeviceEvent *conference_participant_device_event
);

// -----------------------------------------------------------------------------
// ConferenceSubjectEvent.
// -----------------------------------------------------------------------------

/**
 * Constructs a #LinphoneConferenceSubjectEvent object.
 **/
LINPHONE_PUBLIC LinphoneConferenceSubjectEvent *linphone_conference_subject_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conference_address,
	unsigned int notify_id,
	const char *subject
);

/**
 * Returns the subject of a conference subject event.
 * @param[in] conference_subject_event A #LinphoneConferenceSubjectEvent object.
 * @return The conference subject.
 */
LINPHONE_PUBLIC const char *linphone_conference_subject_event_get_subject (
	const LinphoneConferenceSubjectEvent *conference_subject_event
);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _C_EVENT_LOG_H_
