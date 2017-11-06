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
 * Increment reference count of #LinphoneEventLog object.
 **/
LINPHONE_PUBLIC LinphoneEventLog *linphone_event_log_ref (LinphoneEventLog *event_log);

/**
 * Decrement reference count of #LinphoneEventLog object. When dropped to zero, memory is freed.
 **/
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
 * Returns the conference address of a conference event.
 * @param[in] event_log A #LinphoneEventLog object.
 * @return The conference address.
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_event_log_get_conference_address (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceNotifiedEvent.
// -----------------------------------------------------------------------------

/**
 * Returns the notify id of a conference notified event.
 * @param[in] event_log A #LinphoneEventLog object.
 * @return The conference notify id.
 */
LINPHONE_PUBLIC unsigned int linphone_event_log_get_notify_id (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceCallEvent.
// -----------------------------------------------------------------------------

/**
 * Returns the call of a conference call event.
 * @param[in] event_log A #LinphoneEventLog object.
 * @return The conference call.
 */
LINPHONE_PUBLIC LinphoneCall *linphone_event_log_get_call (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceChatMessageEvent.
// -----------------------------------------------------------------------------

/**
 * Returns the chat message of a conference chat message event.
 * @param[in] event_log A #LinphoneEventLog object.
 * @return The conference chat message.
 */
LINPHONE_PUBLIC LinphoneChatMessage *linphone_event_log_get_chat_message (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceParticipantEvent.
// -----------------------------------------------------------------------------

/**
 * Returns the participant address of a conference participant event.
 * @param[in] event_log A ConferenceParticipantEvent object.
 * @return The conference participant address.
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_event_log_get_participant_address (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceParticipantDeviceEvent.
// -----------------------------------------------------------------------------

/**
 * Returns the gruu address of a conference participant device event.
 * @param[in] event_log A #LinphoneEventLog object.
 * @return The conference gruu address.
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_event_log_get_gruu_address (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// ConferenceSubjectEvent.
// -----------------------------------------------------------------------------

/**
 * Returns the subject of a conference subject event.
 * @param[in] event_log A #LinphoneEventLog object.
 * @return The conference subject.
 */
LINPHONE_PUBLIC const char *linphone_event_log_get_subject (const LinphoneEventLog *event_log);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _C_EVENT_LOG_H_