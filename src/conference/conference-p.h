/*
 * conference-p.h
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

#ifndef _CONFERENCE_P_H_
#define _CONFERENCE_P_H_

#include <memory>

#include "object/object-p.h"

#include "conference/conference.h"
#include "conference/participant.h"
#include "conference/session/call-session-listener.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ConferencePrivate : public ObjectPrivate, CallSessionListener {
public:
	virtual ~ConferencePrivate () = default;

	LinphoneCore * getCore () const { return core; }

private:
	/* CallSessionListener */
	virtual void onAckBeingSent (const CallSession &session, LinphoneHeaders *headers);
	virtual void onAckReceived (const CallSession &session, LinphoneHeaders *headers);
	virtual void onCallSessionAccepted (const CallSession &session);
	virtual void onCallSessionSetReleased (const CallSession &session);
	virtual void onCallSessionSetTerminated (const CallSession &session);
	virtual void onCallSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message);
	virtual void onIncomingCallSessionStarted (const CallSession &session);
	virtual void onEncryptionChanged (const CallSession &session, bool activated, const std::string &authToken);
	virtual void onStatsUpdated (const LinphoneCallStats *stats);
	virtual void onResetCurrentSession (const CallSession &session);
	virtual void onSetCurrentSession (const CallSession &session);
	virtual void onFirstVideoFrameDecoded (const CallSession &session);
	virtual void onResetFirstVideoFrameDecoded (const CallSession &session);

private:
	LinphoneCore *core = nullptr;
	CallListener *callListener = nullptr;

	std::shared_ptr<Participant> activeParticipant = nullptr;
	std::string id;
	std::shared_ptr<Participant> me = nullptr;
	std::list<std::shared_ptr<Participant>> participants;

	L_DECLARE_PUBLIC(Conference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_P_H_
