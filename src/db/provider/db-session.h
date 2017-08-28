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

#ifdef SOCI_ENABLED
	namespace soci {
		class session;
	}
#endif // ifdef SOCI_ENABLED

LINPHONE_BEGIN_NAMESPACE

class DbSessionPrivate;

class DbSession : public ClonableObject {
	friend class DbSessionProvider;

public:
	enum Type {
		None,
		Soci
	};

	DbSession (Type type = None);
	DbSession (const DbSession &src);

	DbSession &operator= (const DbSession &src);

	operator bool () const;

	Type getBackendType () const;

	template<typename T>
	T *getBackendSession () const;

private:
	void *getBackendSession () const;

	L_DECLARE_PRIVATE(DbSession);
};

// -----------------------------------------------------------------------------

template<typename T>
struct TypeOfDbSession {
	static const DbSession::Type type = DbSession::None;
};

#ifdef SOCI_ENABLED

	template<>
	struct TypeOfDbSession<::soci::session> {
		static const DbSession::Type type = DbSession::Soci;
	};

#endif // ifdef SOCI_ENABLED

template<typename T>
T *DbSession::getBackendSession () const {
	typedef TypeOfDbSession<T> Type;
	static_assert(Type::type != DbSession::None, "Unable to get backend session, invalid type.");
	if (getBackendType() != Type::type)
		return nullptr;
	return static_cast<T *>(getBackendSession());
}

LINPHONE_END_NAMESPACE

#endif // ifndef _DB_SESSION_H_
