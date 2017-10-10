/*
 * events-db.cpp
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

#include "abstract/abstract-db-p.h"
#include "chat/chat-message.h"
#include "conference/participant.h"
#include "content/content.h"
#include "db/provider/db-session-provider.h"
#include "event-log/call-event.h"
#include "event-log/chat-message-event.h"
#include "event-log/event-log-p.h"
#include "logger/logger.h"

#include "events-db.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

struct MessageEventReferences {
#ifdef SOCI_ENABLED
	long eventId;
	long localSipAddressId;
	long remoteSipAddressId;
	long chatRoomId;
#endif
};

class EventsDbPrivate : public AbstractDbPrivate {
#ifdef SOCI_ENABLED
public:
	long insertSipAddress (const string &sipAddress);
	void insertContent (long messageEventId, const Content &content);
	long insertContentType (const string &contentType);
	long insertEvent (EventLog::Type type, const tm &date);
	long insertChatRoom (long sipAddressId, const tm &date);
	void insertChatRoomParticipant (long chatRoomId, long sipAddressId, bool isAdmin);

	long insertMessageEvent (
		const MessageEventReferences &references,
		ChatMessage::State state,
		ChatMessage::Direction direction,
		const string &imdnMessageId,
		bool isSecured,
		const list<Content> &contents
	);

	void insertMessageParticipant (long messageEventId, long sipAddressId, ChatMessage::State state);

	void importLegacyMessages (const soci::rowset<soci::row> &messages);
#endif

private:
	L_DECLARE_PUBLIC(EventsDb);
};

// -----------------------------------------------------------------------------

EventsDb::EventsDb () : AbstractDb(*new EventsDbPrivate) {}

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

	static constexpr EnumToSql<EventsDb::Filter> eventFilterToSql[] = {
		{ EventsDb::MessageFilter, "1" },
		{ EventsDb::CallFilter, "2" },
		{ EventsDb::ConferenceFilter, "3" }
	};

	static constexpr const char *mapEventFilterToSql (EventsDb::Filter filter) {
		return mapEnumToSql(
			eventFilterToSql, sizeof eventFilterToSql / sizeof eventFilterToSql[0], filter
		);
	}

// -----------------------------------------------------------------------------

	static string buildSqlEventFilter (const list<EventsDb::Filter> &filters, EventsDb::FilterMask mask) {
		L_ASSERT(
			find_if(filters.cbegin(), filters.cend(), [](const EventsDb::Filter &filter) {
					return filter == EventsDb::NoFilter;
				}) == filters.cend()
		);

		if (mask == EventsDb::NoFilter)
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

	long EventsDbPrivate::insertSipAddress (const string &sipAddress) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		long id;
		*session << "SELECT id FROM sip_address WHERE value = :sipAddress", soci::use(sipAddress), soci::into(id);
		if (session->got_data())
			return id;

		*session << "INSERT INTO sip_address (value) VALUES (:sipAddress)", soci::use(sipAddress);
		return q->getLastInsertId();
	}

	void EventsDbPrivate::insertContent (long messageEventId, const Content &content) {
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

	long EventsDbPrivate::insertContentType (const string &contentType) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		long id;
		*session << "SELECT id FROM content_type WHERE value = :contentType", soci::use(contentType), soci::into(id);
		if (session->got_data())
			return id;

		*session << "INSERT INTO content_type (value) VALUES (:contentType)", soci::use(contentType);
		return q->getLastInsertId();
	}

	long EventsDbPrivate::insertEvent (EventLog::Type type, const tm &date) {
		L_Q();
		soci::session *session = dbSession.getBackendSession<soci::session>();

		*session << "INSERT INTO event (type, date) VALUES (:type, :date)",
			soci::use(static_cast<int>(type)), soci::use(date);
		return q->getLastInsertId();
	}

	long EventsDbPrivate::insertChatRoom (long sipAddressId, const tm &date) {
		soci::session *session = dbSession.getBackendSession<soci::session>();

		long id;
		*session << "SELECT peer_sip_address_id FROM chat_room WHERE peer_sip_address_id = :sipAddressId",
			soci::use(sipAddressId), soci::into(id);
		if (!session->got_data())
			*session << "INSERT INTO chat_room (peer_sip_address_id, creation_date, last_update_date, subject) VALUES"
				"  (:sipAddressId, :creationDate, :lastUpdateDate, '')", soci::use(sipAddressId), soci::use(date), soci::use(date);
		else
			*session << "UPDATE chat_room SET last_update_date = :lastUpdateDate WHERE peer_sip_address_id = :sipAddressId",
				soci::use(date), soci::use(sipAddressId);

		return sipAddressId;
	}

	void EventsDbPrivate::insertChatRoomParticipant (long chatRoomId, long sipAddressId, bool isAdmin) {
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

	long EventsDbPrivate::insertMessageEvent (
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

	void EventsDbPrivate::insertMessageParticipant (long messageEventId, long sipAddressId, ChatMessage::State state) {
		soci::session *session = dbSession.getBackendSession<soci::session>();
		soci::statement statement = (
			session->prepare << "UPDATE message_participant SET state = :state"
				"  WHERE message_event_id = :messageEventId AND sip_address_id = :sipAddressId",
				soci::use(static_cast<int>(state)), soci::use(messageEventId), soci::use(sipAddressId)
		);
		statement.execute(true);
		if (statement.get_affected_rows() == 0)
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

	void EventsDbPrivate::importLegacyMessages (const soci::rowset<soci::row> &messages) {
		soci::session *session = dbSession.getBackendSession<soci::session>();

		soci::transaction tr(*session);

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
			references.eventId = insertEvent(EventLog::Type::ChatMessage, date);
			references.localSipAddressId = insertSipAddress(message.get<string>(LEGACY_MESSAGE_COL_LOCAL_ADDRESS));
			references.remoteSipAddressId = insertSipAddress(message.get<string>(LEGACY_MESSAGE_COL_REMOTE_ADDRESS));
			references.chatRoomId = insertChatRoom(references.remoteSipAddressId, date);

			insertChatRoomParticipant(references.chatRoomId, references.remoteSipAddressId, false);

			long messageEventId = insertMessageEvent (
				references,
				static_cast<ChatMessage::State>(state),
				static_cast<ChatMessage::Direction>(direction),
				message.get<string>(LEGACY_MESSAGE_COL_IMDN_MESSAGE_ID, ""),
				!!message.get<int>(LEGACY_MESSAGE_COL_IS_SECURED, 0),
				{ move(content) }
			);

			if (state != static_cast<int>(ChatMessage::State::Displayed))
				insertMessageParticipant(messageEventId, references.remoteSipAddressId, static_cast<ChatMessage::State>(state));
		}

		tr.commit();
	}

// -----------------------------------------------------------------------------

	void EventsDb::init () {
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

			// Chatroom subject.
			"  subject VARCHAR(255),"

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
	}

	bool EventsDb::addEvent (const EventLog &eventLog) {
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

	bool EventsDb::deleteEvent (const EventLog &eventLog) {
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

	void EventsDb::cleanEvents (FilterMask mask) {
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

	int EventsDb::getEventsCount (FilterMask mask) const {
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

	int EventsDb::getMessagesCount (const string &peerAddress) const {
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

	int EventsDb::getUnreadMessagesCount (const string &peerAddress) const {
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

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &peerAddress, int nLast, FilterMask mask) const {
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

	list<shared_ptr<EventLog>> EventsDb::getHistory (
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

	void EventsDb::cleanHistory (const string &peerAddress, FilterMask mask) {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to clean history. Not connected.";
			return;
		}

		string query;
		if (mask == EventsDb::NoFilter || mask & MessageFilter)
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

	bool EventsDb::import (Backend, const string &parameters) {
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
				d->importLegacyMessages(messages);
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

	void EventsDb::init () {}

	bool EventsDb::addEvent (const EventLog &) {
		return false;
	}

	bool EventsDb::deleteEvent (const EventLog &) {
		return false;
	}

	void EventsDb::cleanEvents (FilterMask) {}

	int EventsDb::getEventsCount (FilterMask) const {
		return 0;
	}

	int EventsDb::getMessagesCount (const string &) const {
		return 0;
	}

	int EventsDb::getUnreadMessagesCount (const string &) const {
		return 0;
	}

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &, int, FilterMask) const {
		return list<shared_ptr<EventLog>>();
	}

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &, int, int, FilterMask) const {
		return list<shared_ptr<EventLog>>();
	}

	void EventsDb::cleanHistory (const string &, FilterMask) {}

	bool EventsDb::import (Backend, const string &) {
		return false;
	}

#endif // ifdef SOCI_ENABLED

LINPHONE_END_NAMESPACE
