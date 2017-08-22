/*
 * db-session.h
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

#ifndef _DB_SESSION_H_
#define _DB_SESSION_H_

#include <string>

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class DbSessionPrivate;

class DbSession : public ClonableObject {
	friend class DbSessionProvider;

public:
	DbSession ();
	DbSession (const DbSession &src);

	DbSession &operator= (const DbSession &src);

	operator bool () const;

private:
	L_DECLARE_PRIVATE(DbSession);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _DB_SESSION_H_
