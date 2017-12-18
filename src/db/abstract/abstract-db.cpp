/*
 * abstract-db.cpp
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

#ifdef SOCI_ENABLED
	#include <soci/soci.h>
#endif // ifdef SOCI_ENABLED

#ifdef __APPLE__
	#include <TargetConditionals.h>
#endif // ifdef __APPLE__

#include "linphone/utils/utils.h"

#include "abstract-db-p.h"
#include "db/session/db-session-provider.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

AbstractDb::AbstractDb (AbstractDbPrivate &p) : Object(p) {}

// Force static sqlite3 linking for IOS and Android.
#if TARGET_OS_IPHONE || defined(__ANDROID__)
	extern "C" void register_factory_sqlite3();
#endif // TARGET_OS_IPHONE || defined(__ANDROID__)

bool AbstractDb::connect (Backend backend, const string &parameters) {
	L_D();

	#if TARGET_OS_IPHONE || defined(__ANDROID__)
		if (backend == Sqlite3)
			register_factory_sqlite3();
	#endif // defined(TARGET_OS_IPHONE) || defined(__ANDROID__)

	d->backend = backend;
	d->dbSession = DbSessionProvider::getInstance()->getSession(
		(backend == Mysql ? "mysql://" : "sqlite3://") + parameters
	);

	if (d->dbSession) {
		try {
			enableForeignKeys(false);
			init();
			enableForeignKeys(true);
		} catch (const exception &e) {
			lWarning() << "Unable to init database: " << e.what();

			// Reset session.
			d->dbSession = DbSession();
		}
	}

	return d->dbSession;
}

bool AbstractDb::isConnected () const {
	L_D();
	return d->dbSession;
}

AbstractDb::Backend AbstractDb::getBackend () const {
	L_D();
	return d->backend;
}

bool AbstractDb::import (Backend, const string &) {
	return false;
}

// -----------------------------------------------------------------------------

void AbstractDb::init () {
	// Nothing.
}

// -----------------------------------------------------------------------------

string AbstractDb::primaryKeyStr (const string &type) const {
	L_D();

	switch (d->backend) {
		case Mysql:
			return " " + type + " AUTO_INCREMENT PRIMARY KEY";
		case Sqlite3:
			// See: ROWIDs and the INTEGER PRIMARY KEY
			// https://www.sqlite.org/lang_createtable.html
			return " INTEGER PRIMARY KEY ASC";
	}

	L_ASSERT(false);
	return "";
}

string AbstractDb::primaryKeyRefStr (const string &type) const {
	L_D();

	switch (d->backend) {
		case Mysql:
			return " " + type;
		case Sqlite3:
			return " INTEGER";
	}

	L_ASSERT(false);
	return "";
}

string AbstractDb::varcharPrimaryKeyStr (int length) const {
	L_D();

	switch (d->backend) {
		case Mysql:
			return " VARCHAR(" + Utils::toString(length) + ") AUTO_INCREMENT PRIMARY KEY";
		case Sqlite3:
			return " VARCHAR(" + Utils::toString(length) + ") PRIMARY KEY";
	}

	L_ASSERT(false);
	return "";
}

string AbstractDb::timestampType () const {
	L_D();

	switch (d->backend) {
		case Mysql:
			return " TIMESTAMP";
		case Sqlite3:
			return " DATE";
	}

	L_ASSERT(false);
	return "";
}

string AbstractDb::noLimitValue () const {
	L_D();

	switch (d->backend) {
		case Mysql:
			return "9999999999999999999";
		case Sqlite3:
			return "-1";
	}

	L_ASSERT(false);
	return "";
}

long long AbstractDb::getLastInsertId () const {
	long long id = 0;

	#ifdef SOCI_ENABLED
		L_D();

		string sql;
		switch (d->backend) {
			case Mysql:
				sql = "SELECT LAST_INSERT_ID()";
				break;
			case Sqlite3:
				sql = "SELECT last_insert_rowid()";
				break;
		}

		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		*session << sql, soci::into(id);
	#endif // ifdef SOCI_ENABLED

	return id;
}

void AbstractDb::enableForeignKeys (bool status) {
	#ifdef SOCI_ENABLED
		L_D();
		soci::session *session = d->dbSession.getBackendSession<soci::session>();
		switch (d->backend) {
			case Mysql:
				*session << string("SET FOREIGN_KEY_CHECKS = ") + (status ? "1" : "0");
				break;
			case Sqlite3:
				*session << string("PRAGMA foreign_keys = ") + (status ? "ON" : "OFF");
				break;
		}
	#endif // ifdef SOCI_ENABLED
}

LINPHONE_END_NAMESPACE
