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
	using Backend = AbstractDb::Backend;

	struct Statement {
		template<size_t N>
		constexpr Statement (Backend _backend, const char (&_sql)[N]) : backend(_backend), sql(_sql) {}

		Backend backend;
		const char *sql;
	};

	struct AbstractStatement {
	public:
		template<size_t N>
		constexpr AbstractStatement (const char (&_sql)[N]) : mSql{ _sql, nullptr } {}

		constexpr AbstractStatement (const Statement &a, const Statement &b) : mSql{ a.sql, b.sql } {}

		const char *getSql (Backend backend) const {
			return backend == Backend::Mysql && mSql[1] ? mSql[1] : mSql[0];
		}

	private:
		const char *mSql[2];
	};

	// ---------------------------------------------------------------------------
	// Create statements.
	// ---------------------------------------------------------------------------

	constexpr AbstractStatement create[CreateCount] = {
		[CreateConferenceEventView] = R"(
			CREATE VIEW IF NOT EXISTS conference_event_view AS
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
		[SelectSipAddressId] = R"(
			SELECT id
			FROM sip_address
			WHERE value = :1
		)",

		[SelectChatRoomId] = R"(
			SELECT id
			FROM chat_room
			WHERE peer_sip_address_id = :1 AND local_sip_address_id = :2
		)",

		[SelectChatRoomParticipantId] = R"(
			SELECT id
			FROM chat_room_participant
			WHERE chat_room_id = :1 AND participant_sip_address_id = :2
		)",

		[SelectOneToOneChatRoomId] = R"(
			SELECT chat_room_id
			FROM one_to_one_chat_room
			WHERE participant_a_sip_address_id IN (:1, :2)
			AND participant_b_sip_address_id IN (:3, :4)
		)",

		[SelectConferenceEvents] = R"(
			SELECT conference_event_view.id AS event_id, type, creation_time, from_sip_address.value, to_sip_address.value, time, imdn_message_id, state, direction, is_secured, notify_id, device_sip_address.value, participant_sip_address.value, subject
			FROM conference_event_view
			LEFT JOIN sip_address AS from_sip_address ON from_sip_address.id = from_sip_address_id
			LEFT JOIN sip_address AS to_sip_address ON to_sip_address.id = to_sip_address_id
			LEFT JOIN sip_address AS device_sip_address ON device_sip_address.id = device_sip_address_id
			LEFT JOIN sip_address AS participant_sip_address ON participant_sip_address.id = participant_sip_address_id
			WHERE chat_room_id = :1
		)"
	};

	// ---------------------------------------------------------------------------
	// Select statements.
	// ---------------------------------------------------------------------------

	constexpr const char *insert[InsertCount] = {
		[InsertOneToOneChatRoom] = R"(
			INSERT INTO one_to_one_chat_room (
				chat_room_id, participant_a_sip_address_id, participant_b_sip_address_id
			) VALUES (:1, :2, :3)
		)"
	};

	// ---------------------------------------------------------------------------
	// Getters.
	// ---------------------------------------------------------------------------

	const char *get (Create createStmt, AbstractDb::Backend backend) {
		(void)backend;
		return createStmt >= Create::CreateCount ? nullptr : create[createStmt].getSql(backend);
	}

	const char *get (Select selectStmt, AbstractDb::Backend backend) {
		(void)backend;
		return selectStmt >= Select::SelectCount ? nullptr : select[selectStmt];
	}

	const char *get (Insert insertStmt, AbstractDb::Backend backend) {
		(void)backend;
		return insertStmt >= Insert::InsertCount ? nullptr : insert[insertStmt];
	}
}

LINPHONE_END_NAMESPACE
