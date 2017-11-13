/*
 * participant-p.h
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#ifndef _PARTICIPANT_P_H_
#define _PARTICIPANT_P_H_

#include <memory>

#include "object/object-p.h"

#include "conference/participant.h"
#include "conference/session/call-session.h"
#include "conference/session/call-session-listener.h"
#include "conference/params/call-session-params.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ParticipantPrivate : public ObjectPrivate {
public:
	virtual ~ParticipantPrivate ();

	std::shared_ptr<CallSession> createSession (const Conference &conference, const CallSessionParams *params, bool hasMedia, CallSessionListener *listener);
	inline std::shared_ptr<CallSession> getSession () const { return session; }
	inline bool isSubscribedToConferenceEventPackage () const { return conferenceSubscribeEvent != nullptr; }
	LinphoneEvent *getConferenceSubscribeEvent () const { return conferenceSubscribeEvent; }
	void setConferenceSubscribeEvent (LinphoneEvent *ev);
	inline void removeSession () { session.reset(); }
	inline void setAddress (const SimpleAddress &newAddr) { addr = newAddr; }
	inline void setAdmin (bool isAdmin) { this->isAdmin = isAdmin; }
	inline void setContactAddress (const Address &contactAddr) { this->contactAddr = contactAddr; }
	std::shared_ptr<ParticipantDevice> findDevice (const GruuAddress &gruu) const;
	std::shared_ptr<ParticipantDevice> findDevice (const std::shared_ptr<const CallSession> &session);
	const std::list<std::shared_ptr<ParticipantDevice>> &getDevices () const;
	std::shared_ptr<ParticipantDevice> addDevice (const GruuAddress &gruu);
	void removeDevice (const GruuAddress &gruu);

private:
	SimpleAddress addr;
	Address contactAddr;
	bool isAdmin = false;
	LinphoneEvent *conferenceSubscribeEvent = nullptr;
	std::shared_ptr<CallSession> session;
	std::list<std::shared_ptr<ParticipantDevice>> devices;

	L_DECLARE_PUBLIC(Participant);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _PARTICIPANT_P_H_
