/*
 * main-db-key.h
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

#ifndef _L_MAIN_DB_KEY_H_
#define _L_MAIN_DB_KEY_H_

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Core;
class MainDbKeyPrivate;

class MainDbKey : public ClonableObject {
	friend class MainDb;
	friend class MainDbPrivate;

public:
	MainDbKey ();
	MainDbKey (const std::shared_ptr<Core> &core, long long storageId);
	MainDbKey (const MainDbKey &other);
	virtual ~MainDbKey () = 0;

	MainDbKey &operator= (const MainDbKey &other);

	bool isValid () const;

private:
	L_DECLARE_PRIVATE(MainDbKey);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_MAIN_DB_KEY_H_
