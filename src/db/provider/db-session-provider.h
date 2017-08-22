/*
 * db-session-provider.h
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

#ifndef _DB_SESSION_PROVIDER_H_
#define _DB_SESSION_PROVIDER_H_

#include <string>

#include "db-session.h"
#include "object/singleton.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class DbSessionProviderPrivate;

class DbSessionProvider : public Singleton<DbSessionProvider> {
	friend class Singleton<DbSessionProvider>;

public:
	DbSession getSession (const std::string &uri);

private:
	DbSessionProvider ();

	L_DECLARE_PRIVATE(DbSessionProvider);
	L_DISABLE_COPY(DbSessionProvider);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _DB_SESSION_PROVIDER_H_
