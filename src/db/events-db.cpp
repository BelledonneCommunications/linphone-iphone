/*
 * events-db.cpp
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

#include <algorithm>
#include <ctime>

#ifdef SOCI_ENABLED
	#include <soci/soci.h>
#endif // ifdef SOCI_ENABLED

#include "linphone/utils/utils.h"

#include "abstract/abstract-db-p.h"
#include "chat/chat-message.h"
#include "db/provider/db-session-provider.h"
#include "event-log/call-event.h"
#include "event-log/chat-message-event.h"
#include "logger/logger.h"

#include "events-db.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class EventsDbPrivate : public AbstractDbPrivate {};

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

	struct MessageEventReferences {
		long eventId;
		long localSipAddressId;
		long remoteSipAddressId;
		long chatRoomId;
		long contentTypeId;
	};

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
			sql += " event_type_id = ";
			sql += mapEventFilterToSql(filter);
		}

		return sql;
	}

// -----------------------------------------------------------------------------

	static inline long insertSipAddress (soci::session &session, const string &sipAddress) {
		long id;
		session << "SELECT id FROM sip_address WHERE value = :sipAddress", soci::use(sipAddress), soci::into(id);
		if (session.got_data())
			return id;

		session << "INSERT INTO sip_address (value) VALUES (:sipAddress)", soci::use(sipAddress);
		session.get_last_insert_id("sip_address", id);
		return id;
	}

	static inline long insertContentType (soci::session &session, const string &contentType) {
		long id;
		session << "SELECT id FROM content_type WHERE value = :contentType", soci::use(contentType), soci::into(id);
		if (session.got_data())
			return id;

		session << "INSERT INTO content_type (value) VALUES (:contentType)", soci::use(contentType);
		session.get_last_insert_id("content_type", id);
		return id;
	}

	static inline long insertEvent (soci::session &session, EventLog::Type type, const tm &date) {
		session << "INSERT INTO event (event_type_id, date) VALUES (:eventTypeId, :date)",
			soci::use(static_cast<int>(type)), soci::use(date);
		long id;
		session.get_last_insert_id("event", id);
		return id;
	}

	static inline long insertChatRoom (soci::session &session, long sipAddressId, const tm &date) {
		long id;
		session << "SELECT peer_sip_address_id FROM chat_room WHERE peer_sip_address_id = :sipAddressId",
			soci::use(sipAddressId), soci::into(id);
		if (!session.got_data())
			session << "INSERT INTO chat_room (peer_sip_address_id, creation_date, last_update_date, subject) VALUES"
				"  (:sipAddressId, :creationDate, :lastUpdateDate, '')", soci::use(sipAddressId), soci::use(date), soci::use(date);
		else
			session << "UPDATE chat_room SET last_update_date = :lastUpdateDate WHERE peer_sip_address_id = :sipAddressId",
				soci::use(date), soci::use(sipAddressId);
		return sipAddressId;
	}

	static inline long insertMessageEvent (
		soci::session &session,
		const MessageEventReferences &references,
		ChatMessage::State state,
		ChatMessage::Direction direction,
		const string &imdnMessageId,
		bool isSecured,
		const string *text = nullptr
	) {
		soci::indicator textIndicator = text ? soci::i_ok : soci::i_null;

		session << "INSERT INTO message_event ("
			"  event_id, chat_room_id, local_sip_address_id, remote_sip_address_id, content_type_id,"
			"  state, direction, imdn_message_id, is_secured, text"
			") VALUES ("
			"  :eventId, :chatRoomId, :localSipaddressId, :remoteSipaddressId, :contentTypeId,"
			"  :state, :direction, :imdnMessageId, :isSecured, :text"
			")", soci::use(references.eventId), soci::use(references.chatRoomId), soci::use(references.localSipAddressId),
			soci::use(references.remoteSipAddressId), soci::use(references.contentTypeId),
			soci::use(static_cast<int>(state)), soci::use(static_cast<int>(direction)), soci::use(imdnMessageId),
			soci::use(isSecured ? 1 : 0), soci::use(text ? *text : string(), textIndicator);
		long id;
		return session.get_last_insert_id("message_event", id);
		return id;
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
			"  event_type_id TINYINT UNSIGNED NOT NULL,"
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
			"CREATE TABLE IF NOT EXISTS message_event ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  event_id INT UNSIGNED NOT NULL,"
			"  chat_room_id INT UNSIGNED NOT NULL,"

			"  local_sip_address_id INT UNSIGNED NOT NULL,"
			"  remote_sip_address_id INT UNSIGNED NOT NULL,"

			"  content_type_id INT UNSIGNED NOT NULL,"

			// See: https://tools.ietf.org/html/rfc5438#section-6.3
			"  imdn_message_id VARCHAR(255) NOT NULL,"

			"  state TINYINT UNSIGNED NOT NULL,"
			"  direction TINYINT UNSIGNED NOT NULL,"
			"  is_secured BOOLEAN NOT NULL,"

			"  text TEXT,"

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
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (content_type_id)"
			"    REFERENCES content_type(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_crypto_data ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  message_event_id INT UNSIGNED NOT NULL,"
			"  data BLOB,"

			"  FOREIGN KEY (message_event_id)"
			"    REFERENCES message_event(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_file_info ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  message_id INT UNSIGNED NOT NULL,"
			"  content_type_id INT UNSIGNED NOT NULL,"

			"  name VARCHAR(255) NOT NULL,"
			"  size INT UNSIGNED NOT NULL,"
			"  url VARCHAR(255) NOT NULL,"

			"  FOREIGN KEY (message_id)"
			"    REFERENCES message(id)"
			"    ON DELETE CASCADE"
			"  FOREIGN KEY (content_type_id)"
			"    REFERENCES content_type(id)"
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
		if (!isConnected()) {
			lWarning() << "Unable to delete event. Not connected.";
			return false;
		}

		// TODO.
		(void)eventLog;
		return true;
	}

	void EventsDb::cleanEvents (FilterMask mask) {
		if (!isConnected()) {
			lWarning() << "Unable to clean events. Not connected.";
			return;
		}

		// TODO.
		(void)mask;
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

	int EventsDb::getMessagesCount (const string &remoteAddress) const {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to get messages count. Not connected.";
			return 0;
		}

		string query = "SELECT COUNT(*) FROM message_event";
		if (!remoteAddress.empty())
			query += "  WHERE chat_room_id = ("
				"    SELECT id FROM dialog WHERE remote_sip_address_id =("
				"      SELECT id FROM sip_address WHERE value = :remote_address"
				"    )"
				"  )";
		int count = 0;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << query, soci::use(remoteAddress), soci::into(count);

		L_END_LOG_EXCEPTION

		return count;
	}

	int EventsDb::getUnreadMessagesCount (const string &remoteAddress) const {
		L_D();

		if (!isConnected()) {
			lWarning() << "Unable to get unread messages count. Not connected.";
			return 0;
		}

		string query = "SELECT COUNT(*) FROM message_event";
		if (!remoteAddress.empty())
			query += "  WHERE chat_room_id = ("
				"    SELECT id FROM dialog WHERE remote_sip_address_id = ("
				"      SELECT id FROM sip_address WHERE value = :remote_address"
				"    )"
				"  )"
				"  AND direction = " + Utils::toString(static_cast<int>(ChatMessage::Direction::Incoming)) +
				"  AND state = " + Utils::toString(static_cast<int>(ChatMessage::State::Displayed));
		int count = 0;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << query, soci::use(remoteAddress), soci::into(count);

		L_END_LOG_EXCEPTION

		return count;
	}

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &remoteAddress, int nLast, FilterMask mask) const {
		if (!isConnected()) {
			lWarning() << "Unable to get history. Not connected.";
			return list<shared_ptr<EventLog>>();
		}

		// TODO.
		(void)remoteAddress;
		(void)nLast;
		(void)mask;
		return list<shared_ptr<EventLog>>();
	}

	list<shared_ptr<EventLog>> EventsDb::getHistory (
		const string &remoteAddress,
		int begin,
		int end,
		FilterMask mask
	) const {
		if (!isConnected()) {
			lWarning() << "Unable to get history. Not connected.";
			return list<shared_ptr<EventLog>>();
		}

		// TODO.
		(void)remoteAddress;
		(void)begin;
		(void)end;
		(void)mask;
		return list<shared_ptr<EventLog>>();
	}

	void EventsDb::cleanHistory (const string &remoteAddress) {
		if (!isConnected()) {
			lWarning() << "Unable to clean history. Not connected.";
			return;
		}

		// TODO.
		(void)remoteAddress;
	}

// -----------------------------------------------------------------------------

	template<typename T>
	static T getValueFromLegacyMessage (const soci::row &message, int index, bool &isNull) {
		isNull = false;

		try {
			return message.get<T>(index);
		} catch (const exception &) {
			isNull = true;
		}

		return T();
	}

	static void importLegacyMessages (
		soci::session *session,
		const string &insertOrIgnoreStr,
		const soci::rowset<soci::row> &messages
	) {
		soci::transaction tr(*session);

		for (const auto &message : messages) {
			const int direction = message.get<int>(3) + 1;
			if (direction != 1 && direction != 2) {
				lWarning() << "Unable to import legacy message with invalid direction.";
				return;
			}

			const int state = message.get<int>(7, static_cast<int>(ChatMessage::State::Displayed));

			const tm date = Utils::getLongAsTm(message.get<int>(9, 0));

			const bool noUrl = false;
			const string url = getValueFromLegacyMessage<string>(message, 8, const_cast<bool &>(noUrl));

			const string contentType = message.get<string>(
				13,
				message.get<int>(11, -1) != -1
					? "application/vnd.gsma.rcs-ft-http+xml"
					: (noUrl ? "text/plain" : "message/external-body")
			);

			const bool noText = false;
			const string text = getValueFromLegacyMessage<string>(message, 4, const_cast<bool &>(noText));

			struct MessageEventReferences references;
			references.eventId = insertEvent(*session, EventLog::Type::ChatMessage, date);
			references.localSipAddressId = insertSipAddress(*session, message.get<string>(1));
			references.remoteSipAddressId = insertSipAddress(*session, message.get<string>(2));
			references.chatRoomId = insertChatRoom(*session, references.remoteSipAddressId, date);
			references.contentTypeId = insertContentType(*session, contentType);

			insertMessageEvent (
				*session,
				references,
				static_cast<ChatMessage::State>(state),
				static_cast<ChatMessage::Direction>(direction),
				message.get<string>(12, ""),
				!!message.get<int>(14, 0),
				noText ? nullptr : &text
			);

			const bool noAppData = false;
			const string appData = getValueFromLegacyMessage<string>(message, 10, const_cast<bool &>(noAppData));
			(void)text;
			(void)appData;
		}

		tr.commit();
	}

	bool EventsDb::import (Backend, const string &parameters) {
		L_D();

		// Backend is useless, it's sqlite3. (Only available legacy backend.)
		const string uri = "sqlite3://" + parameters;
		DbSession inDbSession = DbSessionProvider::getInstance()->getSession(uri);

		if (!inDbSession) {
			lWarning() << "Unable to connect to: `" << uri << "`.";
			return false;
		}

		soci::session *outSession = d->dbSession.getBackendSession<soci::session>();
		soci::session *inSession = inDbSession.getBackendSession<soci::session>();

		// Import messages.
		try {
			soci::rowset<soci::row> messages = (inSession->prepare << "SELECT * FROM history ORDER BY id DESC");
			try {
				importLegacyMessages(outSession, insertOrIgnoreStr(), messages);
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

	void EventsDb::cleanHistory (const string &) {}

#endif // ifdef SOCI_ENABLED

LINPHONE_END_NAMESPACE
