/*
 * cenference-listener.h
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

#ifndef _L_CONFERENCE_LISTENER_H_
#define _L_CONFERENCE_LISTENER_H_

#include <vector>

#include "event-log/events.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class LINPHONE_PUBLIC ConferenceListener {
public:
	virtual ~ConferenceListener () = default;

	virtual void onConferenceCreated (const IdentityAddress &addr) {}
	virtual void onConferenceKeywordsChanged (const std::vector<std::string> &keywords) {}
	virtual void onConferenceTerminated (const IdentityAddress &addr) {}
	virtual void onFirstNotifyReceived (const IdentityAddress &addr) {}
	virtual void onParticipantAdded (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {}
	virtual void onParticipantRemoved (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {}
	virtual void onParticipantSetAdmin (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {}
	virtual void onSubjectChanged (const std::shared_ptr<ConferenceSubjectEvent> &event, bool isFullState) {}
	virtual void onParticipantDeviceAdded (const std::shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {}
	virtual void onParticipantDeviceRemoved (const std::shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {}
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONFERENCE_LISTENER_H_
