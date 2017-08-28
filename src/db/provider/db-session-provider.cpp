/*
 * db-session-provider.cpp
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

#include <unordered_map>

#ifdef SOCI_ENABLED
	#include <soci/soci.h>
#endif // ifdef SOCI_ENABLED

#include "db-session-p.h"
#include "object/object-p.h"

#include "db-session-provider.h"

#define CLEAN_COUNTER_MAX 1000

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class DbSessionProviderPrivate : public ObjectPrivate {
public:
	typedef pair<weak_ptr<void>, DbSessionPrivate *> InternalSession;
	unordered_map<string, InternalSession> sessions;
	int cleanCounter = 0;
};

DbSessionProvider::DbSessionProvider () : Singleton(*new DbSessionProviderPrivate) {}

DbSession DbSessionProvider::getSession (const string &uri) {
	L_D(DbSessionProvider);

	#ifdef SOCI_ENABLED
		DbSession session(DbSession::Soci);
		try {
			shared_ptr<void> backendSession = d->sessions[uri].first.lock();
			++d->cleanCounter;
			if (!backendSession) { // Create new session.
				backendSession = make_shared<soci::session>(uri);
				DbSessionPrivate *p = session.getPrivate();
				p->backendSession = backendSession;
				p->isValid = true;
				d->sessions[uri] = make_pair(backendSession, p);
			} else // Share session.
				session.setRef(*d->sessions[uri].second);
		} catch (const exception &) {}
	#else
		DbSession session(DbSession::None);
	#endif // ifdef SOCI_ENABLED

	// Remove invalid weak ptrs.
	if (d->cleanCounter >= CLEAN_COUNTER_MAX) {
		d->cleanCounter = 0;

		for (auto it = d->sessions.begin(), itEnd = d->sessions.end(); it != itEnd;) {
			if (it->second.first.expired())
				it = d->sessions.erase(it);
			else
				++it;
		}
	}

	return session;
}

LINPHONE_END_NAMESPACE
