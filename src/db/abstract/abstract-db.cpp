/*
 * abstract-db.cpp
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

#ifdef __APPLE__
	#include <TargetConditionals.h>
#endif // ifdef __APPLE__

#if (TARGET_OS_IPHONE || defined(__ANDROID__))
	#include <sqlite3.h>
#endif // if (TARGET_OS_IPHONE || defined(__ANDROID__))

#include "abstract-db-p.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

#if (TARGET_OS_IPHONE || defined(__ANDROID__))
	// Force static sqlite3 linking for IOS and Android.
	extern "C" void register_factory_sqlite3();

	static void sqlite3Log (void *, int iErrCode, const char *zMsg) {
		lInfo() << "[sqlite3][" << iErrCode << "]" << zMsg;
	}
#endif // if (TARGET_OS_IPHONE || defined(__ANDROID__))

void AbstractDbPrivate::safeInit () {
	L_Q();
	dbSession.enableForeignKeys(false);
	q->init();
	dbSession.enableForeignKeys(true);
}

AbstractDb::AbstractDb (AbstractDbPrivate &p) : Object(p) {}

bool AbstractDb::connect (Backend backend, const string &parameters) {
	L_D();

	#if (TARGET_OS_IPHONE || defined(__ANDROID__))
		if (backend == Sqlite3) {
			static bool registered = false;
			if (!registered) {
				registered = true;
				register_factory_sqlite3();
				sqlite3_config(SQLITE_CONFIG_LOG, sqlite3Log, nullptr);
			}
		}
	#endif // if (TARGET_OS_IPHONE || defined(__ANDROID__))

	d->backend = backend;
	d->dbSession = DbSession(
		(backend == Mysql ? "mysql://" : "sqlite3://") + parameters
	);

	if (d->dbSession) {
		try {
			d->safeInit();
		} catch (const exception &e) {
			lWarning() << "Unable to init database: " << e.what();

			// Reset session.
			d->dbSession = DbSession();
		}
	}

	return d->dbSession;
}

void AbstractDb::disconnect () {
	L_D();
	d->dbSession = DbSession();
}

bool AbstractDb::forceReconnect () {
	L_D();
	if (!d->dbSession) {
		lWarning() << "Unable to reconnect. Not a valid database session.";
		return false;
	}

	constexpr int retryCount = 2;
	lInfo() << "Trying sql backend reconnect...";

	try {
		for (int i = 0; i < retryCount; ++i) {
			try {
				lInfo() << "Reconnect... Try: " << i;
				d->dbSession.getBackendSession()->reconnect(); // Equivalent to close and connect.
				d->safeInit();
				lInfo() << "Database reconnection successful!";
				return true;
			} catch (const soci::soci_error &e) {
				if (e.get_error_category() != soci::soci_error::connection_error)
				throw e;
			}
		}
	} catch (const exception &e) {
		lError() << "Unable to reconnect: `" << e.what() << "`.";
		return false;
	}

	lError() << "Database reconnection failed!";

	return false;
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

LINPHONE_END_NAMESPACE
