/*
 * conference-call-event.h
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

#ifndef _L_CONFERENCE_CALL_EVENT_H_
#define _L_CONFERENCE_CALL_EVENT_H_

#include "event-log/event-log.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Call;
class ConferenceCallEventPrivate;

class LINPHONE_PUBLIC ConferenceCallEvent : public EventLog {
public:
	ConferenceCallEvent (Type type, time_t creationTime, const std::shared_ptr<Call> &call);

	std::shared_ptr<Call> getCall () const;

private:
	L_DECLARE_PRIVATE(ConferenceCallEvent);
	L_DISABLE_COPY(ConferenceCallEvent);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONFERENCE_CALL_EVENT_H_
