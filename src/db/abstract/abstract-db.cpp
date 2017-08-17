/*
 * abstract-db.cpp
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

#include "abstract-db-p.h"
#include "logger/logger.h"

#include "abstract-db.h"

using namespace std;

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

AbstractDb::AbstractDb (AbstractDbPrivate &p) : Object(*new AbstractDbPrivate) {}

bool AbstractDb::connect (Backend backend, const std::string &parameters) {
	L_D(AbstractDb);

	#ifdef SOCI_ENABLED

	try {
		if (d->isConnected) {
			d->session.close();
			d->isConnected = false;
		}

		d->session.open(backend == Mysql ? "mysql" : "sqlite3", parameters);
		init();
	} catch (const exception &e) {
		return false;
	}

	return true;

	#else

	lWarning() << "Cannot use AbstractDb. Soci is not enabled.";
	return false;

	#endif // ifndef SOCI_ENABLED
}

bool AbstractDb::isConnected () const {
	L_D(const AbstractDb);
	return d->isConnected;
}

AbstractDb::Backend AbstractDb::getBackend () const {
	L_D(const AbstractDb);
	return d->backend;
}

// -----------------------------------------------------------------------------

void AbstractDb::init () {
	// Nothing.
}

// -----------------------------------------------------------------------------

string AbstractDb::primaryKeyAutoIncrementStr () const {
	L_D(const AbstractDb);

	switch (d->backend) {
		case Mysql:
			return " BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT";
		case Sqlite3:
			return " INTEGER PRIMARY KEY AUTOINCREMENT";
	}

	return "";
}

LINPHONE_END_NAMESPACE
