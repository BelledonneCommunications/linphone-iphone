/*
 * main-db-p.h
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

#ifndef _L_MAIN_DB_P_H_
#define _L_MAIN_DB_P_H_

#include <unordered_map>

#include "abstract/abstract-db-p.h"
#include "event-log/event-log.h"
#include "main-db.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Content;

class MainDbPrivate : public AbstractDbPrivate {
public:
	struct Statements;

	mutable std::unordered_map<long long, std::weak_ptr<EventLog>> storageIdToEvent;
	mutable std::unordered_map<long long, std::weak_ptr<ChatMessage>> storageIdToChatMessage;

private:
	std::unique_ptr<Statements> statements;

	void initStatements ();

	// ---------------------------------------------------------------------------
	// Low level API.
	// ---------------------------------------------------------------------------

	long long insertSipAddress (const std::string &sipAddress);
	void insertContent (long long messageEventId, const Content &content);
	long long insertContentType (const std::string &contentType);
	long long insertOrUpdateImportedBasicChatRoom (
		long long peerSipAddressId,
		long long localSipAddressId,
		const tm &creationTime
	);
	long long insertChatRoom (const std::shared_ptr<AbstractChatRoom> &chatRoom);
	long long insertChatRoomParticipant (long long chatRoomId, long long participantSipAddressId, bool isAdmin);
	void insertChatRoomParticipantDevice (long long participantId, long long participantDeviceSipAddressId);
	void insertChatMessageParticipant (long long messageEventId, long long sipAddressId, int state);

	long long selectSipAddressId (const std::string &sipAddress) const;
	long long selectChatRoomId (long long peerSipAddressId, long long localSipAddressId) const;
	long long selectChatRoomId (const ChatRoomId &chatRoomId) const;
	long long selectChatRoomParticipantId (long long chatRoomId, long long participantSipAddressId) const;
	long long selectOneToOneChatRoomId (long long sipAddressIdA, long long sipAddressIdB) const;

	void deleteContents (long long messageEventId);
	void deleteChatRoomParticipant (long long chatRoomId, long long participantSipAddressId);
	void deleteChatRoomParticipantDevice (long long participantId, long long participantDeviceSipAddressId);

	// ---------------------------------------------------------------------------
	// Events API.
	// ---------------------------------------------------------------------------

	std::shared_ptr<EventLog> selectGenericConferenceEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	std::shared_ptr<EventLog> selectConferenceEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	std::shared_ptr<EventLog> selectConferenceCallEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	std::shared_ptr<EventLog> selectConferenceChatMessageEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	std::shared_ptr<EventLog> selectConferenceParticipantEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	std::shared_ptr<EventLog> selectConferenceParticipantDeviceEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	std::shared_ptr<EventLog> selectConferenceSubjectEvent (
		long long eventId,
		EventLog::Type type,
		time_t creationTime,
		const ChatRoomId &chatRoomId
	) const;

	long long insertEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceEvent (const std::shared_ptr<EventLog> &eventLog, long long *chatRoomId = nullptr);
	long long insertConferenceCallEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceChatMessageEvent (const std::shared_ptr<EventLog> &eventLog);
	void updateConferenceChatMessageEvent(const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceNotifiedEvent (const std::shared_ptr<EventLog> &eventLog, long long *chatRoomId = nullptr);
	long long insertConferenceParticipantEvent (const std::shared_ptr<EventLog> &eventLog, long long *chatRoomId = nullptr);
	long long insertConferenceParticipantDeviceEvent (const std::shared_ptr<EventLog> &eventLog);
	long long insertConferenceSubjectEvent (const std::shared_ptr<EventLog> &eventLog);

	// ---------------------------------------------------------------------------
	// Cache API.
	// ---------------------------------------------------------------------------

	void cache (const std::shared_ptr<EventLog> &eventLog, long long storageId) const;
	void cache (const std::shared_ptr<ChatMessage> &chatMessage, long long storageId) const;

	std::shared_ptr<EventLog> getEventFromCache (long long storageId) const;
	std::shared_ptr<ChatMessage> getChatMessageFromCache (long long storageId) const;

	void invalidConferenceEventsFromQuery (const std::string &query, long long chatRoomId);

	// ---------------------------------------------------------------------------
	// Versions.
	// ---------------------------------------------------------------------------

	unsigned int getModuleVersion (const std::string &name);
	void updateModuleVersion (const std::string &name, unsigned int version);
	void updateSchema ();

	// ---------------------------------------------------------------------------
	// Import.
	// ---------------------------------------------------------------------------

	void importLegacyFriends (DbSession &inDbSession);
	void importLegacyHistory (DbSession &inDbSession);

	L_DECLARE_PUBLIC(MainDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_MAIN_DB_P_H_
