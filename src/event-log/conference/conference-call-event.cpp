/*
 * conference-call-event.cpp
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

#include "conference-call-event.h"
#include "event-log/event-log-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class ConferenceCallEventPrivate : public EventLogPrivate {
public:
	shared_ptr<Call> call;
};

// -----------------------------------------------------------------------------

ConferenceCallEvent::ConferenceCallEvent (Type type, time_t creationTime, const shared_ptr<Call> &call) :
	EventLog(*new ConferenceCallEventPrivate, type, creationTime) {
	L_D();
	L_ASSERT(call);
	L_ASSERT(type == Type::ConferenceCallStart || type == Type::ConferenceCallEnd);
	d->call = call;
}

shared_ptr<Call> ConferenceCallEvent::getCall () const {
	L_D();
	return d->call;
}

LINPHONE_END_NAMESPACE
