/*
 * call-listener.h
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

#ifndef _CALL_LISTENER_H_
#define _CALL_LISTENER_H_

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallListener {
public:
	virtual ~CallListener () = default;

	virtual void onAckBeingSent (LinphoneHeaders *headers) = 0;
	virtual void onAckReceived (LinphoneHeaders *headers) = 0;
	virtual void onCallSetReleased () = 0;
	virtual void onCallSetTerminated () = 0;
	virtual void onCallStateChanged (LinphoneCallState state, const std::string &message) = 0;
	virtual void onCheckForAcceptation () = 0;
	virtual void onIncomingCallStarted () = 0;
	virtual void onIncomingCallToBeAdded () = 0;
	virtual void onInfoReceived (const LinphoneInfoMessage *im) = 0;

	virtual void onEncryptionChanged (bool activated, const std::string &authToken) = 0;

	virtual void onStatsUpdated (const LinphoneCallStats *stats) = 0;

	virtual void onResetCurrentCall () = 0;
	virtual void onSetCurrentCall () = 0;

	virtual void onFirstVideoFrameDecoded () = 0;
	virtual void onResetFirstVideoFrameDecoded () = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_LISTENER_H_
