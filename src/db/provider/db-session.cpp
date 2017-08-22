/*
 * db-session.cpp
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

#include "db-session-p.h"

#include "db-session.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

L_USE_DEFAULT_SHARE_IMPL(DbSession, ClonableObject);

DbSession::operator bool () const {
	L_D(const DbSession);
	return d->isValid;
}

LINPHONE_END_NAMESPACE
