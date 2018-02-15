/*
 * statements.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#include "statements.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Statements {
	// ---------------------------------------------------------------------------
	// Create statements.
	// ---------------------------------------------------------------------------

	constexpr const char *create[CreateCount] = {
		[CreateConferenceEventView] = R"(
			CREATE TEMP VIEW conference_event_view AS
			SELECT id, type, creation_time, chat_room_id, from_sip_address_id, to_sip_address_id, time, imdn_message_id, state, direction, is_secured, notify_id, device_sip_address_id, participant_sip_address_id, subject
			FROM event
			LEFT JOIN conference_event ON conference_event.event_id = event.id
			LEFT JOIN conference_chat_message_event ON conference_chat_message_event.event_id = event.id
			LEFT JOIN conference_notified_event ON conference_notified_event.event_id = event.id
			LEFT JOIN conference_participant_device_event ON conference_participant_device_event.event_id = event.id
			LEFT JOIN conference_participant_event ON conference_participant_event.event_id = event.id
			LEFT JOIN conference_subject_event ON conference_subject_event.event_id = event.id
		)"
	};

	// ---------------------------------------------------------------------------
	// Select statements.
	// ---------------------------------------------------------------------------

	constexpr const char *select[SelectCount] = {
		[SelectConferenceEvents] = R"(
			SELECT conference_event_view.id AS event_id, type, creation_time, from_sip_address.value, to_sip_address.value, time, imdn_message_id, state, direction, is_secured, notify_id, device_sip_address.value, participant_sip_address.value, subject
			FROM conference_event_view
			LEFT JOIN sip_address AS from_sip_address ON from_sip_address.id = from_sip_address_id
			LEFT JOIN sip_address AS to_sip_address ON to_sip_address.id = to_sip_address_id
			LEFT JOIN sip_address AS device_sip_address ON device_sip_address.id = device_sip_address_id
			LEFT JOIN sip_address AS participant_sip_address ON participant_sip_address.id = participant_sip_address_id
			WHERE chat_room_id = :chatRoomId
		)"
	};

	// ---------------------------------------------------------------------------
	// Getters.
	// ---------------------------------------------------------------------------

	const char *get (Create createStmt, AbstractDb::Backend backend) {
		(void)backend;
		return createStmt >= Create::CreateCount ? nullptr : create[createStmt];
	}

	const char *get (Select selectStmt, AbstractDb::Backend backend) {
		(void)backend;
		return selectStmt >= Select::SelectCount ? nullptr : select[selectStmt];
	}
}

LINPHONE_END_NAMESPACE
