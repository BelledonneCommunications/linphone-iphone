/*
 * call.cpp
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

#include "object/object-p.h"

#include "call.h"

using namespace std;

using namespace LinphonePrivate;

// =============================================================================

class Call::CallPrivate : public ObjectPrivate {
public:
	LinphoneCallDir direction;
	LinphoneCallState state = LinphoneCallIdle;
};

// =============================================================================

Call::Call::Call (LinphoneCallDir direction) : Object(*new CallPrivate) {
	L_D(Call);
	d->direction = direction;
}

// -----------------------------------------------------------------------------

LinphoneCallDir Call::Call::getDirection () const {
	L_D(const Call);
	return d->direction;
}

// -----------------------------------------------------------------------------

LinphoneCallState Call::Call::getState () const {
	L_D(const Call);
	return d->state;
}
