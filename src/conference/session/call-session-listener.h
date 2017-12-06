/*
 * call-session-listener.h
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

#ifndef _CALL_SESSION_LISTENER_H_
#define _CALL_SESSION_LISTENER_H_

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;

class LINPHONE_PUBLIC CallSessionListener {
public:
	virtual ~CallSessionListener() = default;

	virtual void onAckBeingSent (const std::shared_ptr<const CallSession> &session, LinphoneHeaders *headers) = 0;
	virtual void onAckReceived (const std::shared_ptr<const CallSession> &session, LinphoneHeaders *headers) = 0;
	virtual void onCallSessionAccepted (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onCallSessionSetReleased (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onCallSessionSetTerminated (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onCallSessionStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state, const std::string &message) = 0;
	virtual void onCheckForAcceptation (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onDtmfReceived (const std::shared_ptr<const CallSession> &session, char dtmf) = 0;
	virtual void onIncomingCallSessionStarted (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onInfoReceived (const std::shared_ptr<const CallSession> &session, const LinphoneInfoMessage *im) = 0;

	virtual void onEncryptionChanged (const std::shared_ptr<const CallSession> &session, bool activated, const std::string &authToken) = 0;

	virtual void onStatsUpdated (const LinphoneCallStats *stats) = 0;

	virtual void onResetCurrentSession (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onSetCurrentSession (const std::shared_ptr<const CallSession> &session) = 0;

	virtual void onFirstVideoFrameDecoded (const std::shared_ptr<const CallSession> &session) = 0;
	virtual void onResetFirstVideoFrameDecoded (const std::shared_ptr<const CallSession> &session) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_LISTENER_H_
