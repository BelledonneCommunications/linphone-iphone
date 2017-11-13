/*
 * participant-device.h
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

#ifndef _PARTICIPANT_DEVICE_H_
#define _PARTICIPANT_DEVICE_H_

#include <memory>
#include <string>

#include "address/gruu-address.h"
#include "linphone/types.h"
#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;

class ParticipantDevice {
public:
	ParticipantDevice ();
	explicit ParticipantDevice (const GruuAddress &gruu);
	virtual ~ParticipantDevice ();

	bool operator== (const ParticipantDevice &device) const;

	inline const GruuAddress &getGruu () const { return mGruu; }
	inline std::shared_ptr<CallSession> getSession () const { return mSession; }
	inline void setSession (std::shared_ptr<CallSession> session) { mSession = session; }

	inline bool isSubscribedToConferenceEventPackage () const { return mConferenceSubscribeEvent != nullptr; }
	LinphoneEvent *getConferenceSubscribeEvent () const { return mConferenceSubscribeEvent; }
	void setConferenceSubscribeEvent (LinphoneEvent *ev);

	bool isValid () const { return mGruu.isValid(); }

private:
	GruuAddress mGruu;
	std::shared_ptr<CallSession> mSession;
	LinphoneEvent *mConferenceSubscribeEvent = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _PARTICIPANT_DEVICE_H_