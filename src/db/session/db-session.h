/*
 * db-session.h
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

#ifndef _L_DB_SESSION_H_
#define _L_DB_SESSION_H_

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

	explicit DbSession (Type type = None);
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

#endif // ifndef _L_DB_SESSION_H_
