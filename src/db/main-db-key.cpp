/*
 * main-db-key.cpp
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

#include "core/core-p.h"
#include "main-db-key-p.h"
#include "main-db-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

MainDbKey::MainDbKey () : ClonableObject(*new MainDbKeyPrivate) {}

MainDbKey::MainDbKey (const shared_ptr<Core> &core, long long storageId) : MainDbKey() {
	L_D();
	d->core = core;
	d->storageId = storageId;
}

MainDbKey::MainDbKey (const MainDbKey &other) : MainDbKey() {
	L_D();
	const MainDbKeyPrivate *dOther = other.getPrivate();

	d->core = dOther->core;
	d->storageId = dOther->storageId;
}

MainDbKey::~MainDbKey () {}

MainDbKey &MainDbKey::operator= (const MainDbKey &other) {
	L_D();

	if (this != &other) {
		const MainDbKeyPrivate *dOther = other.getPrivate();
		d->core = dOther->core;
		d->storageId = dOther->storageId;
	}

	return *this;
}

bool MainDbKey::isValid () const {
	L_D();
	return !d->core.expired() && d->storageId >= 0;
}

LINPHONE_END_NAMESPACE
