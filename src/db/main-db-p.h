/*
 * main-db-p.h
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

#ifndef _MAIN_DB_P_H_
#define _MAIN_DB_P_H_

#include "abstract/abstract-db-p.h"
#include "event-log/event-log.h"
#include "main-db.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Content;

class MainDbPrivate : public AbstractDbPrivate {
public:
	std::unordered_map<long long, std::weak_ptr<EventLog>> storageIdToEvent;

private:
	// ---------------------------------------------------------------------------
	// Low level API.
	// ---------------------------------------------------------------------------

	long long insertSipAddress (const std::string &sipAddress);
	void insertContent (long long messageEventId, const Content &content);
	long long insertContentType (const std::string &contentType);
	long long insertChatRoom (long long sipAddressId, int capabilities, const tm &date);
	void insertChatRoomParticipant (long long chatRoomId, long long sipAddressId, bool isAdmin);
	void insertChatMessageParticipant (long long messageEventId, long long sipAddressId, int state);

	// ---------------------------------------------------------------------------
	// Events API.
	// ---------------------------------------------------------------------------

	std::shared_ptr<EventLog> selectGenericConferenceEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	std::shared_ptr<EventLog> selectConferenceEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	std::shared_ptr<EventLog> selectConferenceCallEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	std::shared_ptr<EventLog> selectConferenceChatMessageEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	std::shared_ptr<EventLog> selectConferenceParticipantEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	std::shared_ptr<EventLog> selectConferenceParticipantDeviceEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	std::shared_ptr<EventLog> selectConferenceSubjectEvent (
		long long eventId,
		EventLog::Type type,
		time_t date,
		const std::string &peerAddress
	) const;

	long long insertEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceEvent (const std::shared_ptr<EventLog> &eventLog, long long *chatRoomId = nullptr);
	long long insertConferenceCallEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceChatMessageEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceNotifiedEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceParticipantEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceParticipantDeviceEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceSubjectEvent (const std::shared_ptr<EventLog> &eventLog);

	L_DECLARE_PUBLIC(MainDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MAIN_DB_P_H_
