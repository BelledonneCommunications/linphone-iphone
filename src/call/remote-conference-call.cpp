/*
 * remote-conference-call.cpp
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

#include "conference/remote-conference-p.h"
#include "conference/participant-p.h"
#include "conference/session/media-session-p.h"
#include "remote-conference-call-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

shared_ptr<CallSession> RemoteConferenceCallPrivate::getActiveSession () const {
	L_Q();
	return q->getActiveParticipant()->getPrivate()->getSession();
}

// =============================================================================

RemoteConferenceCall::RemoteConferenceCall (
		shared_ptr<Core> core,
		LinphoneCallDir direction,
		const Address &from,
		const Address &to,
		LinphoneProxyConfig *cfg,
		SalCallOp *op,
		const MediaSessionParams *msp
	)
	: Call(*new RemoteConferenceCallPrivate, core),
	RemoteConference(core, IdentityAddress((direction == LinphoneCallIncoming) ? to : from), getPrivate()) {
	addParticipant((direction == LinphoneCallIncoming) ? from : to, msp, true);
	shared_ptr<Participant> participant = getParticipants().front();
	participant->getPrivate()->getSession()->configure(direction, cfg, op, from, to);
}

RemoteConferenceCall::~RemoteConferenceCall () {
	L_D();
	auto session = d->getActiveSession();
	if (session)
		session->getPrivate()->setCallSessionListener(nullptr);
}

shared_ptr<Core> RemoteConferenceCall::getCore () const {
	return Call::getCore();
}

LINPHONE_END_NAMESPACE
