/*
 * db-session-p.h
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

#ifndef _DB_SESSION_P_H_
#define _DB_SESSION_P_H_

#ifdef SOCI_ENABLED
	#include <memory>
#endif // ifdef SOCI_ENABLED

#include "db-session.h"
#include "object/clonable-object-p.h"

// =============================================================================

#ifdef SOCI_ENABLED
	namespace soci {
		class session;
	}
#endif // ifdef SOCI_ENABLED

LINPHONE_BEGIN_NAMESPACE

class DbSessionPrivate : public ClonableObjectPrivate {
	friend class DbSessionProvider;

private:
	bool isValid = false;

	#ifdef SOCI_ENABLED
		std::shared_ptr<soci::session> session;
	#endif // ifndef SOCI_ENABLED

	L_DECLARE_PUBLIC(DbSession);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _DB_SESSION_P_H_
