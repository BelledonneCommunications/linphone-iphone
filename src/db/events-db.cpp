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

#ifdef SOCI_ENABLED
	#include <soci/soci.h>
#endif // ifdef SOCI_ENABLED

#include "abstract/abstract-db-p.h"
#include "chat/chat-message.h"
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

// -----------------------------------------------------------------------------

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

	static constexpr EnumToSql<ChatMessage::State> messageStateToSql[] = {
		{ ChatMessage::Idle, "1" },
		{ ChatMessage::InProgress, "2" },
		{ ChatMessage::Delivered, "3" },
		{ ChatMessage::NotDelivered, "4" },
		{ ChatMessage::FileTransferError, "5" },
		{ ChatMessage::FileTransferDone, "6" },
		{ ChatMessage::DeliveredToUser, "7" },
		{ ChatMessage::Displayed, "8" }
	};

	static constexpr const char *mapMessageStateToSql (ChatMessage::State state) {
		return mapEnumToSql(
			messageStateToSql, sizeof messageStateToSql / sizeof messageStateToSql[0], state
		);
	}

	static constexpr const char *mapMessageDirectionToSql (ChatMessage::Direction direction) {
		return direction == ChatMessage::Direction::Incoming ? "1" : "2";
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
			sql += " event_type_id = ";
			sql += mapEventFilterToSql(filter);
		}

		return sql;
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
			"CREATE TABLE IF NOT EXISTS event_type ("
			"  id TINYINT UNSIGNED,"
			"  value VARCHAR(255) UNIQUE NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS event ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  event_type_id TINYINT UNSIGNED NOT NULL,"
			"  timestamp TIMESTAMP NOT NULL,"
			"  FOREIGN KEY (event_type_id)"
			"    REFERENCES event_type(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_state ("
			"  id TINYINT UNSIGNED,"
			"  value VARCHAR(255) UNIQUE NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_direction ("
			"  id TINYINT UNSIGNED,"
			"  value VARCHAR(255) UNIQUE NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS dialog ("
			"  id" + primaryKeyAutoIncrementStr() + ","

			// Sip address used to communicate.
			"  local_sip_address_id INT UNSIGNED NOT NULL,"

			// Server (for conference) or user sip address.
			"  remote_sip_address_id INT UNSIGNED NOT NULL,"

			// Dialog creation date.
			"  creation_timestamp TIMESTAMP NOT NULL,"

			// Last event timestamp (call, message...).
			"  last_update_timestamp TIMESTAMP NOT NULL,"
			"  FOREIGN KEY (local_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (remote_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_event ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  event_id INT UNSIGNED NOT NULL,"
			"  dialog_id INT UNSIGNED NOT NULL,"
			"  state_id TINYINT UNSIGNED NOT NULL,"
			"  direction_id TINYINT UNSIGNED NOT NULL,"
			"  sender_sip_address_id INT UNSIGNED NOT NULL,"

			// See: https://tools.ietf.org/html/rfc5438#section-6.3
			"  imdn_message_id VARCHAR(255) NOT NULL,"

			"  is_secured BOOLEAN NOT NULL,"

			// Content type of text. (Html or text for example.)
			"  content_type VARCHAR(255) NOT NULL,"
			"  text TEXT,"

			// App user data.
			"  app_data VARCHAR(2048),"

			"  FOREIGN KEY (event_id)"
			"    REFERENCES event(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (dialog_id)"
			"    REFERENCES dialog(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (state_id)"
			"    REFERENCES message_state(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (direction_id)"
			"    REFERENCES message_direction(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (sender_sip_address_id)"
			"    REFERENCES sip_address(id)"
			"    ON DELETE CASCADE"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_file_info ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  message_id INT UNSIGNED NOT NULL,"

			// File content type.
			"  content_type VARCHAR(255) NOT NULL,"

			// File name.
			"  name VARCHAR(255) NOT NULL,"

			// File size.
			"  size INT UNSIGNED NOT NULL,"

			// File url.
			"  url VARCHAR(255) NOT NULL,"
			"  key VARCHAR(4096),"
			"  key_size INT UNSIGNED,"
			"  FOREIGN KEY (message_id)"
			"    REFERENCES message(id)"
			"    ON DELETE CASCADE"
			")";

		{
			string query = getBackend() == Mysql
				? "INSERT INTO event_type (id, value)"
				: "INSERT OR IGNORE INTO event_type (id, value)";
			query += "VALUES"
				"(1, \"Message\"),"
				"(2, \"Call\"),"
				"(3, \"Conference\")";
			if (getBackend() == Mysql)
				query += "ON DUPLICATE KEY UPDATE value = VALUES(value)";

			*session << query;
		}

		{
			string query = getBackend() == Mysql
				? "INSERT INTO message_direction (id, value)"
				: "INSERT OR IGNORE INTO message_direction (id, value)";
			query += "VALUES"
				"(1, \"Incoming\"),"
				"(2, \"Outgoing\")";
			if (getBackend() == Mysql)
				query += "ON DUPLICATE KEY UPDATE value = VALUES(value)";

			*session << query;
		}

		{
			string query = getBackend() == Mysql
				? "INSERT INTO message_state (id, value)"
				: "INSERT OR IGNORE INTO message_state (id, value)";
			query += "VALUES"
				"(1, \"Idle\"),"
				"(2, \"InProgress\"),"
				"(3, \"Delivered\"),"
				"(4, \"NotDelivered\"),"
				"(5, \"FileTransferError\"),"
				"(6, \"FileTransferDone\"),"
				"(7, \"DeliveredToUser\"),"
				"(8, \"Displayed\")";
			if (getBackend() == Mysql)
				query += "ON DUPLICATE KEY UPDATE value = VALUES(value)";

			*session << query;
		}
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
			query += "  WHERE dialog_id = ("
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
			query += "  WHERE dialog_id = ("
				"    SELECT id FROM dialog WHERE remote_sip_address_id = ("
				"      SELECT id FROM sip_address WHERE value = :remote_address"
				"    )"
				"  )"
				"  AND direction_id = " + string(mapMessageDirectionToSql(ChatMessage::Incoming)) +
				"  AND state_id = " + string(mapMessageStateToSql(ChatMessage::Displayed));
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

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &remoteAddress, int begin, int end, FilterMask mask) const {
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
