/*
 * main-db.cpp
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

#include <ctime>

#include "linphone/utils/algorithm.h"
#include "linphone/utils/static-string.h"

#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/client-group-chat-room.h"
#include "chat/chat-room/server-group-chat-room.h"
#include "conference/participant-p.h"
#include "core/core-p.h"
#include "event-log/event-log-p.h"
#include "event-log/events.h"
#include "main-db-key-p.h"

#include "internal/db-transaction.h"
#include "internal/statements.h"

// =============================================================================

// See: http://soci.sourceforge.net/doc/3.2/exchange.html
// Part: Object lifetime and immutability

// -----------------------------------------------------------------------------

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace {
	constexpr unsigned int ModuleVersionEvents = makeVersion(1, 0, 1);
	constexpr unsigned int ModuleVersionFriends = makeVersion(1, 0, 0);
	constexpr unsigned int ModuleVersionLegacyFriendsImport = makeVersion(1, 0, 0);
	constexpr unsigned int ModuleVersionLegacyHistoryImport = makeVersion(1, 0, 0);

	constexpr int LegacyFriendListColId = 0;
	constexpr int LegacyFriendListColName = 1;
	constexpr int LegacyFriendListColRlsUri = 2;
	constexpr int LegacyFriendListColSyncUri = 3;
	constexpr int LegacyFriendListColRevision = 4;

	constexpr int LegacyFriendColFriendListId = 1;
	constexpr int LegacyFriendColSipAddress = 2;
	constexpr int LegacyFriendColSubscribePolicy = 3;
	constexpr int LegacyFriendColSendSubscribe = 4;
	constexpr int LegacyFriendColRefKey = 5;
	constexpr int LegacyFriendColVCard = 6;
	constexpr int LegacyFriendColVCardEtag = 7;
	constexpr int LegacyFriendColVCardSyncUri = 8;
	constexpr int LegacyFriendColPresenceReceived = 9;

	constexpr int LegacyMessageColLocalAddress = 1;
	constexpr int LegacyMessageColRemoteAddress = 2;
	constexpr int LegacyMessageColDirection = 3;
	constexpr int LegacyMessageColText = 4;
	constexpr int LegacyMessageColState = 7;
	constexpr int LegacyMessageColUrl = 8;
	constexpr int LegacyMessageColDate = 9;
	constexpr int LegacyMessageColAppData = 10;
	constexpr int LegacyMessageColContentId = 11;
	constexpr int LegacyMessageColContentType = 13;
	constexpr int LegacyMessageColIsSecured = 14;
}

// -----------------------------------------------------------------------------
// soci helpers.
// -----------------------------------------------------------------------------

static inline vector<char> blobToVector (soci::blob &in) {
	size_t len = in.get_len();
	if (!len)
		return vector<char>();
	vector<char> out(len);
	in.read(0, &out[0], len);
	return out;
}

static inline string blobToString (soci::blob &in) {
	vector<char> out = blobToVector(in);
	return string(out.begin(), out.end());
}

static constexpr string &blobToString (string &in) {
	return in;
}

template<typename T>
static T getValueFromRow (const soci::row &row, int index, bool &isNull) {
	isNull = false;

	if (row.get_indicator(size_t(index)) == soci::i_null) {
		isNull = true;
		return T();
	}
	return row.get<T>(size_t(index));
}

// -----------------------------------------------------------------------------
// Event filter tools.
// -----------------------------------------------------------------------------

// Some tools to build filters at compile time.
template<typename T>
struct EnumToSql {
	T first;
	const char *second;
};

template<typename T>
static constexpr const char *mapEnumToSql (const EnumToSql<T> enumToSql[], size_t n, T key) {
	return n == 0 ? "" : (
		enumToSql[n - 1].first == key ? enumToSql[n - 1].second : mapEnumToSql(enumToSql, n - 1, key)
	);
}

template<EventLog::Type ...Type>
struct SqlEventFilterBuilder {};

template<EventLog::Type Type, EventLog::Type... List>
struct SqlEventFilterBuilder<Type, List...> {
	static constexpr Private::StaticString<1 + getIntLength(int(Type)) + sums((1 + getIntLength(int(List)))...)> get () {
		return StaticIntString<int(Type)>() + "," + SqlEventFilterBuilder<List...>::get();
	}
};

template<EventLog::Type Type>
struct SqlEventFilterBuilder<Type> {
	static constexpr Private::StaticString<1 + getIntLength(int(Type))> get () {
		return StaticIntString<int(Type)>();
	}
};

// -----------------------------------------------------------------------------
// Event filters.
// -----------------------------------------------------------------------------

namespace {
	constexpr auto ConferenceCallFilter = SqlEventFilterBuilder<
		EventLog::Type::ConferenceCallStart,
		EventLog::Type::ConferenceCallEnd
	>::get();

	constexpr auto ConferenceChatMessageFilter = SqlEventFilterBuilder<EventLog::Type::ConferenceChatMessage>::get();

	constexpr auto ConferenceInfoNoDeviceFilter = SqlEventFilterBuilder<
		EventLog::Type::ConferenceCreated,
		EventLog::Type::ConferenceTerminated,
		EventLog::Type::ConferenceParticipantAdded,
		EventLog::Type::ConferenceParticipantRemoved,
		EventLog::Type::ConferenceParticipantSetAdmin,
		EventLog::Type::ConferenceParticipantUnsetAdmin,
		EventLog::Type::ConferenceSubjectChanged
	>::get();

	constexpr auto ConferenceInfoFilter = ConferenceInfoNoDeviceFilter + "," + SqlEventFilterBuilder<
		EventLog::Type::ConferenceParticipantDeviceAdded,
		EventLog::Type::ConferenceParticipantDeviceRemoved
	>::get();

	constexpr EnumToSql<MainDb::Filter> EventFilterToSql[] = {
		{ MainDb::ConferenceCallFilter, ConferenceCallFilter },
		{ MainDb::ConferenceChatMessageFilter, ConferenceChatMessageFilter },
		{ MainDb::ConferenceInfoNoDeviceFilter, ConferenceInfoNoDeviceFilter },
		{ MainDb::ConferenceInfoFilter, ConferenceInfoFilter }
	};
}

static const char *mapEventFilterToSql (MainDb::Filter filter) {
	return mapEnumToSql(
		EventFilterToSql, sizeof EventFilterToSql / sizeof EventFilterToSql[0], filter
	);
}

// -----------------------------------------------------------------------------

static string buildSqlEventFilter (
	const list<MainDb::Filter> &filters,
	MainDb::FilterMask mask,
	const string &condKeyWord = "WHERE"
) {
	L_ASSERT(findIf(filters, [](const MainDb::Filter &filter) { return filter == MainDb::NoFilter; }) == filters.cend());

	if (mask == MainDb::NoFilter)
		return "";

	bool isStart = true;
	string sql;
	for (const auto &filter : filters) {
		if (!mask.isSet(filter))
			continue;

		if (isStart) {
			isStart = false;
			sql += " " + condKeyWord + " type IN (";
		} else
			sql += ", ";
		sql += mapEventFilterToSql(filter);
	}

	if (!isStart)
		sql += ") ";

	return sql;
}

// -----------------------------------------------------------------------------
// Misc helpers.
// -----------------------------------------------------------------------------

shared_ptr<AbstractChatRoom> MainDbPrivate::findChatRoom (const ChatRoomId &chatRoomId) const {
	L_Q();
	shared_ptr<AbstractChatRoom> chatRoom = q->getCore()->findChatRoom(chatRoomId);
	if (!chatRoom)
		lError() << "Unable to find chat room: " << chatRoomId << ".";
	return chatRoom;
}

// -----------------------------------------------------------------------------
// Low level API.
// -----------------------------------------------------------------------------

long long MainDbPrivate::insertSipAddress (const string &sipAddress) {
	long long id = selectSipAddressId(sipAddress);
	if (id >= 0)
		return id;

	lInfo() << "Insert new sip address in database: `" << sipAddress << "`.";
	*dbSession.getBackendSession() << "INSERT INTO sip_address (value) VALUES (:sipAddress)", soci::use(sipAddress);
	return dbSession.getLastInsertId();
}

void MainDbPrivate::insertContent (long long chatMessageId, const Content &content) {
	soci::session *session = dbSession.getBackendSession();

	const long long &contentTypeId = insertContentType(content.getContentType().asString());
	const string &body = content.getBodyAsString();
	*session << "INSERT INTO chat_message_content (event_id, content_type_id, body) VALUES"
		" (:chatMessageId, :contentTypeId, :body)", soci::use(chatMessageId), soci::use(contentTypeId),
		soci::use(body);

	const long long &chatMessageContentId = dbSession.getLastInsertId();
	if (content.isFile()) {
		const FileContent &fileContent = static_cast<const FileContent &>(content);
		const string &name = fileContent.getFileName();
		const size_t &size = fileContent.getFileSize();
		const string &path = fileContent.getFilePath();
		*session << "INSERT INTO chat_message_file_content (chat_message_content_id, name, size, path) VALUES"
			" (:chatMessageContentId, :name, :size, :path)",
			soci::use(chatMessageContentId), soci::use(name), soci::use(size), soci::use(path);
	}

	for (const auto &appData : content.getAppDataMap())
		*session << "INSERT INTO chat_message_content_app_data (chat_message_content_id, name, data) VALUES"
			" (:chatMessageContentId, :name, :data)",
			soci::use(chatMessageContentId), soci::use(appData.first), soci::use(appData.second);
}

long long MainDbPrivate::insertContentType (const string &contentType) {
	soci::session *session = dbSession.getBackendSession();

	long long id;
	*session << "SELECT id FROM content_type WHERE value = :contentType", soci::use(contentType), soci::into(id);
	if (session->got_data())
		return id;

	lInfo() << "Insert new content type in database: `" << contentType << "`.";
	*session << "INSERT INTO content_type (value) VALUES (:contentType)", soci::use(contentType);
	return dbSession.getLastInsertId();
}

long long MainDbPrivate::insertOrUpdateImportedBasicChatRoom (
	long long peerSipAddressId,
	long long localSipAddressId,
	const tm &creationTime
) {
	soci::session *session = dbSession.getBackendSession();

	long long id = selectChatRoomId(peerSipAddressId, localSipAddressId);
	if (id >= 0) {
		*session << "UPDATE chat_room SET last_update_time = :lastUpdateTime WHERE id = :id",
			soci::use(creationTime), soci::use(id);
		return id;
	}

	constexpr int capabilities = ChatRoom::CapabilitiesMask(
		{ ChatRoom::Capabilities::Basic, ChatRoom::Capabilities::Migratable }
	);
	lInfo() << "Insert new chat room in database: (peer=" << peerSipAddressId <<
		", local=" << localSipAddressId << ", capabilities=" << capabilities << ").";
	*session << "INSERT INTO chat_room ("
		"  peer_sip_address_id, local_sip_address_id, creation_time, last_update_time, capabilities"
		") VALUES (:peerSipAddressId, :localSipAddressId, :creationTime, :lastUpdateTime, :capabilities)",
		soci::use(peerSipAddressId), soci::use(localSipAddressId), soci::use(creationTime), soci::use(creationTime),
		soci::use(capabilities);

	return dbSession.getLastInsertId();
}

long long MainDbPrivate::insertChatRoom (const shared_ptr<AbstractChatRoom> &chatRoom) {
	const ChatRoomId &chatRoomId = chatRoom->getChatRoomId();
	const long long &peerSipAddressId = insertSipAddress(chatRoomId.getPeerAddress().asString());
	const long long &localSipAddressId = insertSipAddress(chatRoomId.getLocalAddress().asString());

	long long id = selectChatRoomId(peerSipAddressId, localSipAddressId);
	if (id >= 0)
		return id;

	lInfo() << "Insert new chat room in database: " << chatRoomId << ".";

	const tm &creationTime = Utils::getTimeTAsTm(chatRoom->getCreationTime());
	const tm &lastUpdateTime = Utils::getTimeTAsTm(chatRoom->getLastUpdateTime());

	// Remove capabilities like `Proxy`.
	const int &capabilities = chatRoom->getCapabilities() & ~ChatRoom::CapabilitiesMask(ChatRoom::Capabilities::Proxy);

	const string &subject = chatRoom->getSubject();
	const int &flags = chatRoom->hasBeenLeft();
	*dbSession.getBackendSession() << "INSERT INTO chat_room ("
		"  peer_sip_address_id, local_sip_address_id, creation_time, last_update_time, capabilities, subject, flags"
		") VALUES (:peerSipAddressId, :localSipAddressId, :creationTime, :lastUpdateTime, :capabilities, :subject, :flags)",
		soci::use(peerSipAddressId), soci::use(localSipAddressId), soci::use(creationTime), soci::use(lastUpdateTime),
		soci::use(capabilities), soci::use(subject), soci::use(flags);

	id = dbSession.getLastInsertId();
	if (!chatRoom->canHandleParticipants())
		return id;

	// Do not add 'me' when creating a server-group-chat-room.
	if (chatRoomId.getLocalAddress() != chatRoomId.getPeerAddress()) {
		shared_ptr<Participant> me = chatRoom->getMe();
		long long meId = insertChatRoomParticipant(
			id,
			insertSipAddress(me->getAddress().asString()),
			me->isAdmin()
		);
		for (const auto &device : me->getPrivate()->getDevices())
			insertChatRoomParticipantDevice(meId, insertSipAddress(device->getAddress().asString()));
	}

	for (const auto &participant : chatRoom->getParticipants()) {
		long long participantId = insertChatRoomParticipant(
			id,
			insertSipAddress(participant->getAddress().asString()),
			participant->isAdmin()
		);
		for (const auto &device : participant->getPrivate()->getDevices())
			insertChatRoomParticipantDevice(participantId, insertSipAddress(device->getAddress().asString()));
	}

	return id;
}

long long MainDbPrivate::insertChatRoomParticipant (
	long long chatRoomId,
	long long participantSipAddressId,
	bool isAdmin
) {
	soci::session *session = dbSession.getBackendSession();
	long long id = selectChatRoomParticipantId(chatRoomId, participantSipAddressId);
	if (id >= 0) {
		// See: https://stackoverflow.com/a/15299655 (cast to reference)
		*session << "UPDATE chat_room_participant SET is_admin = :isAdmin WHERE id = :id",
			soci::use(static_cast<const int &>(isAdmin)), soci::use(id);
		return id;
	}

	*session << "INSERT INTO chat_room_participant (chat_room_id, participant_sip_address_id, is_admin)"
		" VALUES (:chatRoomId, :participantSipAddressId, :isAdmin)",
		soci::use(chatRoomId), soci::use(participantSipAddressId), soci::use(static_cast<const int &>(isAdmin));

	return dbSession.getLastInsertId();
}

void MainDbPrivate::insertChatRoomParticipantDevice (
	long long participantId,
	long long participantDeviceSipAddressId
) {
	soci::session *session = dbSession.getBackendSession();
	long long count;
	*session << "SELECT COUNT(*) FROM chat_room_participant_device"
		" WHERE chat_room_participant_id = :participantId"
		" AND participant_device_sip_address_id = :participantDeviceSipAddressId",
		soci::into(count), soci::use(participantId), soci::use(participantDeviceSipAddressId);
	if (count)
		return;

	*session << "INSERT INTO chat_room_participant_device (chat_room_participant_id, participant_device_sip_address_id)"
		" VALUES (:participantId, :participantDeviceSipAddressId)",
		soci::use(participantId), soci::use(participantDeviceSipAddressId);
}

void MainDbPrivate::insertChatMessageParticipant (long long chatMessageId, long long sipAddressId, int state) {
	if (state != static_cast<int>(ChatMessage::State::Displayed))
		*dbSession.getBackendSession() <<
			"INSERT INTO chat_message_participant (event_id, participant_sip_address_id, state)"
			" VALUES (:chatMessageId, :sipAddressId, :state)",
			soci::use(chatMessageId), soci::use(sipAddressId), soci::use(state);
}

// -----------------------------------------------------------------------------

long long MainDbPrivate::selectSipAddressId (const string &sipAddress) const {
	long long id;

	soci::session *session = dbSession.getBackendSession();
	*session << Statements::get(Statements::SelectSipAddressId),
		soci::use(sipAddress), soci::into(id);

	return session->got_data() ? id : -1;
}

long long MainDbPrivate::selectChatRoomId (long long peerSipAddressId, long long localSipAddressId) const {
	long long id;

	soci::session *session = dbSession.getBackendSession();
	*session << Statements::get(Statements::SelectChatRoomId),
		soci::use(peerSipAddressId), soci::use(localSipAddressId), soci::into(id);

	return session->got_data() ? id : -1;
}

long long MainDbPrivate::selectChatRoomId (const ChatRoomId &chatRoomId) const {
	long long peerSipAddressId = selectSipAddressId(chatRoomId.getPeerAddress().asString());
	if (peerSipAddressId < 0)
		return -1;

	long long localSipAddressId = selectSipAddressId(chatRoomId.getLocalAddress().asString());
	if (localSipAddressId < 0)
		return -1;

	return selectChatRoomId(peerSipAddressId, localSipAddressId);
}

long long MainDbPrivate::selectChatRoomParticipantId (long long chatRoomId, long long participantSipAddressId) const {
	long long id;

	soci::session *session = dbSession.getBackendSession();
	*session << Statements::get(Statements::SelectChatRoomParticipantId),
		soci::use(chatRoomId), soci::use(participantSipAddressId), soci::into(id);

	return session->got_data() ? id : -1;
}

long long MainDbPrivate::selectOneToOneChatRoomId (long long sipAddressIdA, long long sipAddressIdB) const {
	long long id;

	soci::session *session = dbSession.getBackendSession();
	*session << Statements::get(Statements::SelectOneToOneChatRoomId),
		soci::use(sipAddressIdA), soci::use(sipAddressIdB), soci::use(sipAddressIdA), soci::use(sipAddressIdB),
		soci::into(id);

	return session->got_data() ? id : -1;
}

// -----------------------------------------------------------------------------

void MainDbPrivate::deleteContents (long long chatMessageId) {
	*dbSession.getBackendSession() << "DELETE FROM chat_message_content WHERE event_id = :chatMessageId",
		soci::use(chatMessageId);
}

void MainDbPrivate::deleteChatRoomParticipant (long long chatRoomId, long long participantSipAddressId) {
	*dbSession.getBackendSession() << "DELETE FROM chat_room_participant"
		" WHERE chat_room_id = :chatRoomId AND participant_sip_address_id = :participantSipAddressId",
		soci::use(chatRoomId), soci::use(participantSipAddressId);
}

void MainDbPrivate::deleteChatRoomParticipantDevice (
	long long participantId,
	long long participantDeviceSipAddressId
) {
	*dbSession.getBackendSession() << "DELETE FROM chat_room_participant_device"
		" WHERE chat_room_participant_id = :participantId"
		" AND participant_device_sip_address_id = :participantDeviceSipAddressId",
		soci::use(participantId), soci::use(participantDeviceSipAddressId);
}

// -----------------------------------------------------------------------------
// Events API.
// -----------------------------------------------------------------------------

shared_ptr<EventLog> MainDbPrivate::selectGenericConferenceEvent (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const soci::row &row
) const {
	EventLog::Type type = EventLog::Type(row.get<int>(1));
	if (type == EventLog::Type::ConferenceChatMessage) {
		long long eventId = getConferenceEventIdFromRow(row);
		shared_ptr<EventLog> eventLog = getEventFromCache(eventId);
		if (!eventLog) {
			eventLog = selectConferenceChatMessageEvent(chatRoom, type, row);
			if (eventLog)
				cache(eventLog, eventId);
		}
		return eventLog;
	}

	return selectGenericConferenceNotifiedEvent(chatRoom->getChatRoomId(), row);
}

shared_ptr<EventLog> MainDbPrivate::selectGenericConferenceNotifiedEvent (
	const ChatRoomId &chatRoomId,
	const soci::row &row
) const {
	long long eventId = getConferenceEventIdFromRow(row);
	shared_ptr<EventLog> eventLog = getEventFromCache(eventId);
	if (eventLog)
		return eventLog;

	EventLog::Type type = EventLog::Type(row.get<int>(1));
	switch (type) {
		case EventLog::Type::None:
		case EventLog::Type::ConferenceChatMessage:
			return nullptr;

		case EventLog::Type::ConferenceCreated:
		case EventLog::Type::ConferenceTerminated:
			eventLog = selectConferenceEvent(chatRoomId, type, row);
			break;

		case EventLog::Type::ConferenceCallStart:
		case EventLog::Type::ConferenceCallEnd:
			eventLog = selectConferenceCallEvent(chatRoomId, type, row);
			break;

		case EventLog::Type::ConferenceParticipantAdded:
		case EventLog::Type::ConferenceParticipantRemoved:
		case EventLog::Type::ConferenceParticipantSetAdmin:
		case EventLog::Type::ConferenceParticipantUnsetAdmin:
			eventLog = selectConferenceParticipantEvent(chatRoomId, type, row);
			break;

		case EventLog::Type::ConferenceParticipantDeviceAdded:
		case EventLog::Type::ConferenceParticipantDeviceRemoved:
			eventLog = selectConferenceParticipantDeviceEvent(chatRoomId, type, row);
			break;

		case EventLog::Type::ConferenceSubjectChanged:
			eventLog = selectConferenceSubjectEvent(chatRoomId, type, row);
			break;
	}

	if (eventLog)
		cache(eventLog, eventId);

	return eventLog;
}

shared_ptr<EventLog> MainDbPrivate::selectConferenceEvent (
	const ChatRoomId &chatRoomId,
	EventLog::Type type,
	const soci::row &row
) const {
	return make_shared<ConferenceEvent>(
		type,
		getConferenceEventCreationTimeFromRow(row),
		chatRoomId
	);
}

shared_ptr<EventLog> MainDbPrivate::selectConferenceCallEvent (
	const ChatRoomId &chatRoomId,
	EventLog::Type type,
	const soci::row &row
) const {
	// TODO.
	return nullptr;
}

shared_ptr<EventLog> MainDbPrivate::selectConferenceChatMessageEvent (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	EventLog::Type type,
	const soci::row &row
) const {
	long long eventId = getConferenceEventIdFromRow(row);
	shared_ptr<ChatMessage> chatMessage = getChatMessageFromCache(eventId);
	if (!chatMessage) {
		chatMessage = shared_ptr<ChatMessage>(new ChatMessage(
			chatRoom,
			ChatMessage::Direction(row.get<int>(8))
		));
		chatMessage->setIsSecured(bool(row.get<int>(9)));

		ChatMessagePrivate *dChatMessage = chatMessage->getPrivate();
		dChatMessage->setState(ChatMessage::State(row.get<int>(7)), true);

		dChatMessage->forceFromAddress(IdentityAddress(row.get<string>(3)));
		dChatMessage->forceToAddress(IdentityAddress(row.get<string>(4)));

		dChatMessage->setTime(Utils::getTmAsTimeT(row.get<tm>(5)));
		dChatMessage->setImdnMessageId(row.get<string>(6));

		dChatMessage->markContentsAsNotLoaded();
		dChatMessage->setIsReadOnly(true);

		cache(chatMessage, eventId);
	}

	return make_shared<ConferenceChatMessageEvent>(
		getConferenceEventCreationTimeFromRow(row),
		chatMessage
	);
}

shared_ptr<EventLog> MainDbPrivate::selectConferenceParticipantEvent (
	const ChatRoomId &chatRoomId,
	EventLog::Type type,
	const soci::row &row
) const {
	return make_shared<ConferenceParticipantEvent>(
		type,
		getConferenceEventCreationTimeFromRow(row),
		chatRoomId,
		getConferenceEventNotifyIdFromRow(row),
		IdentityAddress(row.get<string>(12))
	);
}

shared_ptr<EventLog> MainDbPrivate::selectConferenceParticipantDeviceEvent (
	const ChatRoomId &chatRoomId,
	EventLog::Type type,
	const soci::row &row
) const {
	return make_shared<ConferenceParticipantDeviceEvent>(
		type,
		getConferenceEventCreationTimeFromRow(row),
		chatRoomId,
		getConferenceEventNotifyIdFromRow(row),
		IdentityAddress(row.get<string>(12)),
		IdentityAddress(row.get<string>(11))
	);
}

shared_ptr<EventLog> MainDbPrivate::selectConferenceSubjectEvent (
	const ChatRoomId &chatRoomId,
	EventLog::Type type,
	const soci::row &row
) const {
	return make_shared<ConferenceSubjectEvent>(
		getConferenceEventCreationTimeFromRow(row),
		chatRoomId,
		getConferenceEventNotifyIdFromRow(row),
		row.get<string>(13)
	);
}

// -----------------------------------------------------------------------------

long long MainDbPrivate::insertEvent (const shared_ptr<EventLog> &eventLog) {
	const int &type = int(eventLog->getType());
	const tm &creationTime = Utils::getTimeTAsTm(eventLog->getCreationTime());
	*dbSession.getBackendSession() << "INSERT INTO event (type, creation_time) VALUES (:type, :creationTime)",
		soci::use(type), soci::use(creationTime);

	return dbSession.getLastInsertId();
}

long long MainDbPrivate::insertConferenceEvent (const shared_ptr<EventLog> &eventLog, long long *chatRoomId) {
	shared_ptr<ConferenceEvent> conferenceEvent = static_pointer_cast<ConferenceEvent>(eventLog);

	long long eventId = -1;
	const long long &curChatRoomId = selectChatRoomId(conferenceEvent->getChatRoomId());
	if (curChatRoomId < 0) {
		// A conference event can be inserted in database only if chat room exists.
		// Otherwise it's an error.
		const ChatRoomId &chatRoomId = conferenceEvent->getChatRoomId();
		lError() << "Unable to find chat room storage id of: " << chatRoomId << ".";
	} else {
		eventId = insertEvent(eventLog);

		soci::session *session = dbSession.getBackendSession();
		*session << "INSERT INTO conference_event (event_id, chat_room_id)"
			" VALUES (:eventId, :chatRoomId)", soci::use(eventId), soci::use(curChatRoomId);

		const tm &lastUpdateTime = Utils::getTimeTAsTm(eventLog->getCreationTime());
		*session << "UPDATE chat_room SET last_update_time = :lastUpdateTime"
			" WHERE id = :chatRoomId", soci::use(lastUpdateTime),
			soci::use(curChatRoomId);

		if (eventLog->getType() == EventLog::Type::ConferenceTerminated)
			*session << "UPDATE chat_room SET flags = 1 WHERE id = :chatRoomId", soci::use(curChatRoomId);
	}

	if (chatRoomId)
		*chatRoomId = curChatRoomId;

	return eventId;
}

long long MainDbPrivate::insertConferenceCallEvent (const shared_ptr<EventLog> &eventLog) {
	// TODO.
	return 0;
}

long long MainDbPrivate::insertConferenceChatMessageEvent (const shared_ptr<EventLog> &eventLog) {
	const long long &eventId = insertConferenceEvent(eventLog);
	if (eventId < 0)
		return -1;

	shared_ptr<ChatMessage> chatMessage = static_pointer_cast<ConferenceChatMessageEvent>(eventLog)->getChatMessage();
	const long long &fromSipAddressId = insertSipAddress(chatMessage->getFromAddress().asString());
	const long long &toSipAddressId = insertSipAddress(chatMessage->getToAddress().asString());
	const tm &messageTime = Utils::getTimeTAsTm(chatMessage->getTime());
	const int &state = int(chatMessage->getState());
	const int &direction = int(chatMessage->getDirection());
	const string &imdnMessageId = chatMessage->getImdnMessageId();
	const int &isSecured = chatMessage->isSecured() ? 1 : 0;

	*dbSession.getBackendSession() << "INSERT INTO conference_chat_message_event ("
		"  event_id, from_sip_address_id, to_sip_address_id,"
		"  time, state, direction, imdn_message_id, is_secured"
		") VALUES ("
		"  :eventId, :localSipaddressId, :remoteSipaddressId,"
		"  :time, :state, :direction, :imdnMessageId, :isSecured"
		")", soci::use(eventId), soci::use(fromSipAddressId), soci::use(toSipAddressId),
		soci::use(messageTime), soci::use(state), soci::use(direction),
		soci::use(imdnMessageId), soci::use(isSecured);

	for (const Content *content : chatMessage->getContents())
		insertContent(eventId, *content);

	for (const auto &participant : chatMessage->getChatRoom()->getParticipants()) {
		const long long &participantSipAddressId = selectSipAddressId(participant->getAddress().asString());
		insertChatMessageParticipant(eventId, participantSipAddressId, state);
	}

	return eventId;
}

void MainDbPrivate::updateConferenceChatMessageEvent (const shared_ptr<EventLog> &eventLog) {
	shared_ptr<ChatMessage> chatMessage = static_pointer_cast<ConferenceChatMessageEvent>(eventLog)->getChatMessage();

	const EventLogPrivate *dEventLog = eventLog->getPrivate();
	MainDbKeyPrivate *dEventKey = static_cast<MainDbKey &>(dEventLog->dbKey).getPrivate();
	const long long &eventId = dEventKey->storageId;

	const int &state = int(chatMessage->getState());
	const string &imdnMessageId = chatMessage->getImdnMessageId();
	*dbSession.getBackendSession() << "UPDATE conference_chat_message_event SET state = :state, imdn_message_id = :imdnMessageId"
		" WHERE event_id = :eventId",
		soci::use(state), soci::use(imdnMessageId), soci::use(eventId);

	deleteContents(eventId);
	for (const auto &content : chatMessage->getContents())
		insertContent(eventId, *content);
}

long long MainDbPrivate::insertConferenceNotifiedEvent (const shared_ptr<EventLog> &eventLog, long long *chatRoomId) {
	long long curChatRoomId;
	const long long &eventId = insertConferenceEvent(eventLog, &curChatRoomId);
	if (eventId < 0)
		return -1;

	const unsigned int &lastNotifyId = static_pointer_cast<ConferenceNotifiedEvent>(eventLog)->getNotifyId();

	soci::session *session = dbSession.getBackendSession();
	*session << "INSERT INTO conference_notified_event (event_id, notify_id)"
		" VALUES (:eventId, :notifyId)", soci::use(eventId), soci::use(lastNotifyId);
	*session << "UPDATE chat_room SET last_notify_id = :lastNotifyId WHERE id = :chatRoomId",
		soci::use(lastNotifyId), soci::use(curChatRoomId);

	if (chatRoomId)
		*chatRoomId = curChatRoomId;

	return eventId;
}

long long MainDbPrivate::insertConferenceParticipantEvent (
	const shared_ptr<EventLog> &eventLog,
	long long *chatRoomId
) {
	long long curChatRoomId;
	const long long &eventId = insertConferenceNotifiedEvent(eventLog, &curChatRoomId);
	if (eventId < 0)
		return -1;

	shared_ptr<ConferenceParticipantEvent> participantEvent =
		static_pointer_cast<ConferenceParticipantEvent>(eventLog);

	const long long &participantAddressId = insertSipAddress(
		participantEvent->getParticipantAddress().asString()
	);

	*dbSession.getBackendSession() << "INSERT INTO conference_participant_event (event_id, participant_sip_address_id)"
		" VALUES (:eventId, :participantAddressId)", soci::use(eventId), soci::use(participantAddressId);

	bool isAdmin = eventLog->getType() == EventLog::Type::ConferenceParticipantSetAdmin;
	switch (eventLog->getType()) {
		case EventLog::Type::ConferenceParticipantAdded:
		case EventLog::Type::ConferenceParticipantSetAdmin:
		case EventLog::Type::ConferenceParticipantUnsetAdmin:
			insertChatRoomParticipant(curChatRoomId, participantAddressId, isAdmin);
			break;

		case EventLog::Type::ConferenceParticipantRemoved:
			deleteChatRoomParticipant(curChatRoomId, participantAddressId);
			break;

		default:
			break;
	}

	if (chatRoomId)
		*chatRoomId = curChatRoomId;

	return eventId;
}

long long MainDbPrivate::insertConferenceParticipantDeviceEvent (const shared_ptr<EventLog> &eventLog) {
	long long chatRoomId;
	const long long &eventId = insertConferenceParticipantEvent(eventLog, &chatRoomId);
	if (eventId < 0)
		return -1;

	shared_ptr<ConferenceParticipantDeviceEvent> participantDeviceEvent =
		static_pointer_cast<ConferenceParticipantDeviceEvent>(eventLog);

	const string participantAddress = participantDeviceEvent->getParticipantAddress().asString();
	const long long &participantAddressId = selectSipAddressId(participantAddress);
	if (participantAddressId < 0) {
		lError() << "Unable to find sip address id of: `" << participantAddress << "`.";
		return -1;
	}
	const long long &participantId = selectChatRoomParticipantId(chatRoomId, participantAddressId);
	if (participantId < 0) {
		lError() << "Unable to find valid participant id in database with chat room id = " << chatRoomId <<
		" and participant address id = " << participantAddressId;
		return -1;
	}
	const long long &deviceAddressId = insertSipAddress(
		participantDeviceEvent->getDeviceAddress().asString()
	);

	*dbSession.getBackendSession() << "INSERT INTO conference_participant_device_event (event_id, device_sip_address_id)"
		" VALUES (:eventId, :deviceAddressId)", soci::use(eventId), soci::use(deviceAddressId);

	switch (eventLog->getType()) {
		case EventLog::Type::ConferenceParticipantDeviceAdded:
			insertChatRoomParticipantDevice(participantId, deviceAddressId);
			break;

		case EventLog::Type::ConferenceParticipantDeviceRemoved:
			deleteChatRoomParticipantDevice(participantId, deviceAddressId);
			break;

		default:
			break;
	}

	return eventId;
}

long long MainDbPrivate::insertConferenceSubjectEvent (const shared_ptr<EventLog> &eventLog) {
	long long chatRoomId;
	const long long &eventId = insertConferenceNotifiedEvent(eventLog, &chatRoomId);
	if (eventId < 0)
		return -1;

	const string &subject = static_pointer_cast<ConferenceSubjectEvent>(eventLog)->getSubject();

	soci::session *session = dbSession.getBackendSession();
	*session << "INSERT INTO conference_subject_event (event_id, subject)"
		" VALUES (:eventId, :subject)", soci::use(eventId), soci::use(subject);

	*session << "UPDATE chat_room SET subject = :subject"
		" WHERE id = :chatRoomId", soci::use(subject), soci::use(chatRoomId);

	return eventId;
}

// -----------------------------------------------------------------------------
// Cache API.
// -----------------------------------------------------------------------------

shared_ptr<EventLog> MainDbPrivate::getEventFromCache (long long storageId) const {
	auto it = storageIdToEvent.find(storageId);
	if (it == storageIdToEvent.cend())
		return nullptr;

	shared_ptr<EventLog> eventLog = it->second.lock();
	L_ASSERT(eventLog);
	return eventLog;
}

shared_ptr<ChatMessage> MainDbPrivate::getChatMessageFromCache (long long storageId) const {
	auto it = storageIdToChatMessage.find(storageId);
	if (it == storageIdToChatMessage.cend())
		return nullptr;

	shared_ptr<ChatMessage> chatMessage = it->second.lock();
	L_ASSERT(chatMessage);
	return chatMessage;
}

void MainDbPrivate::cache (const shared_ptr<EventLog> &eventLog, long long storageId) const {
	L_Q();

	EventLogPrivate *dEventLog = eventLog->getPrivate();
	L_ASSERT(!dEventLog->dbKey.isValid());
	dEventLog->dbKey = MainDbEventKey(q->getCore(), storageId);
	storageIdToEvent[storageId] = eventLog;
	L_ASSERT(dEventLog->dbKey.isValid());
}

void MainDbPrivate::cache (const shared_ptr<ChatMessage> &chatMessage, long long storageId) const {
	L_Q();

	ChatMessagePrivate *dChatMessage = chatMessage->getPrivate();
	L_ASSERT(!dChatMessage->dbKey.isValid());
	dChatMessage->dbKey = MainDbChatMessageKey(q->getCore(), storageId);
	storageIdToChatMessage[storageId] = chatMessage;
	L_ASSERT(dChatMessage->dbKey.isValid());
}

void MainDbPrivate::invalidConferenceEventsFromQuery (const string &query, long long chatRoomId) {
	soci::rowset<soci::row> rows = (dbSession.getBackendSession()->prepare << query, soci::use(chatRoomId));
	for (const auto &row : rows) {
		long long eventId = dbSession.resolveId(row, 0);
		shared_ptr<EventLog> eventLog = getEventFromCache(eventId);
		if (eventLog) {
			const EventLogPrivate *dEventLog = eventLog->getPrivate();
			L_ASSERT(dEventLog->dbKey.isValid());
			dEventLog->dbKey = MainDbEventKey();
		}
		shared_ptr<ChatMessage> chatMessage = getChatMessageFromCache(eventId);
		if (chatMessage) {
			const ChatMessagePrivate *dChatMessage = chatMessage->getPrivate();
			L_ASSERT(dChatMessage->dbKey.isValid());
			dChatMessage->dbKey = MainDbChatMessageKey();
		}
	}
}

// -----------------------------------------------------------------------------
// Versions.
// -----------------------------------------------------------------------------

unsigned int MainDbPrivate::getModuleVersion (const string &name) {
	soci::session *session = dbSession.getBackendSession();

	unsigned int version;
	*session << "SELECT version FROM db_module_version WHERE name = :name", soci::into(version), soci::use(name);
	return session->got_data() ? version : 0;
}

void MainDbPrivate::updateModuleVersion (const string &name, unsigned int version) {
	unsigned int oldVersion = getModuleVersion(name);
	if (oldVersion == version)
		return;

	soci::session *session = dbSession.getBackendSession();
	*session << "REPLACE INTO db_module_version (name, version) VALUES (:name, :version)",
		soci::use(name), soci::use(version);
}

void MainDbPrivate::updateSchema () {
	soci::session *session = dbSession.getBackendSession();
	unsigned int version = getModuleVersion("events");

	if (version < makeVersion(1, 0, 1))
		*session << "ALTER TABLE chat_room_participant_device ADD COLUMN state TINYINT UNSIGNED DEFAULT 0";
}

// -----------------------------------------------------------------------------
// Import.
// -----------------------------------------------------------------------------

static inline bool checkLegacyTableExists (soci::session &session, const string &name) {
	session << "SELECT name FROM sqlite_master WHERE type='table' AND name = :name", soci::use(name);
	return session.got_data() > 0;
}

static inline bool checkLegacyFriendsTableExists (soci::session &session) {
	return checkLegacyTableExists(session, "friends");
}

static inline bool checkLegacyHistoryTableExists (soci::session &session) {
	return checkLegacyTableExists(session, "history");
}

void MainDbPrivate::importLegacyFriends (DbSession &inDbSession) {
	L_Q();
	L_DB_TRANSACTION_C(q) {
		if (getModuleVersion("legacy-friends-import") >= makeVersion(1, 0, 0))
			return;
		updateModuleVersion("legacy-friends-import", ModuleVersionLegacyFriendsImport);

		soci::session *inSession = inDbSession.getBackendSession();
		if (!checkLegacyFriendsTableExists(*inSession))
			return;

		unordered_map<int, long long> resolvedListsIds;
		soci::session *session = dbSession.getBackendSession();

		soci::rowset<soci::row> friendsLists = (inSession->prepare << "SELECT * FROM friends_lists");

		set<string> names;
		for (const auto &friendList : friendsLists) {
			const string &name = friendList.get<string>(LegacyFriendListColName, "");
			const string &rlsUri = friendList.get<string>(LegacyFriendListColRlsUri, "");
			const string &syncUri = friendList.get<string>(LegacyFriendListColSyncUri, "");
			const int &revision = friendList.get<int>(LegacyFriendListColRevision, 0);

			string uniqueName = name;
			for (int id = 0; names.find(uniqueName) != names.end(); uniqueName = name + "-" + Utils::toString(id++));
			names.insert(uniqueName);

			*session << "INSERT INTO friends_list (name, rls_uri, sync_uri, revision) VALUES ("
				"  :name, :rlsUri, :syncUri, :revision"
				")", soci::use(uniqueName), soci::use(rlsUri), soci::use(syncUri), soci::use(revision);
			resolvedListsIds[friendList.get<int>(LegacyFriendListColId)] = dbSession.getLastInsertId();
		}

		soci::rowset<soci::row> friends = (inSession->prepare << "SELECT * FROM friends");
		for (const auto &friendInfo : friends) {
			long long friendsListId;
			{
				auto it = resolvedListsIds.find(friendInfo.get<int>(LegacyFriendColFriendListId, -1));
				if (it == resolvedListsIds.end())
					continue;
				friendsListId = it->second;
			}

			const long long &sipAddressId = insertSipAddress(friendInfo.get<string>(LegacyFriendColSipAddress, ""));
			const int &subscribePolicy = friendInfo.get<int>(LegacyFriendColSubscribePolicy, LinphoneSPAccept);
			const int &sendSubscribe = friendInfo.get<int>(LegacyFriendColSendSubscribe, 1);
			const string &vCard = friendInfo.get<string>(LegacyFriendColVCard, "");
			const string &vCardEtag = friendInfo.get<string>(LegacyFriendColVCardEtag, "");
			const string &vCardSyncUri = friendInfo.get<string>(LegacyFriendColVCardSyncUri, "");
			const int &presenceReveived = friendInfo.get<int>(LegacyFriendColPresenceReceived, 0);

			*session << "INSERT INTO friend ("
				"  sip_address_id, friends_list_id, subscribe_policy, send_subscribe,"
				"  presence_received, v_card, v_card_etag, v_card_sync_uri"
				") VALUES ("
				"  :sipAddressId, :friendsListId, :subscribePolicy, :sendSubscribe,"
				"  :presenceReceived, :vCard, :vCardEtag, :vCardSyncUri"
				")", soci::use(sipAddressId), soci::use(friendsListId), soci::use(subscribePolicy), soci::use(sendSubscribe),
				soci::use(presenceReveived), soci::use(vCard), soci::use(vCardEtag), soci::use(vCardSyncUri);

			bool isNull;
			const string &data = getValueFromRow<string>(friendInfo, LegacyFriendColRefKey, isNull);
			if (!isNull)
				*session << "INSERT INTO friend_app_data (friend_id, name, data) VALUES"
					" (:friendId, 'legacy', :data)",
					soci::use(dbSession.getLastInsertId()), soci::use(data);
		}
		tr.commit();
		lInfo() << "Successful import of legacy friends.";
	};
}

void MainDbPrivate::importLegacyHistory (DbSession &inDbSession) {
	L_Q();
	L_DB_TRANSACTION_C(q) {
		if (getModuleVersion("legacy-history-import") >= makeVersion(1, 0, 0))
			return;
		updateModuleVersion("legacy-history-import", ModuleVersionLegacyHistoryImport);

		soci::session *inSession = inDbSession.getBackendSession();
		if (!checkLegacyHistoryTableExists(*inSession))
			return;

		soci::rowset<soci::row> messages = (inSession->prepare << "SELECT * FROM history");
		for (const auto &message : messages) {
			const int direction = message.get<int>(LegacyMessageColDirection);
			if (direction != 0 && direction != 1) {
				lWarning() << "Unable to import legacy message with invalid direction.";
				continue;
			}

			const int &state = message.get<int>(
				LegacyMessageColState, int(ChatMessage::State::Displayed)
			);
			if (state < 0 || state > int(ChatMessage::State::Displayed)) {
				lWarning() << "Unable to import legacy message with invalid state.";
				continue;
			}

			const tm &creationTime = Utils::getTimeTAsTm(message.get<int>(LegacyMessageColDate, 0));

			bool isNull;
			getValueFromRow<string>(message, LegacyMessageColUrl, isNull);

			const int &contentId = message.get<int>(LegacyMessageColContentId, -1);
			ContentType contentType(message.get<string>(LegacyMessageColContentType, ""));
			if (!contentType.isValid())
				contentType = contentId != -1
					? ContentType::FileTransfer
					: (isNull ? ContentType::PlainText : ContentType::ExternalBody);
			if (contentType == ContentType::ExternalBody) {
				lInfo() << "Import of external body content is skipped.";
				continue;
			}

			const string &text = getValueFromRow<string>(message, LegacyMessageColText, isNull);

			Content content;
			content.setContentType(contentType);
			if (contentType == ContentType::PlainText) {
				if (isNull) {
					lWarning() << "Unable to import legacy message with no text.";
					continue;
				}
				content.setBody(text);
			} else {
				if (contentType != ContentType::FileTransfer) {
					lWarning() << "Unable to import unsupported legacy content.";
					continue;
				}

				const string appData = getValueFromRow<string>(message, LegacyMessageColAppData, isNull);
				if (isNull) {
					lWarning() << "Unable to import legacy file message without app data.";
					continue;
				}

				content.setAppData("legacy", appData);
			}

			soci::session *session = dbSession.getBackendSession();
			const int &eventType = int(EventLog::Type::ConferenceChatMessage);
			*session << "INSERT INTO event (type, creation_time) VALUES (:type, :creationTime)",
				soci::use(eventType), soci::use(creationTime);

			const long long &eventId = dbSession.getLastInsertId();
			const long long &localSipAddressId = insertSipAddress(
				IdentityAddress(message.get<string>(LegacyMessageColLocalAddress)).asString()
			);
			const long long &remoteSipAddressId = insertSipAddress(
				IdentityAddress(message.get<string>(LegacyMessageColRemoteAddress)).asString()
			);
			const long long &chatRoomId = insertOrUpdateImportedBasicChatRoom(
				remoteSipAddressId,
				localSipAddressId,
				creationTime
			);
			const int &isSecured = message.get<int>(LegacyMessageColIsSecured, 0);

			*session << "INSERT INTO conference_event (event_id, chat_room_id)"
				" VALUES (:eventId, :chatRoomId)", soci::use(eventId), soci::use(chatRoomId);

			*session << "INSERT INTO conference_chat_message_event ("
				"  event_id, from_sip_address_id, to_sip_address_id,"
				"  time, state, direction, imdn_message_id, is_secured"
				") VALUES ("
				"  :eventId, :localSipAddressId, :remoteSipAddressId,"
				"  :creationTime, :state, :direction, '', :isSecured"
				")", soci::use(eventId), soci::use(localSipAddressId), soci::use(remoteSipAddressId),
				soci::use(creationTime), soci::use(state), soci::use(direction),
				soci::use(isSecured);

			insertContent(eventId, content);
			insertChatRoomParticipant(chatRoomId, remoteSipAddressId, false);

			if (state != int(ChatMessage::State::Displayed))
				insertChatMessageParticipant(eventId, remoteSipAddressId, state);
		}
		tr.commit();
		lInfo() << "Successful import of legacy messages.";
	};
}

// =============================================================================

MainDb::MainDb (const shared_ptr<Core> &core) : AbstractDb(*new MainDbPrivate), CoreAccessor(core) {}

void MainDb::init () {
	L_D();

	Backend backend = getBackend();

	const string charset = backend == Mysql ? "DEFAULT CHARSET=utf8" : "";
	soci::session *session = d->dbSession.getBackendSession();

	using namespace placeholders;
	auto primaryKeyRefStr = bind(&DbSession::primaryKeyRefStr, &d->dbSession, _1);
	auto primaryKeyStr = bind(&DbSession::primaryKeyStr, &d->dbSession, _1);
	auto timestampType = bind(&DbSession::timestampType, &d->dbSession);
	auto varcharPrimaryKeyStr = bind(&DbSession::varcharPrimaryKeyStr, &d->dbSession, _1);

	*session <<
		"CREATE TABLE IF NOT EXISTS sip_address ("
		"  id" + primaryKeyStr("BIGINT UNSIGNED") + ","
		"  value VARCHAR(255) UNIQUE NOT NULL"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS content_type ("
		"  id" + primaryKeyStr("SMALLINT UNSIGNED") + ","
		"  value VARCHAR(255) UNIQUE NOT NULL"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS event ("
		"  id" + primaryKeyStr("BIGINT UNSIGNED") + ","
		"  type TINYINT UNSIGNED NOT NULL,"
		"  creation_time" + timestampType() + " NOT NULL"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_room ("
		"  id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		// Server (for conference) or user sip address.
		"  peer_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

		"  local_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

		// Dialog creation time.
		"  creation_time" + timestampType() + " NOT NULL,"

		// Last event time (call, message...).
		"  last_update_time" + timestampType() + " NOT NULL,"

		// ConferenceChatRoom, BasicChatRoom, RTT...
		"  capabilities TINYINT UNSIGNED NOT NULL,"

		// Chatroom subject.
		"  subject VARCHAR(255),"

		"  last_notify_id INT UNSIGNED DEFAULT 0,"

		"  flags INT UNSIGNED DEFAULT 0,"

		"  UNIQUE (peer_sip_address_id, local_sip_address_id),"

		"  FOREIGN KEY (peer_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (local_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

		*session <<
			"CREATE TABLE IF NOT EXISTS one_to_one_chat_room ("
			"  chat_room_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

			"  participant_a_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"
			"  participant_b_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

			"  FOREIGN KEY (chat_room_id)"
			"    REFERENCES chat_room(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (participant_a_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (participant_b_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_room_participant ("
		"  id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  chat_room_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","
		"  participant_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","

		"  is_admin BOOLEAN NOT NULL,"

		"  UNIQUE (chat_room_id, participant_sip_address_id),"

		"  FOREIGN KEY (chat_room_id)"
		"    REFERENCES chat_room(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (participant_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_room_participant_device ("
		"  chat_room_participant_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","
		"  participant_device_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","

		"  PRIMARY KEY (chat_room_participant_id, participant_device_sip_address_id),"

		"  FOREIGN KEY (chat_room_participant_id)"
		"    REFERENCES chat_room_participant(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (participant_device_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_event ("
		"  event_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  chat_room_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES event(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (chat_room_id)"
		"    REFERENCES chat_room(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_notified_event ("
		"  event_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  notify_id INT UNSIGNED NOT NULL,"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_event(event_id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_participant_event ("
		"  event_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  participant_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_notified_event(event_id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (participant_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_participant_device_event ("
		"  event_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  device_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_participant_event(event_id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (device_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_subject_event ("
		"  event_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  subject VARCHAR(255) NOT NULL,"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_notified_event(event_id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_chat_message_event ("
		"  event_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  from_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"
		"  to_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"

		"  time" + timestampType() + " ,"

		// See: https://tools.ietf.org/html/rfc5438#section-6.3
		"  imdn_message_id VARCHAR(255) NOT NULL,"

		"  state TINYINT UNSIGNED NOT NULL,"
		"  direction TINYINT UNSIGNED NOT NULL,"
		"  is_secured BOOLEAN NOT NULL,"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_event(event_id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (from_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (to_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_message_participant ("
		"  event_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","
		"  participant_sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","
		"  state TINYINT UNSIGNED NOT NULL,"

		"  PRIMARY KEY (event_id, participant_sip_address_id),"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_chat_message_event(event_id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (participant_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_message_content ("
		"  id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  event_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"
		"  content_type_id" + primaryKeyRefStr("SMALLINT UNSIGNED") + " NOT NULL,"
		"  body TEXT NOT NULL,"

		"  UNIQUE (id, event_id),"

		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_chat_message_event(event_id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (content_type_id)"
		"    REFERENCES content_type(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_message_file_content ("
		"  chat_message_content_id" + primaryKeyStr("BIGINT UNSIGNED") + ","

		"  name VARCHAR(256) NOT NULL,"
		"  size INT UNSIGNED NOT NULL,"
		"  path VARCHAR(512) NOT NULL,"

		"  FOREIGN KEY (chat_message_content_id)"
		"    REFERENCES chat_message_content(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS chat_message_content_app_data ("
		"  chat_message_content_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","

		"  name VARCHAR(255),"
		"  data BLOB NOT NULL,"

		"  PRIMARY KEY (chat_message_content_id, name),"
		"  FOREIGN KEY (chat_message_content_id)"
		"    REFERENCES chat_message_content(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS conference_message_crypto_data ("
		"  event_id" + primaryKeyRefStr("BIGINT UNSIGNED") + ","

		"  name VARCHAR(255),"
		"  data BLOB NOT NULL,"

		"  PRIMARY KEY (event_id, name),"
		"  FOREIGN KEY (event_id)"
		"    REFERENCES conference_chat_message_event(event_id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS friends_list ("
		"  id" + primaryKeyStr("INT UNSIGNED") + ","

		"  name VARCHAR(255) UNIQUE,"
		"  rls_uri VARCHAR(2047),"
		"  sync_uri VARCHAR(2047),"
		"  revision INT UNSIGNED NOT NULL"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS friend ("
		"  id" + primaryKeyStr("INT UNSIGNED") + ","

		"  sip_address_id" + primaryKeyRefStr("BIGINT UNSIGNED") + " NOT NULL,"
		"  friends_list_id" + primaryKeyRefStr("INT UNSIGNED") + " NOT NULL,"

		"  subscribe_policy TINYINT UNSIGNED NOT NULL,"
		"  send_subscribe BOOLEAN NOT NULL,"
		"  presence_received BOOLEAN NOT NULL,"

		"  v_card MEDIUMTEXT,"
		"  v_card_etag VARCHAR(255),"
		"  v_card_sync_uri VARCHAR(2047),"

		"  FOREIGN KEY (sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (friends_list_id)"
		"    REFERENCES friends_list(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	*session <<
		"CREATE TABLE IF NOT EXISTS friend_app_data ("
		"  friend_id" + primaryKeyRefStr("INT UNSIGNED") + ","

		"  name VARCHAR(255),"
		"  data BLOB NOT NULL,"

		"  PRIMARY KEY (friend_id, name),"
		"  FOREIGN KEY (friend_id)"
		"    REFERENCES friend(id)"
		"    ON DELETE CASCADE"
		") " + charset;

	{
		string query;
		if (getBackend() == Backend::Mysql)
			query = "CREATE OR REPLACE VIEW conference_event_view AS";
		else
			query = "CREATE VIEW IF NOT EXISTS conference_event_view AS";

		*session << query +
			"  SELECT id, type, creation_time, chat_room_id, from_sip_address_id, to_sip_address_id, time, imdn_message_id, state, direction, is_secured, notify_id, device_sip_address_id, participant_sip_address_id, subject"
			"  FROM event"
			"  LEFT JOIN conference_event ON conference_event.event_id = event.id"
			"  LEFT JOIN conference_chat_message_event ON conference_chat_message_event.event_id = event.id"
			"  LEFT JOIN conference_notified_event ON conference_notified_event.event_id = event.id"
			"  LEFT JOIN conference_participant_device_event ON conference_participant_device_event.event_id = event.id"
			"  LEFT JOIN conference_participant_event ON conference_participant_event.event_id = event.id"
			"  LEFT JOIN conference_subject_event ON conference_subject_event.event_id = event.id";
	}

	*session <<
		"CREATE TABLE IF NOT EXISTS db_module_version ("
		"  name" + varcharPrimaryKeyStr(255) + ","
		"  version INT UNSIGNED NOT NULL"
		") " + charset;

	if (getBackend() == Backend::Mysql) {
		*session <<
			"DROP TRIGGER IF EXISTS chat_message_participant_deleter";
		*session <<
			"CREATE TRIGGER chat_message_participant_deleter"
			"  AFTER UPDATE ON conference_chat_message_event FOR EACH ROW"
			"  BEGIN"
			"    IF NEW.state = " + Utils::toString(int(ChatMessage::State::Displayed)) + " THEN"
			"      DELETE FROM chat_message_participant WHERE event_id = NEW.event_id;"
			"    END IF;"
			"  END ";
	} else
		*session <<
			"CREATE TRIGGER IF NOT EXISTS chat_message_participant_deleter"
			"  AFTER UPDATE OF state ON conference_chat_message_event FOR EACH ROW"
			"  WHEN NEW.state = " + Utils::toString(int(ChatMessage::State::Displayed)) +
			"  BEGIN"
			"    DELETE FROM chat_message_participant WHERE event_id = NEW.event_id;"
			"  END ";

	d->updateSchema();

	d->updateModuleVersion("events", ModuleVersionEvents);
	d->updateModuleVersion("friends", ModuleVersionFriends);
}

bool MainDb::addEvent (const shared_ptr<EventLog> &eventLog) {
	if (eventLog->getPrivate()->dbKey.isValid()) {
		lWarning() << "Unable to add an event twice!!!";
		return false;
	}

	return L_DB_TRANSACTION {
		L_D();

		long long eventId = -1;

		EventLog::Type type = eventLog->getType();
		switch (type) {
			case EventLog::Type::None:
				return false;

			case EventLog::Type::ConferenceCreated:
			case EventLog::Type::ConferenceTerminated:
				eventId = d->insertConferenceEvent(eventLog);
				break;

			case EventLog::Type::ConferenceCallStart:
			case EventLog::Type::ConferenceCallEnd:
				eventId = d->insertConferenceCallEvent(eventLog);
				break;

			case EventLog::Type::ConferenceChatMessage:
				eventId = d->insertConferenceChatMessageEvent(eventLog);
				break;

			case EventLog::Type::ConferenceParticipantAdded:
			case EventLog::Type::ConferenceParticipantRemoved:
			case EventLog::Type::ConferenceParticipantSetAdmin:
			case EventLog::Type::ConferenceParticipantUnsetAdmin:
				eventId = d->insertConferenceParticipantEvent(eventLog);
				break;

			case EventLog::Type::ConferenceParticipantDeviceAdded:
			case EventLog::Type::ConferenceParticipantDeviceRemoved:
				eventId = d->insertConferenceParticipantDeviceEvent(eventLog);
				break;

			case EventLog::Type::ConferenceSubjectChanged:
				eventId = d->insertConferenceSubjectEvent(eventLog);
				break;
		}

		if (eventId >= 0) {
			tr.commit();
			d->cache(eventLog, eventId);

			if (type == EventLog::Type::ConferenceChatMessage)
				d->cache(static_pointer_cast<ConferenceChatMessageEvent>(eventLog)->getChatMessage(), eventId);

			return true;
		}

		return false;
	};
}

bool MainDb::updateEvent (const shared_ptr<EventLog> &eventLog) {
	if (!eventLog->getPrivate()->dbKey.isValid()) {
		lWarning() << "Unable to update an event that wasn't inserted yet!!!";
		return false;
	}

	return L_DB_TRANSACTION {
		L_D();

		switch (eventLog->getType()) {
			case EventLog::Type::None:
				return false;

			case EventLog::Type::ConferenceChatMessage:
				d->updateConferenceChatMessageEvent(eventLog);
				break;

			case EventLog::Type::ConferenceCreated:
			case EventLog::Type::ConferenceTerminated:
			case EventLog::Type::ConferenceCallStart:
			case EventLog::Type::ConferenceCallEnd:
			case EventLog::Type::ConferenceParticipantAdded:
			case EventLog::Type::ConferenceParticipantRemoved:
			case EventLog::Type::ConferenceParticipantSetAdmin:
			case EventLog::Type::ConferenceParticipantUnsetAdmin:
			case EventLog::Type::ConferenceParticipantDeviceAdded:
			case EventLog::Type::ConferenceParticipantDeviceRemoved:
			case EventLog::Type::ConferenceSubjectChanged:
				return false;
		}

		tr.commit();

		return true;
	};
}

bool MainDb::deleteEvent (const shared_ptr<const EventLog> &eventLog) {
	const EventLogPrivate *dEventLog = eventLog->getPrivate();
	if (!dEventLog->dbKey.isValid()) {
		lWarning() << "Unable to delete invalid event.";
		return false;
	}

	MainDbKeyPrivate *dEventKey = static_cast<MainDbKey &>(dEventLog->dbKey).getPrivate();
	shared_ptr<Core> core = dEventKey->core.lock();
	L_ASSERT(core);

	MainDb &mainDb = *core->getPrivate()->mainDb.get();

	return L_DB_TRANSACTION_C(&mainDb) {
		soci::session *session = mainDb.getPrivate()->dbSession.getBackendSession();
		*session << "DELETE FROM event WHERE id = :id", soci::use(dEventKey->storageId);
		tr.commit();

		dEventLog->dbKey = MainDbEventKey();

		if (eventLog->getType() == EventLog::Type::ConferenceChatMessage)
			static_pointer_cast<const ConferenceChatMessageEvent>(
				eventLog
			)->getChatMessage()->getPrivate()->dbKey = MainDbChatMessageKey();

		return true;
	};
}

int MainDb::getEventCount (FilterMask mask) const {
	const string query = "SELECT COUNT(*) FROM event" +
		buildSqlEventFilter(
			{ ConferenceCallFilter, ConferenceChatMessageFilter, ConferenceInfoFilter, ConferenceInfoNoDeviceFilter },
			mask
		);

	DurationLogger durationLogger("Get event count with mask=" + Utils::toString(mask) + ".");

	return L_DB_TRANSACTION {
		L_D();

		int count;
		*d->dbSession.getBackendSession() << query, soci::into(count);
		return count;
	};
}

shared_ptr<EventLog> MainDb::getEventFromKey (const MainDbKey &dbKey) {
	if (!dbKey.isValid()) {
		lWarning() << "Unable to get event from invalid key.";
		return nullptr;
	}

	unique_ptr<MainDb> &q = dbKey.getPrivate()->core.lock()->getPrivate()->mainDb;
	MainDbPrivate *d = q->getPrivate();

	const long long &eventId = dbKey.getPrivate()->storageId;
	shared_ptr<EventLog> event = d->getEventFromCache(eventId);
	if (event)
		return event;

	return L_DB_TRANSACTION_C(q.get()) {
		// TODO: Improve. Deal with all events in the future.
		soci::row row;
		*d->dbSession.getBackendSession() << Statements::get(Statements::SelectConferenceEvent),
			soci::into(row), soci::use(eventId);

		ChatRoomId chatRoomId(IdentityAddress(row.get<string>(14)), IdentityAddress(row.get<string>(15)));
		shared_ptr<AbstractChatRoom> chatRoom = d->findChatRoom(chatRoomId);
		if (!chatRoom)
			return shared_ptr<EventLog>();

		return d->selectGenericConferenceEvent(chatRoom, row);
	};
}

list<shared_ptr<EventLog>> MainDb::getConferenceNotifiedEvents (
	const ChatRoomId &chatRoomId,
	unsigned int lastNotifyId
) const {
	// TODO: Optimize.
	const string query = Statements::get(Statements::SelectConferenceEvents) +
		string(" AND notify_id > :lastNotifyId");

	DurationLogger durationLogger(
		"Get conference notified events of: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() +
		", lastNotifyId=" + Utils::toString(lastNotifyId) + ")."
	);

	return L_DB_TRANSACTION {
		L_D();

		soci::session *session = d->dbSession.getBackendSession();

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);

		list<shared_ptr<EventLog>> events;
		soci::rowset<soci::row> rows = (session->prepare << query, soci::use(dbChatRoomId), soci::use(lastNotifyId));
		for (const auto &row : rows)
			events.push_back(d->selectGenericConferenceNotifiedEvent(chatRoomId, row));
		return events;
	};
}

int MainDb::getChatMessageCount (const ChatRoomId &chatRoomId) const {
	DurationLogger durationLogger(
		"Get chat messages count of: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() + ")."
	);

	return L_DB_TRANSACTION {
		L_D();

		int count;

		soci::session *session = d->dbSession.getBackendSession();

		string query = "SELECT COUNT(*) FROM conference_chat_message_event";
		if (!chatRoomId.isValid())
			*session << query, soci::into(count);
		else {
			query += " WHERE event_id IN ("
				"  SELECT event_id FROM conference_event WHERE chat_room_id = :chatRoomId"
				")";

			const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);
			*session << query, soci::use(dbChatRoomId), soci::into(count);
		}

		return count;
	};
}

int MainDb::getUnreadChatMessageCount (const ChatRoomId &chatRoomId) const {
	string query = "SELECT COUNT(*) FROM conference_chat_message_event WHERE";
	if (chatRoomId.isValid())
		query += " event_id IN ("
			"  SELECT event_id FROM conference_event WHERE chat_room_id = :chatRoomId"
			") AND";

	query += " direction = " + Utils::toString(int(ChatMessage::Direction::Incoming)) +
		+ " AND state <> " + Utils::toString(int(ChatMessage::State::Displayed));

	DurationLogger durationLogger(
		"Get unread chat messages count of: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() + ")."
	);

	return L_DB_TRANSACTION {
		L_D();

		int count = 0;

		soci::session *session = d->dbSession.getBackendSession();

		if (!chatRoomId.isValid())
			*session << query, soci::into(count);
		else {
			const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);
			*session << query, soci::use(dbChatRoomId), soci::into(count);
		}

		return count;
	};
}

void MainDb::markChatMessagesAsRead (const ChatRoomId &chatRoomId) const {
	if (getUnreadChatMessageCount(chatRoomId) == 0)
		return;

	static const string query = "UPDATE conference_chat_message_event"
		"  SET state = " + Utils::toString(int(ChatMessage::State::Displayed)) +
		"  WHERE event_id IN ("
		"    SELECT event_id FROM conference_event WHERE chat_room_id = :chatRoomId"
		") AND direction = " + Utils::toString(int(ChatMessage::Direction::Incoming));

	DurationLogger durationLogger(
		"Mark chat messages as read of: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() + ")."
	);

	L_DB_TRANSACTION {
		L_D();

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);
		*d->dbSession.getBackendSession() << query, soci::use(dbChatRoomId);

		tr.commit();
	};
}

list<shared_ptr<ChatMessage>> MainDb::getUnreadChatMessages (const ChatRoomId &chatRoomId) const {
	// TODO: Optimize.
	static const string query = Statements::get(Statements::SelectConferenceEvents) +
		string(" AND direction = ") + Utils::toString(int(ChatMessage::Direction::Incoming)) +
		" AND state <> " + Utils::toString(int(ChatMessage::State::Displayed));

	DurationLogger durationLogger(
		"Get unread chat messages: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() + ")."
	);

	return L_DB_TRANSACTION {
		L_D();

		soci::session *session = d->dbSession.getBackendSession();

		long long dbChatRoomId = d->selectChatRoomId(chatRoomId);
		shared_ptr<AbstractChatRoom> chatRoom = d->findChatRoom(chatRoomId);
		list<shared_ptr<ChatMessage>> chatMessages;
		if (!chatRoom)
			return chatMessages;

		soci::rowset<soci::row> rows = (session->prepare << query, soci::use(dbChatRoomId));
		for (const auto &row : rows) {
			shared_ptr<EventLog> event = d->selectGenericConferenceEvent(
				chatRoom,
				row
			);

			if (event)
				chatMessages.push_back(static_pointer_cast<ConferenceChatMessageEvent>(event)->getChatMessage());
		}

		return chatMessages;
	};
}

list<ChatMessage::State> MainDb::getChatMessageParticipantStates (const shared_ptr<EventLog> &eventLog) const {
	return L_DB_TRANSACTION {
		L_D();

		const EventLogPrivate *dEventLog = eventLog->getPrivate();
		MainDbKeyPrivate *dEventKey = static_cast<MainDbKey &>(dEventLog->dbKey).getPrivate();
		const long long &eventId = dEventKey->storageId;

		unsigned int state;
		soci::statement statement = (
			d->dbSession.getBackendSession()->prepare << "SELECT state FROM chat_message_participant WHERE event_id = :eventId",
				soci::into(state), soci::use(eventId)
		);
		statement.execute();

		list<ChatMessage::State> states;
		while (statement.fetch())
			states.push_back(ChatMessage::State(state));

		return states;
	};
}

ChatMessage::State MainDb::getChatMessageParticipantState (
	const shared_ptr<EventLog> &eventLog,
	const IdentityAddress &participantAddress
) const {
	return L_DB_TRANSACTION {
		L_D();

		const EventLogPrivate *dEventLog = eventLog->getPrivate();
		MainDbKeyPrivate *dEventKey = static_cast<MainDbKey &>(dEventLog->dbKey).getPrivate();
		const long long &eventId = dEventKey->storageId;
		const long long &participantSipAddressId = d->selectSipAddressId(participantAddress.asString());

		unsigned int state;
		*d->dbSession.getBackendSession() << "SELECT state FROM chat_message_participant"
			" WHERE event_id = :eventId AND participant_sip_address_id = :participantSipAddressId",
			soci::into(state), soci::use(eventId), soci::use(participantSipAddressId);

		return ChatMessage::State(state);
	};
}

void MainDb::setChatMessageParticipantState (
	const shared_ptr<EventLog> &eventLog,
	const IdentityAddress &participantAddress,
	ChatMessage::State state
) {
	L_DB_TRANSACTION {
		L_D();

		const EventLogPrivate *dEventLog = eventLog->getPrivate();
		MainDbKeyPrivate *dEventKey = static_cast<MainDbKey &>(dEventLog->dbKey).getPrivate();
		const long long &eventId = dEventKey->storageId;
		const long long &participantSipAddressId = d->selectSipAddressId(participantAddress.asString());
		int stateInt = static_cast<int>(state);

		*d->dbSession.getBackendSession() << "UPDATE chat_message_participant SET state = :state"
			" WHERE event_id = :eventId AND participant_sip_address_id = :participantSipAddressId",
			soci::use(stateInt), soci::use(eventId), soci::use(participantSipAddressId);

		tr.commit();
	};
}

shared_ptr<ChatMessage> MainDb::getLastChatMessage (const ChatRoomId &chatRoomId) const {
	list<shared_ptr<EventLog>> chatList = getHistory(chatRoomId, 1, Filter::ConferenceChatMessageFilter);
	if (chatList.empty())
		return nullptr;

	return static_pointer_cast<ConferenceChatMessageEvent>(chatList.front())->getChatMessage();
}

list<shared_ptr<ChatMessage>> MainDb::findChatMessages (
	const ChatRoomId &chatRoomId,
	const string &imdnMessageId
) const {
	// TODO: Optimize.
	static const string query = Statements::get(Statements::SelectConferenceEvents) +
		string(" AND imdn_message_id = :imdnMessageId");

	DurationLogger durationLogger(
		"Find chat messages: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() + ")."
	);

	return L_DB_TRANSACTION {
		L_D();

		shared_ptr<AbstractChatRoom> chatRoom = d->findChatRoom(chatRoomId);
		list<shared_ptr<ChatMessage>> chatMessages;
		if (!chatRoom)
			return chatMessages;

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);
		soci::rowset<soci::row> rows = (
			d->dbSession.getBackendSession()->prepare << query, soci::use(dbChatRoomId), soci::use(imdnMessageId)
		);
		for (const auto &row : rows) {
			shared_ptr<EventLog> event = d->selectGenericConferenceEvent(chatRoom, row);
			if (event) {
				L_ASSERT(event->getType() == EventLog::Type::ConferenceChatMessage);
				chatMessages.push_back(static_pointer_cast<ConferenceChatMessageEvent>(event)->getChatMessage());
			}
		}

		return chatMessages;
	};
}

list<shared_ptr<EventLog>> MainDb::getHistory (const ChatRoomId &chatRoomId, int nLast, FilterMask mask) const {
	return getHistoryRange(chatRoomId, 0, nLast, mask);
}

list<shared_ptr<EventLog>> MainDb::getHistoryRange (
	const ChatRoomId &chatRoomId,
	int begin,
	int end,
	FilterMask mask
) const {
	L_D();

	if (begin < 0)
		begin = 0;

	list<shared_ptr<EventLog>> events;
	if (end > 0 && begin > end) {
		lWarning() << "Unable to get history. Invalid range.";
		return events;
	}

	string query = Statements::get(Statements::SelectConferenceEvents) + buildSqlEventFilter({
		ConferenceCallFilter, ConferenceChatMessageFilter, ConferenceInfoFilter, ConferenceInfoNoDeviceFilter
	}, mask, "AND");
	query += " ORDER BY event_id DESC";

	if (end > 0)
		query += " LIMIT " + Utils::toString(end - begin);
	else
		query += " LIMIT " + d->dbSession.noLimitValue();

	if (begin > 0)
		query += " OFFSET " + Utils::toString(begin);

	DurationLogger durationLogger(
		"Get history range of: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() +
		", begin=" + Utils::toString(begin) + ", end=" + Utils::toString(end) + ")."
	);

	return L_DB_TRANSACTION {
		L_D();

		shared_ptr<AbstractChatRoom> chatRoom = d->findChatRoom(chatRoomId);
		if (!chatRoom)
			return events;

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);
		soci::rowset<soci::row> rows = (d->dbSession.getBackendSession()->prepare << query, soci::use(dbChatRoomId));
		for (const auto &row : rows) {
			shared_ptr<EventLog> event = d->selectGenericConferenceEvent(chatRoom, row);
			if (event)
				events.push_front(event);
		}

		return events;
	};
}

int MainDb::getHistorySize (const ChatRoomId &chatRoomId, FilterMask mask) const {
	const string query = "SELECT COUNT(*) FROM event, conference_event"
		"  WHERE chat_room_id = :chatRoomId"
		"  AND event_id = event.id" + buildSqlEventFilter({
			ConferenceCallFilter, ConferenceChatMessageFilter, ConferenceInfoFilter, ConferenceInfoNoDeviceFilter
		}, mask, "AND");

	return L_DB_TRANSACTION {
		L_D();

		int count;
		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);
		*d->dbSession.getBackendSession() << query, soci::into(count), soci::use(dbChatRoomId);

		return count;
	};
}

template<typename T>
static void fetchContentAppData (soci::session *session, Content &content, long long contentId, T &data) {
	static const string query = "SELECT name, data FROM chat_message_content_app_data"
		" WHERE chat_message_content_id = :contentId";

	string name;
	soci::statement statement = (session->prepare << query, soci::use(contentId), soci::into(name), soci::into(data));
	statement.execute();
	while (statement.fetch())
		content.setAppData(name, blobToString(data));
}

void MainDb::loadChatMessageContents (const shared_ptr<ChatMessage> &chatMessage) {
	L_DB_TRANSACTION {
		L_D();

		soci::session *session = d->dbSession.getBackendSession();

		bool hasFileTransferContent = false;

		ChatMessagePrivate *dChatMessage = chatMessage->getPrivate();
		MainDbKeyPrivate *dEventKey = static_cast<MainDbKey &>(dChatMessage->dbKey).getPrivate();
		const long long &eventId = dEventKey->storageId;

		static const string query = "SELECT chat_message_content.id, content_type.id, content_type.value, body"
			" FROM chat_message_content, content_type"
			" WHERE event_id = :eventId AND content_type_id = content_type.id";
		soci::rowset<soci::row> rows = (session->prepare << query, soci::use(eventId));
		for (const auto &row : rows) {
			ContentType contentType(row.get<string>(2));
			const long long &contentId = d->dbSession.resolveId(row, 0);
			Content *content;

			if (contentType == ContentType::FileTransfer) {
				hasFileTransferContent = true;
				content = new FileTransferContent();
			} else if (contentType.isFile()) {
				// 1.1 - Fetch contents' file informations.
				string name;
				int size;
				string path;

				*session << "SELECT name, size, path FROM chat_message_file_content"
					" WHERE chat_message_content_id = :contentId",
					soci::into(name), soci::into(size), soci::into(path), soci::use(contentId);

				FileContent *fileContent = new FileContent();
				fileContent->setFileName(name);
				fileContent->setFileSize(size_t(size));
				fileContent->setFilePath(path);

				content = fileContent;
			} else
				content = new Content();

			content->setContentType(contentType);
			content->setBody(row.get<string>(3));

			// 1.2 - Fetch contents' app data.
			// TODO: Do not test backend, encapsulate!!!
			if (getBackend() == MainDb::Backend::Sqlite3) {
				soci::blob data(*session);
				fetchContentAppData(session, *content, contentId, data);
			} else {
				string data;
				fetchContentAppData(session, *content, contentId, data);
			}
			chatMessage->addContent(*content);
		}

		// 2 - Load external body url from body into FileTransferContent if needed.
		if (hasFileTransferContent)
			dChatMessage->loadFileTransferUrlFromBodyToContent();
	};
}

void MainDb::cleanHistory (const ChatRoomId &chatRoomId, FilterMask mask) {
	const string query = "SELECT event_id FROM conference_event WHERE chat_room_id = :chatRoomId" +
		buildSqlEventFilter({
			ConferenceCallFilter, ConferenceChatMessageFilter, ConferenceInfoFilter, ConferenceInfoNoDeviceFilter
		}, mask);

	DurationLogger durationLogger(
		"Clean history of: (peer=" + chatRoomId.getPeerAddress().asString() +
		", local=" + chatRoomId.getLocalAddress().asString() +
		", mask=" + Utils::toString(mask) + ")."
	);

	L_DB_TRANSACTION {
		L_D();

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);

		d->invalidConferenceEventsFromQuery(query, dbChatRoomId);
		*d->dbSession.getBackendSession() << "DELETE FROM event WHERE id IN (" + query + ")", soci::use(dbChatRoomId);

		tr.commit();
	};
}

// -----------------------------------------------------------------------------

list<shared_ptr<AbstractChatRoom>> MainDb::getChatRooms () const {
	static const string query = "SELECT chat_room.id, peer_sip_address.value, local_sip_address.value,"
		" creation_time, last_update_time, capabilities, subject, last_notify_id, flags"
		" FROM chat_room, sip_address AS peer_sip_address, sip_address AS local_sip_address"
		" WHERE chat_room.peer_sip_address_id = peer_sip_address.id AND chat_room.local_sip_address_id = local_sip_address.id"
		" ORDER BY last_update_time DESC";

	DurationLogger durationLogger("Get chat rooms.");

	return L_DB_TRANSACTION {
		L_D();

		list<shared_ptr<AbstractChatRoom>> chatRooms;
		shared_ptr<Core> core = getCore();

		soci::session *session = d->dbSession.getBackendSession();

		soci::rowset<soci::row> rows = (session->prepare << query);
		for (const auto &row : rows) {
			ChatRoomId chatRoomId = ChatRoomId(
				IdentityAddress(row.get<string>(1)),
				IdentityAddress(row.get<string>(2))
			);
			shared_ptr<AbstractChatRoom> chatRoom = core->findChatRoom(chatRoomId);
			if (chatRoom) {
				chatRooms.push_back(chatRoom);
				continue;
			}

			tm creationTime = row.get<tm>(3);
			tm lastUpdateTime = row.get<tm>(4);
			int capabilities = row.get<int>(5);
			string subject = row.get<string>(6, "");
			unsigned int lastNotifyId = getBackend() == Backend::Mysql
				? row.get<unsigned int>(7, 0)
				: static_cast<unsigned int>(row.get<int>(7, 0));

			if (capabilities & ChatRoom::CapabilitiesMask(ChatRoom::Capabilities::Basic)) {
				chatRoom = core->getPrivate()->createBasicChatRoom(chatRoomId, capabilities);
				chatRoom->setSubject(subject);
			} else if (capabilities & ChatRoom::CapabilitiesMask(ChatRoom::Capabilities::Conference)) {
				list<shared_ptr<Participant>> participants;

				const long long &dbChatRoomId = d->dbSession.resolveId(row, 0);
				static const string query = "SELECT chat_room_participant.id, sip_address.value, is_admin"
					" FROM sip_address, chat_room, chat_room_participant"
					" WHERE chat_room.id = :chatRoomId"
					" AND sip_address.id = chat_room_participant.participant_sip_address_id"
					" AND chat_room_participant.chat_room_id = chat_room.id";

				// Fetch participants.
				soci::rowset<soci::row> rows = (session->prepare << query, soci::use(dbChatRoomId));
				shared_ptr<Participant> me;
				for (const auto &row : rows) {
					shared_ptr<Participant> participant = make_shared<Participant>(IdentityAddress(row.get<string>(1)));
					ParticipantPrivate *dParticipant = participant->getPrivate();
					dParticipant->setAdmin(!!row.get<int>(2));

					// Fetch devices.
					{
						const long long &participantId = d->dbSession.resolveId(row, 0);
						static const string query = "SELECT sip_address.value, state FROM chat_room_participant_device, sip_address"
							" WHERE chat_room_participant_id = :participantId"
							" AND participant_device_sip_address_id = sip_address.id";

						soci::rowset<soci::row> rows = (session->prepare << query, soci::use(participantId));
						for (const auto &row : rows) {
							shared_ptr<ParticipantDevice> device = dParticipant->addDevice(IdentityAddress(row.get<string>(0)));
							device->setState(ParticipantDevice::State(static_cast<unsigned int>(row.get<int>(1, 0))));
						}
					}

					if (participant->getAddress() == chatRoomId.getLocalAddress().getAddressWithoutGruu())
						me = participant;
					else
						participants.push_back(participant);
				}

				if (!linphone_core_conference_server_enabled(core->getCCore())) {
					bool hasBeenLeft = !!row.get<int>(8, 0);
					if (!me) {
						lError() << "Unable to find me in: (peer=" + chatRoomId.getPeerAddress().asString() +
							", local=" + chatRoomId.getLocalAddress().asString() + ").";
						continue;
					}
					chatRoom = make_shared<ClientGroupChatRoom>(
						core,
						chatRoomId,
						me,
						capabilities,
						subject,
						move(participants),
						lastNotifyId
					);
					AbstractChatRoomPrivate *dChatRoom = chatRoom->getPrivate();
					dChatRoom->setState(ChatRoom::State::Instantiated);
					dChatRoom->setState(hasBeenLeft
						? ChatRoom::State::Terminated
						: ChatRoom::State::Created
					);
				} else {
					chatRoom = make_shared<ServerGroupChatRoom>(
						core,
						chatRoomId.getPeerAddress(),
						capabilities,
						subject,
						move(participants),
						lastNotifyId
					);
					AbstractChatRoomPrivate *dChatRoom = chatRoom->getPrivate();
					dChatRoom->setState(ChatRoom::State::Instantiated);
					dChatRoom->setState(ChatRoom::State::Created);
				}
			}

			if (!chatRoom)
				continue; // Not fetched.

			AbstractChatRoomPrivate *dChatRoom = chatRoom->getPrivate();
			dChatRoom->setCreationTime(Utils::getTmAsTimeT(creationTime));
			dChatRoom->setLastUpdateTime(Utils::getTmAsTimeT(lastUpdateTime));

			lInfo() << "Found chat room in DB: (peer=" <<
				chatRoomId.getPeerAddress().asString() << ", local=" << chatRoomId.getLocalAddress().asString() << ").";

			chatRooms.push_back(chatRoom);
		}

		tr.commit();

		return chatRooms;
	};
}

void MainDb::insertChatRoom (const shared_ptr<AbstractChatRoom> &chatRoom) {
	L_DB_TRANSACTION {
		L_D();

		d->insertChatRoom(chatRoom);
		tr.commit();
	};
}

void MainDb::deleteChatRoom (const ChatRoomId &chatRoomId) {
	L_DB_TRANSACTION {
		L_D();

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);

		d->invalidConferenceEventsFromQuery(
			"SELECT event_id FROM conference_event WHERE chat_room_id = :chatRoomId",
			dbChatRoomId
		);

		*d->dbSession.getBackendSession() << "DELETE FROM chat_room WHERE id = :chatRoomId", soci::use(dbChatRoomId);

		tr.commit();
	};
}

void MainDb::migrateBasicToClientGroupChatRoom (
	const shared_ptr<AbstractChatRoom> &basicChatRoom,
	const shared_ptr<AbstractChatRoom> &clientGroupChatRoom
) {
	L_ASSERT(basicChatRoom->getCapabilities().isSet(ChatRoom::Capabilities::Basic));
	L_ASSERT(clientGroupChatRoom->getCapabilities().isSet(ChatRoom::Capabilities::Conference));

	L_DB_TRANSACTION {
		L_D();

		// TODO: Update events and chat messages. (Or wait signals.)
		const long long &dbChatRoomId = d->selectChatRoomId(basicChatRoom->getChatRoomId());

		const ChatRoomId &newChatRoomId = clientGroupChatRoom->getChatRoomId();
		const long long &peerSipAddressId = d->insertSipAddress(newChatRoomId.getPeerAddress().asString());
		const long long &localSipAddressId = d->insertSipAddress(newChatRoomId.getLocalAddress().asString());
		const int &capabilities = clientGroupChatRoom->getCapabilities();

		*d->dbSession.getBackendSession() << "UPDATE chat_room"
			" SET capabilities = :capabilities,"
			" peer_sip_address_id = :peerSipAddressId,"
			" local_sip_address_id = :localSipAddressId"
			" WHERE id = :chatRoomId", soci::use(capabilities), soci::use(peerSipAddressId),
			soci::use(localSipAddressId), soci::use(dbChatRoomId);

		shared_ptr<Participant> me = clientGroupChatRoom->getMe();
		long long meId = d->insertChatRoomParticipant(
			dbChatRoomId,
			d->insertSipAddress(me->getAddress().asString()),
			true
		);
		for (const auto &device : me->getPrivate()->getDevices())
			d->insertChatRoomParticipantDevice(meId, d->insertSipAddress(device->getAddress().asString()));

		for (const auto &participant : clientGroupChatRoom->getParticipants()) {
			long long participantId = d->insertChatRoomParticipant(
				dbChatRoomId,
				d->insertSipAddress(participant->getAddress().asString()),
				true
			);
			for (const auto &device : participant->getPrivate()->getDevices())
				d->insertChatRoomParticipantDevice(participantId, d->insertSipAddress(device->getAddress().asString()));
		}

		tr.commit();
	};
}

IdentityAddress MainDb::findMissingOneToOneConferenceChatRoomParticipantAddress (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const IdentityAddress &presentParticipantAddr
) {
	L_ASSERT(linphone_core_conference_server_enabled(chatRoom->getCore()->getCCore()));
	L_ASSERT(chatRoom->getCapabilities() & ChatRoom::Capabilities::OneToOne);
	L_ASSERT(chatRoom->getParticipantCount() == 1);

	return L_DB_TRANSACTION {
		L_D();

		string missingParticipantAddress;
		string participantASipAddress;
		string participantBSipAddress;

		const long long &chatRoomId = d->selectChatRoomId(chatRoom->getChatRoomId());
		L_ASSERT(chatRoomId != -1);

		*d->dbSession.getBackendSession() << "SELECT participant_a_sip_address.value, participant_b_sip_address.value"
			" FROM one_to_one_chat_room, sip_address AS participant_a_sip_address, sip_address AS participant_b_sip_address"
			" WHERE chat_room_id = :chatRoomId"
			" AND participant_a_sip_address_id = participant_a_sip_address.id"
			" AND participant_b_sip_address_id = participant_b_sip_address.id",
			soci::into(participantASipAddress), soci::into(participantBSipAddress), soci::use(chatRoomId);

		string presentParticipantAddress(presentParticipantAddr.asString());
		if (presentParticipantAddress == participantASipAddress)
			missingParticipantAddress = participantBSipAddress;
		else if (presentParticipantAddress == participantBSipAddress)
			missingParticipantAddress = participantASipAddress;

		return IdentityAddress(missingParticipantAddress);
	};
}

IdentityAddress MainDb::findOneToOneConferenceChatRoomAddress (
	const IdentityAddress &participantA,
	const IdentityAddress &participantB
) const {
	return L_DB_TRANSACTION {
		L_D();

		const long long &participantASipAddressId = d->selectSipAddressId(participantA.asString());
		const long long &participantBSipAddressId = d->selectSipAddressId(participantB.asString());
		if (participantASipAddressId == -1 || participantBSipAddressId == -1)
			return IdentityAddress();

		const long long &chatRoomId = d->selectOneToOneChatRoomId(participantASipAddressId, participantBSipAddressId);

		string chatRoomAddress;
		*d->dbSession.getBackendSession() << "SELECT sip_address.value"
			" FROM chat_room, sip_address"
			" WHERE chat_room.id = :chatRoomId AND peer_sip_address_id = sip_address.id",
			soci::use(chatRoomId), soci::into(chatRoomAddress);

		return IdentityAddress(chatRoomAddress);
	};
}

void MainDb::insertOneToOneConferenceChatRoom (const shared_ptr<AbstractChatRoom> &chatRoom) {
	L_ASSERT(linphone_core_conference_server_enabled(chatRoom->getCore()->getCCore()));
	L_ASSERT(chatRoom->getCapabilities() & ChatRoom::Capabilities::OneToOne);

	L_DB_TRANSACTION {
		L_D();

		const list<shared_ptr<Participant>> &participants = chatRoom->getParticipants();
		const long long &participantASipAddressId = d->selectSipAddressId(participants.front()->getAddress().asString());
		const long long &participantBSipAddressId = d->selectSipAddressId(participants.back()->getAddress().asString());
		L_ASSERT(participantASipAddressId != -1);
		L_ASSERT(participantBSipAddressId != -1);

		long long chatRoomId = d->selectOneToOneChatRoomId(participantASipAddressId, participantBSipAddressId);
		if (chatRoomId == -1) {
			chatRoomId = d->selectChatRoomId(chatRoom->getChatRoomId());
			*d->dbSession.getBackendSession() << Statements::get(Statements::InsertOneToOneChatRoom, getBackend()),
				soci::use(chatRoomId), soci::use(participantASipAddressId), soci::use(participantBSipAddressId);
		}

		tr.commit();
	};
}

void MainDb::enableChatRoomMigration (const ChatRoomId &chatRoomId, bool enable) {
	L_DB_TRANSACTION {
		L_D();

		soci::session *session = d->dbSession.getBackendSession();

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoomId);

		int capabilities = 0;
		*session << "SELECT capabilities FROM chat_room WHERE id = :chatRoomId",
			soci::use(dbChatRoomId), soci::into(capabilities);
		if (enable)
			capabilities |= int(ChatRoom::Capabilities::Migratable);
		else
			capabilities &= ~int(ChatRoom::Capabilities::Migratable);
		*session << "UPDATE chat_room SET capabilities = :capabilities WHERE id = :chatRoomId",
			soci::use(capabilities), soci::use(dbChatRoomId);

		tr.commit();
	};
}

void MainDb::updateChatRoomParticipantDevice (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const shared_ptr<ParticipantDevice> &device
) {
	L_DB_TRANSACTION {
		L_D();

		const long long &dbChatRoomId = d->selectChatRoomId(chatRoom->getChatRoomId());
		const long long &participantSipAddressId = d->selectSipAddressId(device->getParticipant()->getAddress().asString());
		const long long &participantId = d->selectChatRoomParticipantId(dbChatRoomId, participantSipAddressId);
		const long long &participantSipDeviceAddressId = d->selectSipAddressId(device->getAddress().asString());
		unsigned int state = static_cast<unsigned int>(device->getState());
		*d->dbSession.getBackendSession() << "UPDATE chat_room_participant_device SET state = :state"
			" WHERE chat_room_participant_id = :participantId AND participant_device_sip_address_id = :participantSipDeviceAddressId",
			soci::use(state), soci::use(participantId), soci::use(participantSipDeviceAddressId);

		tr.commit();
	};
}

// -----------------------------------------------------------------------------

bool MainDb::import (Backend, const string &parameters) {
	L_D();

	// Backend is useless, it's sqlite3. (Only available legacy backend.)
	const string uri = "sqlite3://" + parameters;
	DbSession inDbSession(uri);

	if (!inDbSession) {
		lWarning() << "Unable to connect to: `" << uri << "`.";
		return false;
	}

	// TODO: Remove condition after cpp migration in friends/friends list.
	if (false)
		d->importLegacyFriends(inDbSession);

	d->importLegacyHistory(inDbSession);

	return true;
}

LINPHONE_END_NAMESPACE
