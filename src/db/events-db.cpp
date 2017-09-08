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
#include "event-log/call-event.h"
#include "event-log/message-event.h"
#include "logger/logger.h"
#include "message/message.h"

#include "events-db.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class EventsDbPrivate : public AbstractDbPrivate {};

// -----------------------------------------------------------------------------

EventsDb::EventsDb () : AbstractDb(*new EventsDbPrivate) {}

// -----------------------------------------------------------------------------
// Soci backend.
// -----------------------------------------------------------------------------

#ifdef SOCI_ENABLED

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

	static constexpr EnumToSql<Message::State> messageStateToSql[] = {
		{ Message::Idle, "1" },
		{ Message::InProgress, "2" },
		{ Message::Delivered, "3" },
		{ Message::NotDelivered, "4" },
		{ Message::FileTransferError, "5" },
		{ Message::FileTransferDone, "6" },
		{ Message::DeliveredToUser, "7" },
		{ Message::Displayed, "8" }
	};

	static constexpr const char *mapMessageStateToSql (Message::State state) {
		return mapEnumToSql(
			messageStateToSql, sizeof messageStateToSql / sizeof messageStateToSql[0], state
		);
	}

	static constexpr const char *mapMessageDirectionToSql (Message::Direction direction) {
		return direction == Message::Direction::Incoming ? "1" : "2";
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
		L_D(EventsDb);
		soci::session *session = d->dbSession.getBackendSession<soci::session>();

		*session <<
			"CREATE TABLE IF NOT EXISTS sip_address ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  value VARCHAR(255) NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS event_type ("
			"  id" + primaryKeyAutoIncrementStr("TINYINT") + ","
			"  value VARCHAR(255) NOT NULL"
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
			"  id" + primaryKeyAutoIncrementStr("TINYINT") + ","
			"  state VARCHAR(255) NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS message_direction ("
			"  id" + primaryKeyAutoIncrementStr("TINYINT") + ","
			"  direction VARCHAR(255) NOT NULL"
			")";

		*session <<
			"CREATE TABLE IF NOT EXISTS dialog ("
			"  id" + primaryKeyAutoIncrementStr() + ","
			"  local_sip_address_id INT UNSIGNED NOT NULL," // Sip address used to communicate.
			"  remote_sip_address_id INT UNSIGNED NOT NULL," // Server (for conference) or user sip address.
			"  creation_timestamp TIMESTAMP NOT NULL," // Dialog creation date.
			"  last_update_timestamp TIMESTAMP NOT NULL," // Last event timestamp (call, message...).
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
			"  dialog_id INT UNSIGNED NOT NULL,"
			"  state_id TINYINT UNSIGNED NOT NULL,"
			"  direction_id TINYINT UNSIGNED NOT NULL,"
			"  imdn_message_id VARCHAR(255) NOT NULL," // See: https://tools.ietf.org/html/rfc5438#section-6.3
			"  content_type VARCHAR(255) NOT NULL,"
			"  is_secured BOOLEAN NOT NULL,"
			"  app_data VARCHAR(2048),"
			"  FOREIGN KEY (dialog_id)"
			"    REFERENCES dialog(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (state_id)"
			"    REFERENCES message_state(id)"
			"    ON DELETE CASCADE,"
			"  FOREIGN KEY (direction_id)"
			"    REFERENCES message_direction(id)"
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
		// TODO.
		switch (eventLog.getType()) {
			case EventLog::TypeNone:
				return false;
			case EventLog::TypeMessage:
			case EventLog::TypeCallStart:
			case EventLog::TypeCallEnd:
			case EventLog::TypeConferenceCreated:
			case EventLog::TypeConferenceDestroyed:
			case EventLog::TypeConferenceParticipantAdded:
			case EventLog::TypeConferenceParticipantRemoved:
			case EventLog::TypeConferenceParticipantSetAdmin:
			case EventLog::TypeConferenceParticipantUnsetAdmin:
				break;
		}

		return true;
	}

	bool EventsDb::deleteEvent (const EventLog &eventLog) {
		// TODO.
		(void)eventLog;
		return true;
	}

	void EventsDb::cleanEvents (FilterMask mask) {
		// TODO.
		(void)mask;
	}

	int EventsDb::getEventsCount (FilterMask mask) const {
		L_D(const EventsDb);

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
		L_D(const EventsDb);

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
		L_D(const EventsDb);

		string query = "SELECT COUNT(*) FROM message_event";
		if (!remoteAddress.empty())
			query += "  WHERE dialog_id = ("
				"    SELECT id FROM dialog WHERE remote_sip_address_id = ("
				"      SELECT id FROM sip_address WHERE value = :remote_address"
				"    )"
				"  )"
				"  AND direction_id = " + string(mapMessageDirectionToSql(Message::Incoming)) +
				"  AND state_id = " + string(mapMessageStateToSql(Message::Displayed));
		int count = 0;

		L_BEGIN_LOG_EXCEPTION

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << query, soci::use(remoteAddress), soci::into(count);

		L_END_LOG_EXCEPTION

		return count;
	}

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &remoteAddress, int nLast, FilterMask mask) const {
		// TODO.
		(void)remoteAddress;
		(void)nLast;
		(void)mask;
		return list<shared_ptr<EventLog>>();
	}

	list<shared_ptr<EventLog>> EventsDb::getHistory (const string &remoteAddress, int begin, int end, FilterMask mask) const {
		// TODO.
		(void)remoteAddress;
		(void)begin;
		(void)end;
		(void)mask;
		return list<shared_ptr<EventLog>>();
	}

	void EventsDb::cleanHistory (const string &remoteAddress) {
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
