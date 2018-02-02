/*
 * core-accessor.cpp
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

#include "logger/logger.h"

#include "core-accessor.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class CoreAccessorPrivate {
public:
	weak_ptr<Core> core;
};

// -----------------------------------------------------------------------------

CoreAccessor::CoreAccessor (const shared_ptr<Core> &core) {
	L_ASSERT(core);
	mPrivate = new CoreAccessorPrivate();
	mPrivate->core = core;
}

CoreAccessor::CoreAccessor (const shared_ptr<Core> &&core) {
	L_ASSERT(core);
	mPrivate = new CoreAccessorPrivate();
	mPrivate->core = move(core);
}

CoreAccessor::~CoreAccessor () {
	delete mPrivate;
}

shared_ptr<Core> CoreAccessor::getCore () const {
	L_D();

	shared_ptr<Core> core = d->core.lock();
	if (!core) {
		lWarning() << "Unable to get valid core instance.";
		throw bad_weak_ptr();
	}

	return core;
}

LINPHONE_END_NAMESPACE
