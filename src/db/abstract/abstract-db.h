/*
 * abstract-db.h
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

#ifndef _ABSTRACT_DB_H_
#define _ABSTRACT_DB_H_

#include <string>

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class AbstractDbPrivate;

class LINPHONE_PUBLIC AbstractDb : public Object {
public:
	enum Backend {
		Mysql,
		Sqlite3
	};

	virtual ~AbstractDb () = default;

	bool connect (Backend backend, const std::string &parameters);
	bool disconnect ();

	bool isConnected () const;

	Backend getBackend () const;

protected:
	explicit AbstractDb (AbstractDbPrivate &p);

	virtual void init ();

	std::string primaryKeyAutoIncrementStr (const std::string &type = "INT") const;

private:
	L_DECLARE_PRIVATE(AbstractDb);
	L_DISABLE_COPY(AbstractDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _ABSTRACT_DB_H_
