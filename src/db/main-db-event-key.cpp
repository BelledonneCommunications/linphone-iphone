/*
 * main-db-event-key.cpp
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
#include "main-db-event-key.h"
#include "main-db-key-p.h"
#include "main-db-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

MainDbEventKey::MainDbEventKey () : MainDbKey() {};

MainDbEventKey::MainDbEventKey (const shared_ptr<Core> &core, long long storageId) : MainDbKey(core, storageId) {}

MainDbEventKey::~MainDbEventKey () {
	L_D();

	if (isValid())
		d->core.lock()->getPrivate()->mainDb->getPrivate()->storageIdToEvent.erase(d->storageId);
}

LINPHONE_END_NAMESPACE
