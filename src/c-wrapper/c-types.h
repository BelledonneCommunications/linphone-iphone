/*
 * c-types.h
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

#ifndef _C_TYPES_H_
#define _C_TYPES_H_

#define L_DECLARE_ENUM(CLASS, ENUM) enum Linphone ## CLASS ## ENUM
#define L_DECLARE_C_STRUCT(STRUCT) typedef struct _Linphone ## STRUCT Linphone ## STRUCT;

#include "event-log/event-log-enums.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif

L_DECLARE_C_STRUCT(ConferenceEvent);
L_DECLARE_C_STRUCT(ConferenceParticipantEvent);
L_DECLARE_C_STRUCT(EventLog);
L_DECLARE_C_STRUCT(Message);
L_DECLARE_C_STRUCT(MessageEvent);

// TODO: Remove me in the future.
typedef struct SalAddress LinphoneAddress;

// -----------------------------------------------------------------------------
// Conference Event.
// -----------------------------------------------------------------------------

LINPHONE_PUBLIC LinphoneConferenceEvent *conference_event_new (
	LinphoneEventLogType type,
	const LinphoneAddress *address
);

LINPHONE_PUBLIC const LinphoneAddress *conference_event_get_address ();

// -----------------------------------------------------------------------------
// Conference Participant Event.
// -----------------------------------------------------------------------------

LINPHONE_PUBLIC LinphoneConferenceParticipantEvent *conference_participant_event_new (
	LinphoneEventLogType type,
	const LinphoneAddress *conferenceAddress,
	const LinphoneAddress *participantAddress
);

LINPHONE_PUBLIC const LinphoneAddress *conference_participant_event_get_participant_address ();

// -----------------------------------------------------------------------------
// Event log.
// -----------------------------------------------------------------------------

LINPHONE_PUBLIC LinphoneEventLog *event_log_new ();
LINPHONE_PUBLIC LinphoneEventLogType event_log_get_type (const LinphoneEventLog *event_log);

// -----------------------------------------------------------------------------
// Message Event.
// -----------------------------------------------------------------------------

LINPHONE_PUBLIC LinphoneMessageEvent *message_event_new (LinphoneMessage *message);
LINPHONE_PUBLIC LinphoneMessage *message_event_get_message (const LinphoneMessageEvent *message_event);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
	}
#endif

#endif // ifndef _C_TYPES_H_
