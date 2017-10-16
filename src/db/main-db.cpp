/*
 * main-db.cpp
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

#include <algorithm>
#include <ctime>

#ifdef SOCI_ENABLED
	#include <soci/soci.h>
#endif // ifdef SOCI_ENABLED

#include "linphone/utils/utils.h"

#include "chat/chat-room/chat-room.h"
#include "conference/participant.h"
#include "content/content-type.h"
#include "content/content.h"
#include "db/session/db-session-provider.h"
#include "event-log/call-event.h"
#include "event-log/chat-message-event.h"
#include "event-log/event-log-p.h"
#include "logger/logger.h"
#include "main-db-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

MainDb::MainDb () : AbstractDb(*new MainDbPrivate) {}

#ifdef SOCI_ENABLED

// -----------------------------------------------------------------------------
// Soci backend.
// -----------------------------------------------------------------------------

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

	static constexpr EnumToSql<MainDb::Filter> eventFilterToSql[] = {
		{ MainDb::MessageFilter, "1" },
		{ MainDb::CallFilter, "2" },
		{ MainDb::ConferenceFilter, "3" }
	};

	static constexpr const char *mapEventFilterToSql (MainDb::Filter filter) {
		return mapEnumToSql(
			eventFilterToSql, sizeof eventFilterToSql / sizeof eventFilterToSql[0], filter
		);
	}

// -----------------------------------------------------------------------------

	static string buildSqlEventFilter (const list<MainDb::Filter> &filters, MainDb::FilterMask mask) {
		L_ASSERT(
			find_if(filters.cbegin(), filters.cend(), [](const MainDb::Filter &filter) {
					return filter == MainDb::NoFilter;
				}) == filters.cend()
		);

		if (mask == MainDb::NoFilter)
			return "";

		bool isStart = true;
		string sql;
		for (const auto &filter : filters) {
			if (!(mask & filter))
				continue;

			if (isStart) {
				isStart = false;
				sql += " WHERE ";
			} else
				sql += " OR ";
			sql += " type = ";
			sql += mapEventFilterToSql(filter);
		}

		return sql;
	}

// -----------------------------------------------------------------------------

	long MainDbPrivate::insertSipAddress (const string &sipAddress) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		long id;
		*session << "SELECT id FROM sip_address WHERE value = :sipAddress", soci::use(sipAddress), soci::into(id);
		if (session->got_data())
			return id;

		*session << "INSERT INTO sip_address (value) VALUES (:sipAddress)", soci::use(sipAddress);
		return q->getLastInsertId();
	}

	void MainDbPrivate::insertContent (long messageEventId, const Content &content) {
		L_Q();

		soci::session *session = dbSession.getBackendSession<soci::session>();

		long contentTypeId = insertContentType(content.getContentType().asString());
		*session << "INSERT INTO message_content (message_event_id, content_type_id, body) VALUES"
			"  (:messageEventId, :contentTypeId, :body)", soci::use(messageEventId), soci::use(contentTypeId),
			soci::use(content.getBodyAsString());

		long messageContentId = q->getLastInsertId();
		for (const auto &appData : content.getAppDataMap())
			*session << "INSERT INTO message_content_app_data (message_content_id, key, data) VALUES"
				"  (:messageContentId, :key, :data)",
				soci::use(messageContentId), soci::use(appData.first), soci::use(appData.second);
	}

	long MainDbPrivate::insertContentType (const string &contentType) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		long id;
		*session << "SELECT id FROM content_type WHERE value = :contentType", soci::use(contentType), soci::into(id);
		if (session->got_data())
			return id;

		*session << "INSERT INTO content_type (value) VALUES (:contentType)", soci::use(contentType);
		return q->getLastInsertId();
	}

	long MainDbPrivate::insertEvent (EventLog::Type type, const tm &date) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		*session << "INSERT INTO event (type, date) VALUES (:type, :date)",
			soci::use(static_cast<int>(type)), soci::use(date);
		return q->getLastInsertId();
	}

	long MainDbPrivate::insertChatRoom (long sipAddressId, int capabilities, const tm &date) {
		soci::session *session = dbSession.getBackendSession<soci::session>();

		long id;
		*session << "SELECT peer_sip_address_id FROM chat_room WHERE peer_sip_address_id = :sipAddressId",
			soci::use(sipAddressId), soci::into(id);
		if (!session->got_data())
			*session << "INSERT INTO chat_room (peer_sip_address_id, creation_date, last_update_date, capabilities, subject) VALUES"
				"  (:sipAddressId, :creationDate, :lastUpdateDate, :capabilities, '')",
				soci::use(sipAddressId), soci::use(date), soci::use(date), soci::use(capabilities);
		else
			*session << "UPDATE chat_room SET last_update_date = :lastUpdateDate WHERE peer_sip_address_id = :sipAddressId",
				soci::use(date), soci::use(sipAddressId);

		return sipAddressId;
	}

	void MainDbPrivate::insertChatRoomParticipant (long chatRoomId, long sipAddressId, bool isAdmin) {
		soci::session *session = dbSession.getBackendSession<soci::session>();
		soci::statement statement = (
			session->prepare << "UPDATE chat_room_participant SET is_admin = :isAdmin"
				"  WHERE chat_room_id = :chatRoomId AND sip_address_id = :sipAddressId",
				soci::use(static_cast<int>(isAdmin)), soci::use(chatRoomId), soci::use(sipAddressId)
		);
		statement.execute(true);
		if (statement.get_affected_rows() == 0)
			*session << "INSERT INTO chat_room_participant (chat_room_id, sip_address_id, is_admin)"
				"  VALUES (:chatRoomId, :sipAddressId, :isAdmin)",
				soci::use(chatRoomId), soci::use(sipAddressId), soci::use(static_cast<int>(isAdmin));
	}

	long MainDbPrivate::insertMessageEvent (
		const MessageEventReferences &references,
		ChatMessage::State state,
		ChatMessage::Direction direction,
		const string &imdnMessageId,
		bool isSecured,
		const list<Content> &contents
	) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		*session << "INSERT INTO message_event ("
			"  event_id, chat_room_id, local_sip_address_id, remote_sip_address_id,"
			"  state, direction, imdn_message_id, is_secured"
			") VALUES ("
			"  :eventId, :chatRoomId, :localSipaddressId, :remoteSipaddressId,"
			"  :state, :direction, :imdnMessageId, :isSecured"
			")", soci::use(references.eventId), soci::use(references.chatRoomId), soci::use(references.localSipAddressId),
			soci::use(references.remoteSipAddressId), soci::use(static_cast<int>(state)),
			soci::use(static_cast<int>(direction)), soci::use(imdnMessageId), soci::use(isSecured ? 1 : 0);

		long messageEventId = q->getLastInsertId();

		for (const auto &content : contents)
			insertContent(messageEventId, content);

		return messageEventId;
	}

	void MainDbPrivate::insertMessageParticipant (long messageEventId, long sipAddressId, ChatMessage::State state) {
		soci::session *session = dbSession.getBackendSession<soci::session>();
		soci::statement statement = (
			session->prepare << "UPDATE message_participant SET state = :state"
				"  WHERE message_event_id = :messageEventId AND sip_address_id = :sipAddressId",
				soci::use(static_cast<int>(state)), soci::use(messageEventId), soci::use(sipAddressId)
		);
		statement.execute(true);
		if (statement.get_affected_rows() == 0 && state != ChatMessage::State::Displayed)
			*session << "INSERT INTO message_participant (message_event_id, sip_address_id, state)"
				"  VALUES (:messageEventId, :sipAddressId, :state)",
				soci::use(messageEventId), soci::use(sipAddressId), soci::use(static_cast<int>(state));
	}

// -----------------------------------------------------------------------------

	#define LEGACY_MESSAGE_COL_LOCAL_ADDRESS 1
	#define LEGACY_MESSAGE_COL_REMOTE_ADDRESS 2
	#define LEGACY_MESSAGE_COL_DIRECTION 3
	#define LEGACY_MESSAGE_COL_TEXT 4
	#define LEGACY_MESSAGE_COL_STATE 7
	#define LEGACY_MESSAGE_COL_URL 8
	#define LEGACY_MESSAGE_COL_DATE 9
	#define LEGACY_MESSAGE_COL_APP_DATA 10
	#define LEGACY_MESSAGE_COL_CONTENT_ID 11
	#define LEGACY_MESSAGE_COL_IMDN_MESSAGE_ID 12
	#define LEGACY_MESSAGE_COL_CONTENT_TYPE 13
	#define LEGACY_MESSAGE_COL_IS_SECURED 14

	template<typename T>
	static T getValueFromLegacyMessage (const soci::row &message, int index, bool &isNull) {
		isNull = false;

		try {
			return message.get<T>(static_cast<size_t>(index));
		} catch (const exception &) {
			isNull = true;
		}

		return T();
	}

// -----------------------------------------------------------------------------

	void MainDb::init () {
		L_D();
		soci::session *session = d->dbSession.getBackendSession<soci::session>();

		*session <<
			"CREATE TABLE IF NOT EXISTS sip_address ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  value VARCHAR(255) UNIQUE NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS content_type ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  value VARCHAR(255) UNIQUE NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS event ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  type TINYINT UNSIGNED NOT NULL,"
			"  date DATE NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS chat_room ("
			// Server (for conference) or user sip address.
			"  peer_sip_address_id INT UNSIGNED PRIMARY KEY,"

			// Dialog creation date.
			"  creation_date DATE NOT NULL,"

			// Last event date (call, message...).
			"  last_update_date DATE NOT NULL,"

			// ConferenceChatRoom, BasicChatRoom, RTT...
			"capabilities TINYINT UNSIGNED,"

			// Chatroom subject.
			"  subject VARCHAR(255),"

			"  last_notify INT UNSIGNED,"

			"  FOREIGN KEY (peer_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS chat_room_participant ("
			"  chat_room_id INT UNSIGNED NOT NULL,"
			"  sip_address_id INT UNSIGNED NOT NULL,"
			"  is_admin BOOLEAN NOT NULL,"

			"  PRIMARY KEY (chat_room_id, sip_address_id),"
			"  FOREIGN KEY (chat_room_id)"
			"    REFERENCES chat_room(peer_sip_address_id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_event ("
			"  event_id INT UNSIGNED PRIMARY KEY,"
			"  chat_room_id INT UNSIGNED NOT NULL,"
			"  local_sip_address_id INT UNSIGNED NOT NULL,"
			"  remote_sip_address_id INT UNSIGNED NOT NULL,"

			// See: https://tools.ietf.org/html/rfc5438#section-6.3
			"  imdn_message_id VARCHAR(255) NOT NULL,"

			"  state TINYINT UNSIGNED NOT NULL,"
			"  direction TINYINT UNSIGNED NOT NULL,"
			"  is_secured BOOLEAN NOT NULL,"

			"  FOREIGN KEY (event_id)"
			"    REFERENCES event(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (chat_room_id)"
			"    REFERENCES chat_room(peer_sip_address_id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (local_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (remote_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_participant ("
			"  message_event_id INT UNSIGNED NOT NULL,"
			"  sip_address_id INT UNSIGNED NOT NULL,"
			"  state TINYINT UNSIGNED NOT NULL,"

			"  PRIMARY KEY (message_event_id, sip_address_id),"
			"  FOREIGN KEY (message_event_id)"
			"    REFERENCES message_event(event_id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_content ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  message_event_id INT UNSIGNED NOT NULL,"
			"  content_type_id INT UNSIGNED NOT NULL,"
			"  body TEXT NOT NULL,"

			"  FOREIGN KEY (message_event_id)"
			"    REFERENCES message_event(event_id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (content_type_id)"
			"    REFERENCES content_type(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_content_app_data ("
			"  message_content_id INT UNSIGNED NOT NULL,"
			"  key VARCHAR(255),"
			"  data BLOB,"

			"  PRIMARY KEY (message_content_id, key),"
			"  FOREIGN KEY (message_content_id)"
			"    REFERENCES message_content(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_crypto_data ("
			"  message_event_id INT UNSIGNED NOT NULL,"
			"  key VARCHAR(255),"
			"  data BLOB,"

			"  PRIMARY KEY (message_event_id, key),"
			"  FOREIGN KEY (message_event_id)"
			"    REFERENCES message_event(event_id)"
			"    ON DELETE CASCADE"
			")";

		// Trigger to delete participant_message cache entries.
		string displayedId = Utils::toString(static_cast<int>(ChatMessage::State::Displayed));
		string participantMessageDeleter =
			"CREATE TRIGGER IF NOT EXISTS message_participant_deleter"
			"  AFTER UPDATE OF state ON message_participant FOR EACH ROW"
			"  WHEN NEW.state = ";
		participantMessageDeleter += displayedId;
		participantMessageDeleter += " AND (SELECT COUNT(*) FROM ("
			"    SELECT state FROM message_participant WHERE"
			"    NEW.message_event_id = message_participant.message_event_id"
			"    AND state <> ";
		participantMessageDeleter += displayedId;
		participantMessageDeleter += "    LIMIT 1"
			"  )) = 0"
			"  BEGIN"
			"  DELETE FROM message_participant WHERE NEW.message_event_id = message_participant.message_event_id;"
			"  UPDATE message_event SET state = ";
		participantMessageDeleter += displayedId;
		participantMessageDeleter += " WHERE event_id = NEW.message_event_id;"
			"  END";

		*session << participantMessageDeleter;
	}

	bool MainDb::addEvent (const EventLog &eventLog) {
		if (!isConnected()) {
			lWarning() << "Unable to add event. Not connected.";
			return false;
		}

		// TODO.
		switch (eventLog.getType()) {
			case EventLog::Type::None:
				return false;
			case EventLog::Type::ChatMessage:
			case EventLog::Type::CallStart:
			case EventLog::Type::CallEnd:
			case EventLog::Type::ConferenceCreated:
			case EventLog::Type::ConferenceDestroyed:
			case EventLog::Type::ConferenceParticipantAdded:
			case EventLog::Type::ConferenceParticipantRemoved:
			case EventLog::Type::ConferenceParticipantSetAdmin:
			case EventLog::Type::ConferenceParticipantUnsetAdmin:
				break;
		}

		return true;
	}

	bool MainDb::deleteEvent (const EventLog &eventLog) {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to delete event. Not connected.";
			return false;
		}

		long &id = const_cast<EventLog &>(eventLog).getPrivate()->id;
		if (id < 0)
			return false;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << "DELETE FROM event WHERE id = :id", soci::use(id);
		id = -1;

		L_END_LOG_EXCEPTION

		return id == -1;
	}

	void MainDb::cleanEvents (FilterMask mask) {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to clean events. Not connected.";
			return;
		}

		string query = "DELETE FROM event" +
			buildSqlEventFilter({ MessageFilter, CallFilter, ConferenceFilter }, mask);

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << query;

		L_END_LOG_EXCEPTION
	}

	int MainDb::getEventsCount (FilterMask mask) const {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to get events count. Not connected.";
			return 0;
		}

		string query = "SELECT COUNT(*) FROM event" +
			buildSqlEventFilter({ MessageFilter, CallFilter, ConferenceFilter }, mask);
		int count = 0;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << query, soci::into(count);

		L_END_LOG_EXCEPTION

		return count;
	}

	int MainDb::getMessagesCount (const string &peerAddress) const {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to get messages count. Not connected.";
			return 0;
		}

		int count = 0;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();

		string query = "SELECT COUNT(*) FROM message_event";
		if (peerAddress.empty())
			*session << query, soci::into(count);
		else {
			query += "  WHERE chat_room_id = ("
				"  SELECT id FROM sip_address WHERE value = :peerAddress"
				")";

			*session << query, soci::use(peerAddress), soci::into(count);
		}

		L_END_LOG_EXCEPTION

		return count;
	}

	int MainDb::getUnreadMessagesCount (const string &peerAddress) const {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to get unread messages count. Not connected.";
			return 0;
		}

		int count = 0;

		string query = "SELECT COUNT(*) FROM message_event WHERE";
		if (!peerAddress.empty())
			query += " chat_room_id = ("
				"  SELECT id FROM sip_address WHERE value = :peerAddress"
				") AND ";

		query += " direction = " + Utils::toString(static_cast<int>(ChatMessage::Direction::Incoming)) +
			+ "  AND state <> " + Utils::toString(static_cast<int>(ChatMessage::State::Displayed));

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();

		if (peerAddress.empty())
			*session << query, soci::into(count);
		else
			*session << query, soci::use(peerAddress), soci::into(count);

		L_END_LOG_EXCEPTION

		return count;
	}

	list<shared_ptr<EventLog>> MainDb::getHistory (const string &peerAddress, int nLast, FilterMask mask) const {
		if (!isConnected()) {
			lWarning() << "Unable to get history. Not connected.";
			return list<shared_ptr<EventLog>>();
		}

		// TODO.
		(void)peerAddress;
		(void)nLast;
		(void)mask;
		return list<shared_ptr<EventLog>>();
	}

	list<shared_ptr<EventLog>> MainDb::getHistory (
		const string &peerAddress,
		int begin,
		int end,
		FilterMask mask
	) const {
		if (!isConnected()) {
			lWarning() << "Unable to get history. Not connected.";
			return list<shared_ptr<EventLog>>();
		}

		// TODO.
		(void)peerAddress;
		(void)begin;
		(void)end;
		(void)mask;
		return list<shared_ptr<EventLog>>();
	}

	void MainDb::cleanHistory (const string &peerAddress, FilterMask mask) {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to clean history. Not connected.";
			return;
		}

		string query;
		if (mask == MainDb::NoFilter || mask & MessageFilter)
			query += "SELECT event_id FROM message_event WHERE chat_room_id = ("
				"  SELECT peer_sip_address_id FROM chat_room WHERE peer_sip_address_id = ("
				"    SELECT id FROM sip_address WHERE value = :peerAddress"
				"  )"
				")";

		if (query.empty())
			return;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << "DELETE FROM event WHERE id IN (" + query + ")", soci::use(peerAddress);

		L_END_LOG_EXCEPTION
	}

// -----------------------------------------------------------------------------

shared_ptr<ChatRoom> MainDb::findChatRoom (const string &peerAddress) const {
	L_D();

	const auto it = d->chatRooms.find(peerAddress);
	if (it != d->chatRooms.cend()) {
		try {
			return it->second.lock();
		} catch (const exception &) {
			lError() << "Cannot lock chat room: `" + peerAddress + "`";
		}
		return shared_ptr<ChatRoom>();
	}

	L_BEGIN_LOG_EXCEPTION

	soci::session *session = d->dbSession.getBackendSession<soci::session>();

	tm creationDate;
	tm lastUpdateDate;
	int capabilities;
	string subject;

	*session << "SELECT creation_date, last_update_date, capabilities, subject "
		"  FROM chat_room"
		"  WHERE peer_sip_address_id = ("
		"    SELECT id from sip_address WHERE value = :peerAddress"
		"  )", soci::use(peerAddress), soci::into(creationDate), soci::into(lastUpdateDate),
		soci::use(capabilities), soci::use(subject);

	L_END_LOG_EXCEPTION

	return shared_ptr<ChatRoom>();
}

// -----------------------------------------------------------------------------

	bool MainDb::import (Backend, const string &parameters) {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to import data. Not connected.";
			return 0;
		}

		// Backend is useless, it's sqlite3. (Only available legacy backend.)
		const string uri = "sqlite3://" + parameters;
		DbSession inDbSession = DbSessionProvider::getInstance()->getSession(uri);

		if (!inDbSession) {
			lWarning() << "Unable to connect to: `" << uri << "`.";
			return false;
		}

		soci::session *inSession = inDbSession.getBackendSession<soci::session>();

		// Import messages.
		try {
			soci::rowset<soci::row> messages = (inSession->prepare << "SELECT * FROM history");
			try {
				soci::transaction tr(*d->dbSession.getBackendSession<soci::session>());

				for (const auto &message : messages) {
					const int direction = message.get<int>(LEGACY_MESSAGE_COL_DIRECTION);
					if (direction != 0 && direction != 1) {
						lWarning() << "Unable to import legacy message with invalid direction.";
						continue;
					}

					const int state = message.get<int>(
						LEGACY_MESSAGE_COL_STATE, static_cast<int>(ChatMessage::State::Displayed)
					);
					if (state < 0 || state > static_cast<int>(ChatMessage::State::Displayed)) {
						lWarning() << "Unable to import legacy message with invalid state.";
						continue;
					}

					const tm date = Utils::getLongAsTm(message.get<int>(LEGACY_MESSAGE_COL_DATE, 0));

					bool isNull;
					const string url = getValueFromLegacyMessage<string>(message, LEGACY_MESSAGE_COL_URL, isNull);

					const int contentId = message.get<int>(LEGACY_MESSAGE_COL_CONTENT_ID, -1);
					ContentType contentType(message.get<string>(LEGACY_MESSAGE_COL_CONTENT_TYPE, ""));
					if (!contentType.isValid())
						contentType = contentId != -1
							? ContentType::FileTransfer
							: (isNull ? ContentType::PlainText : ContentType::ExternalBody);
					if (contentType == ContentType::ExternalBody) {
						lInfo() << "Import of external body content is skipped.";
						continue;
					}

					const string text = getValueFromLegacyMessage<string>(message, LEGACY_MESSAGE_COL_TEXT, isNull);

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

						const string appData = getValueFromLegacyMessage<string>(message, LEGACY_MESSAGE_COL_APP_DATA, isNull);
						if (isNull) {
							lWarning() << "Unable to import legacy file message without app data.";
							continue;
						}

						content.setAppData("legacy", appData);
					}

					struct MessageEventReferences references;
					references.eventId = d->insertEvent(EventLog::Type::ChatMessage, date);
					references.localSipAddressId = d->insertSipAddress(message.get<string>(LEGACY_MESSAGE_COL_LOCAL_ADDRESS));
					references.remoteSipAddressId = d->insertSipAddress(message.get<string>(LEGACY_MESSAGE_COL_REMOTE_ADDRESS));
					references.chatRoomId = d->insertChatRoom(
						references.remoteSipAddressId,
						static_cast<int>(ChatRoom::Capabilities::Basic),
						date
					);

					d->insertChatRoomParticipant(references.chatRoomId, references.remoteSipAddressId, false);

					long messageEventId = d->insertMessageEvent (
						references,
						static_cast<ChatMessage::State>(state),
						static_cast<ChatMessage::Direction>(direction),
						message.get<string>(LEGACY_MESSAGE_COL_IMDN_MESSAGE_ID, ""),
						!!message.get<int>(LEGACY_MESSAGE_COL_IS_SECURED, 0),
						{ move(content) }
					);

					if (state != static_cast<int>(ChatMessage::State::Displayed))
						d->insertMessageParticipant(
							messageEventId,
							references.remoteSipAddressId,
							static_cast<ChatMessage::State>(state)
						);
				}

				tr.commit();
			} catch (const exception &e) {
				lInfo() << "Failed to import legacy messages from: `" << uri << "`. (" << e.what() << ")";
				return false;
			}
			lInfo() << "Successful import of legacy messages from: `" << uri << "`.";
		} catch (const exception &) {
			// Table doesn't exist.
			return false;
		}

		return true;
	}

// -----------------------------------------------------------------------------
// No backend.
// -----------------------------------------------------------------------------

#else

	void MainDb::init () {}

	bool MainDb::addEvent (const EventLog &) {
		return false;
	}

	bool MainDb::deleteEvent (const EventLog &) {
		return false;
	}

	void MainDb::cleanEvents (FilterMask) {}

	int MainDb::getEventsCount (FilterMask) const {
		return 0;
	}

	int MainDb::getMessagesCount (const string &) const {
		return 0;
	}

	int MainDb::getUnreadMessagesCount (const string &) const {
		return 0;
	}

	list<shared_ptr<EventLog>> MainDb::getHistory (const string &, int, FilterMask) const {
		return list<shared_ptr<EventLog>>();
	}

	list<shared_ptr<EventLog>> MainDb::getHistory (const string &, int, int, FilterMask) const {
		return list<shared_ptr<EventLog>>();
	}

	void MainDb::cleanHistory (const string &, FilterMask) {}

	shared_ptr<ChatRoom> MainDb::findChatRoom (const string &) const {
		return nullptr;
	}

	bool MainDb::import (Backend, const string &) {
		return false;
	}

#endif // ifdef SOCI_ENABLED

LINPHONE_END_NAMESPACE
