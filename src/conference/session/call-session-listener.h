/*
 * call-session-listener.h
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

#ifndef _CALL_SESSION_LISTENER_H_
#define _CALL_SESSION_LISTENER_H_

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionListener {
public:
	virtual void ackBeingSent (const CallSession &session, LinphoneHeaders *headers) = 0;
	virtual void ackReceived (const CallSession &session, LinphoneHeaders *headers) = 0;
	virtual void callSessionAccepted (const CallSession &session) = 0;
	virtual void callSessionSetReleased (const CallSession &session) = 0;
	virtual void callSessionSetTerminated (const CallSession &session) = 0;
	virtual void callSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message) = 0;
	virtual void incomingCallSessionStarted (const CallSession &session) = 0;

	virtual void encryptionChanged (const CallSession &session, bool activated, const std::string &authToken) = 0;

	virtual void statsUpdated (const LinphoneCallStats *stats) = 0;

	virtual void resetCurrentSession (const CallSession &session) = 0;
	virtual void setCurrentSession (const CallSession &session) = 0;

	virtual void firstVideoFrameDecoded (const CallSession &session) = 0;
	virtual void resetFirstVideoFrameDecoded (const CallSession &session) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_LISTENER_H_
