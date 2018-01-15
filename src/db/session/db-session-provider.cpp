/*
 * db-session-provider.cpp
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

#ifdef SOCI_ENABLED
	#include <soci/soci.h>
#endif // ifdef SOCI_ENABLED

#include "db-session-p.h"
#include "logger/logger.h"
#include "object/object-p.h"

#include "db-session-provider.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

DbSessionProvider::DbSessionProvider () : Singleton(*new ObjectPrivate) {}

DbSession DbSessionProvider::getSession (const string &uri) {
	DbSession session(DbSession::None);

	#ifdef SOCI_ENABLED
		try {
			session = DbSession(DbSession::Soci);
			DbSessionPrivate *p = session.getPrivate();
			p->backendSession = make_shared<soci::session>(uri);
			p->isValid = true;
		} catch (const exception &e) {
			session = DbSession(DbSession::None);
			lWarning() << "Unable to get db session: " << e.what();
		}
	#else
		lWarning() << "Unable to get db session: soci not enabled.";
	#endif // ifdef SOCI_ENABLED

	return session;
}

LINPHONE_END_NAMESPACE
