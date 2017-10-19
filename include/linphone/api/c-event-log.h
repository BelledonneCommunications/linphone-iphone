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

LINPHONE_PUBLIC LinphoneEventLog *linphone_event_log_new (void);
LINPHONE_PUBLIC LinphoneEventLog *linphone_event_log_ref (LinphoneEventLog *event_log);
LINPHONE_PUBLIC LinphoneEventLogType linphone_event_log_get_type (const LinphoneEventLog *event_log);

LINPHONE_PUBLIC LinphoneCallEvent *linphone_call_event_new (
	LinphoneEventLogType type,
	time_t time,
	LinphoneCall *call
);
LINPHONE_PUBLIC LinphoneCall *linphone_call_event_get_call (const LinphoneCallEvent *call_event);

LINPHONE_PUBLIC LinphoneConferenceEvent *linphone_conference_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *address
);
LINPHONE_PUBLIC const LinphoneAddress *linphone_conference_event_get_address (const LinphoneConferenceEvent *conference_event);

LINPHONE_PUBLIC LinphoneConferenceParticipantEvent *linphone_conference_participant_event_new (
	LinphoneEventLogType type,
	time_t time,
	const LinphoneAddress *conferenceAddress,
	const LinphoneAddress *participantAddress
);
LINPHONE_PUBLIC const LinphoneAddress *linphone_conference_participant_event_get_participant_address (
	const LinphoneConferenceParticipantEvent *conference_participant_event
);

LINPHONE_PUBLIC LinphoneChatMessageEvent *linphone_chat_message_event_new (
	LinphoneChatMessage *chat_message,
	time_t time
);
LINPHONE_PUBLIC LinphoneChatMessage *linphone_chat_message_event_get_chat_message (
	const LinphoneChatMessageEvent *chat_message_event
);

#ifdef __cplusplus
	}
#endif

#endif // ifndef _C_EVENT_LOG_H_
