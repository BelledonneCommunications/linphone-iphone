/*
 * participant-p.h
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

#ifndef _L_PARTICIPANT_P_H_
#define _L_PARTICIPANT_P_H_

#include "object/object-p.h"

#include "conference/participant.h"
#include "conference/session/call-session.h"
#include "conference/session/call-session-listener.h"
#include "conference/params/call-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ParticipantPrivate : public ObjectPrivate {
public:
	std::shared_ptr<Core> getCore () const { return mConference ? mConference->getCore() : nullptr; }
	Conference *getConference () const { return mConference; }
	void setConference (Conference *conference) { mConference = conference; }

	std::shared_ptr<CallSession> createSession (const Conference &conference, const CallSessionParams *params, bool hasMedia, CallSessionListener *listener);
	inline std::shared_ptr<CallSession> getSession () const { return session; }
	inline void removeSession () { session.reset(); }
	inline void setAddress (const IdentityAddress &newAddr) { addr = newAddr; }
	inline void setAdmin (bool isAdmin) { this->isAdmin = isAdmin; }

	std::shared_ptr<ParticipantDevice> addDevice (const IdentityAddress &gruu);
	void clearDevices ();
	std::shared_ptr<ParticipantDevice> findDevice (const IdentityAddress &gruu) const;
	std::shared_ptr<ParticipantDevice> findDevice (const std::shared_ptr<const CallSession> &session);
	const std::list<std::shared_ptr<ParticipantDevice>> &getDevices () const;
	void removeDevice (const IdentityAddress &gruu);

private:
	Conference *mConference = nullptr;
	IdentityAddress addr;
	bool isAdmin = false;
	std::shared_ptr<CallSession> session;
	std::list<std::shared_ptr<ParticipantDevice>> devices;

	L_DECLARE_PUBLIC(Participant);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_PARTICIPANT_P_H_
