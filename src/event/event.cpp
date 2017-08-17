/*
 * event.cpp
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

#include "event-p.h"

#include "event.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

Event::Event () : ClonableObject(*new EventPrivate) {}

Event::Event (const Event &) : ClonableObject(*new EventPrivate) {
	// `src` parameter is useless.
}

Event::Event (EventPrivate &p) : ClonableObject(p) {}

Event::Type Event::getType () const {
	L_D(const Event);
	return d->type;
}

LINPHONE_END_NAMESPACE
