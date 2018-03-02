/*
 * db-session.cpp
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

#include "linphone/utils/utils.h"

#include "db-session.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class DbSessionPrivate {
public:
	enum class Backend {
		None,
		Mysql,
		Sqlite3
	} backend = Backend::None;

	std::unique_ptr<soci::session> backendSession;
};

DbSession::DbSession () : mPrivate(new DbSessionPrivate) {}

DbSession::DbSession (const string &uri) : DbSession() {
	try {
		L_D();
		d->backendSession = makeUnique<soci::session>(uri);
		d->backend = !uri.find("mysql") ? DbSessionPrivate::Backend::Mysql : DbSessionPrivate::Backend::Sqlite3;
	} catch (const exception &e) {
		lWarning() << "Unable to build db session with uri: " << e.what();
	}
}

DbSession::DbSession (DbSession &&other) : mPrivate(other.mPrivate) {
	other.mPrivate = nullptr;
}

DbSession::~DbSession () {
	delete mPrivate;
}

DbSession &DbSession::operator= (DbSession &&other) {
	std::swap(mPrivate, other.mPrivate);
	return *this;
}

DbSession::operator bool () const {
	L_D();
	return d->backend != DbSessionPrivate::Backend::None;
}

soci::session *DbSession::getBackendSession () const {
	L_D();
	return d->backendSession.get();
}

string DbSession::primaryKeyStr (const string &type) const {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			return " " + type + " AUTO_INCREMENT PRIMARY KEY";
		case DbSessionPrivate::Backend::Sqlite3:
			// See: ROWIDs and the INTEGER PRIMARY KEY
			// https://www.sqlite.org/lang_createtable.html
			return " INTEGER PRIMARY KEY ASC";
		case DbSessionPrivate::Backend::None:
			return "";
	}

	L_ASSERT(false);
	return "";
}

string DbSession::primaryKeyRefStr (const string &type) const {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			return " " + type;
		case DbSessionPrivate::Backend::Sqlite3:
			return " INTEGER";
		case DbSessionPrivate::Backend::None:
			return "";
	}

	L_ASSERT(false);
	return "";
}

string DbSession::varcharPrimaryKeyStr (int length) const {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			return " VARCHAR(" + Utils::toString(length) + ") PRIMARY KEY";
		case DbSessionPrivate::Backend::Sqlite3:
			return " VARCHAR(" + Utils::toString(length) + ") PRIMARY KEY";
		case DbSessionPrivate::Backend::None:
			return "";
	}

	L_ASSERT(false);
	return "";
}

string DbSession::timestampType () const {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			return " TIMESTAMP";
		case DbSessionPrivate::Backend::Sqlite3:
			return " DATE";
		case DbSessionPrivate::Backend::None:
			return "";
	}

	L_ASSERT(false);
	return "";
}

string DbSession::noLimitValue () const {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			return "9999999999999999999";
		case DbSessionPrivate::Backend::Sqlite3:
			return "-1";
		case DbSessionPrivate::Backend::None:
			return "";
	}

	L_ASSERT(false);
	return "";
}

long long DbSession::getLastInsertId () const {
	long long id = 0;

	L_D();

	string sql;
	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			sql = "SELECT LAST_INSERT_ID()";
			break;
		case DbSessionPrivate::Backend::Sqlite3:
			sql = "SELECT last_insert_rowid()";
			break;
		case DbSessionPrivate::Backend::None:
			break;
	}

	*d->backendSession << sql, soci::into(id);

	return id;
}

void DbSession::enableForeignKeys (bool status) {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			*d->backendSession << string("SET FOREIGN_KEY_CHECKS = ") + (status ? "1" : "0");
			break;
		case DbSessionPrivate::Backend::Sqlite3:
			*d->backendSession << string("PRAGMA foreign_keys = ") + (status ? "ON" : "OFF");
			break;
		case DbSessionPrivate::Backend::None:
			break;
	}
}

bool DbSession::checkTableExists (const string &table) const {
	L_D();

	soci::session *session = d->backendSession.get();
	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			*session << "SHOW TABLES LIKE :table", soci::use(table);
			return session->got_data() > 0;
		case DbSessionPrivate::Backend::Sqlite3:
			*session << "SELECT name FROM sqlite_master WHERE type='table' AND name=:table", soci::use(table);
			return session->got_data() > 0;
		case DbSessionPrivate::Backend::None:
			return false;
	}

	L_ASSERT(false);
	return false;
}

long long DbSession::resolveId (const soci::row &row, int col) const {
	L_D();

	switch (d->backend) {
		case DbSessionPrivate::Backend::Mysql:
			return static_cast<long long>(row.get<unsigned long long>(0));
		case DbSessionPrivate::Backend::Sqlite3:
			return static_cast<long long>(row.get<int>(0));
		case DbSessionPrivate::Backend::None:
			return 0;
	}

	L_ASSERT(false);
	return 0;
}

LINPHONE_END_NAMESPACE
