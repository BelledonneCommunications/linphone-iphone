/*
 * call-event.cpp
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

#include "event-log-p.h"

#include "call-event.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class CallEventPrivate : public EventLogPrivate {
public:
	shared_ptr<Call> call;
};

// -----------------------------------------------------------------------------

CallEvent::CallEvent (Type type, const shared_ptr<Call> &call) : EventLog(*new CallEventPrivate, type) {
	L_D(CallEvent);
	L_ASSERT(call);
	L_ASSERT(type == TypeCallStart || type == TypeCallEnd);
	d->call = call;
}

CallEvent::CallEvent (const CallEvent &src) : CallEvent(src.getType(), src.getCall()) {}

CallEvent &CallEvent::operator= (const CallEvent &src) {
	L_D(CallEvent);
	if (this != &src) {
		EventLog::operator=(src);
		d->call = src.getPrivate()->call;
	}

	return *this;
}

shared_ptr<Call> CallEvent::getCall () const {
	L_D(const CallEvent);
	return d->call;
}

LINPHONE_END_NAMESPACE
