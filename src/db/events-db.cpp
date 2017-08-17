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

#include "abstract/abstract-db-p.h"

#include "events-db.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class EventsDbPrivate : public AbstractDbPrivate {};

// -----------------------------------------------------------------------------

EventsDb::EventsDb () : AbstractDb(*new EventsDbPrivate) {}

void EventsDb::init () {
	#ifdef SOCI_ENABLED

	L_D(EventsDb);

	d->session <<
		"CREATE TABLE IF NOT EXISTS sip_address ("
		"  id" + primaryKeyAutoIncrementStr() + ","
		"  value VARCHAR(255) NOT NULL"
		")";

	d->session <<
		"CREATE TABLE IF NOT EXISTS event ("
		"  id" + primaryKeyAutoIncrementStr() + ","
		"  timestamp TIMESTAMP NOT NULL"
		")";

	d->session <<
		"CREATE TABLE IF NOT EXISTS message_status ("
		"  id" + primaryKeyAutoIncrementStr() + ","
		"  status VARCHAR(255) NOT NULL"
		")";

	d->session <<
		"CREATE TABLE IF NOT EXISTS message_direction ("
		"  id" + primaryKeyAutoIncrementStr() + ","
		"  direction VARCHAR(255) NOT NULL"
		")";

	d->session <<
		"CREATE TABLE IF NOT EXISTS dialog ("
		"  local_sip_address_id BIGINT UNSIGNED NOT NULL," // Sip address used to communicate.
		"  remote_sip_address_id BIGINT UNSIGNED NOT NULL," // Server (for conference) or user sip address.
		"  creation_timestamp TIMESTAMP NOT NULL," // Dialog creation date.
		"  last_update_timestamp TIMESTAMP NOT NULL," // Last event timestamp (call, message...).
		"  FOREIGN KEY (local_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (remote_sip_address_id)"
		"    REFERENCES sip_address(id)"
		"    ON DELETE CASCADE"
		")";

	d->session <<
		"CREATE TABLE IF NOT EXISTS message_event ("
		"  id" + primaryKeyAutoIncrementStr() + ","
		"  dialog_id BIGINT UNSIGNED NOT NULL,"
		"  status_id TINYINT UNSIGNED NOT NULL,"
		"  direction_id TINYINT UNSIGNED NOT NULL,"
		"  imdn_message_id VARCHAR(255) NOT NULL," // See: https://tools.ietf.org/html/rfc5438#section-6.3
		"  content_type VARCHAR(255) NOT NULL,"
		"  is_secured BOOLEAN NOT NULL,"
		"  app_data VARCHAR(2048),"
		"  FOREIGN KEY (dialog_id)"
		"    REFERENCES dialog(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (status_id)"
		"    REFERENCES message_status(id)"
		"    ON DELETE CASCADE,"
		"  FOREIGN KEY (direction_id)"
		"    REFERENCES message_direction(id)"
		"    ON DELETE CASCADE"
		")";

	#endif // ifndef SOCI_ENABLED
}

LINPHONE_END_NAMESPACE
