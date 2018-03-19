/*
 * conference-interface.h
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

#ifndef _L_CONFERENCE_INTERFACE_H_
#define _L_CONFERENCE_INTERFACE_H_

#include <list>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class IdentityAddress;
class CallSessionParams;
class Participant;

class LINPHONE_PUBLIC ConferenceInterface {
public:
	virtual ~ConferenceInterface () = default;

	virtual void addParticipant (
		const IdentityAddress &participantAddress,
		const CallSessionParams *params,
		bool hasMedia
	) = 0;
	virtual void addParticipants (
		const std::list<IdentityAddress> &addresses,
		const CallSessionParams *params,
		bool hasMedia
	) = 0;
	virtual bool canHandleParticipants () const = 0;
	virtual std::shared_ptr<Participant> findParticipant (const IdentityAddress &participantAddress) const = 0;
	virtual const IdentityAddress &getConferenceAddress () const = 0;
	virtual std::shared_ptr<Participant> getMe () const = 0;
	virtual int getParticipantCount () const = 0;
	virtual const std::list<std::shared_ptr<Participant>> &getParticipants () const = 0;
	virtual const std::string &getSubject () const = 0;
	virtual void join () = 0;
	virtual void leave () = 0;
	virtual void removeParticipant (const std::shared_ptr<const Participant> &participant) = 0;
	virtual void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) = 0;
	virtual void setParticipantAdminStatus (const std::shared_ptr<Participant> &participant, bool isAdmin) = 0;
	virtual void setSubject (const std::string &subject) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONFERENCE_INTERFACE_H_
