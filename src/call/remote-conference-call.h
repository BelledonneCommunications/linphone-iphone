/*
 * remote-conference-call.h
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

#ifndef _L_REMOTE_CONFERENCE_CALL_H_
#define _L_REMOTE_CONFERENCE_CALL_H_

#include "call/call.h"
#include "conference/remote-conference.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Core;
class RemoteConferenceCallPrivate;

class LINPHONE_PUBLIC RemoteConferenceCall : public Call, public RemoteConference {
public:
	// TODO: Make me private.
	RemoteConferenceCall (
		std::shared_ptr<Core> core,
		LinphoneCallDir direction,
		const Address &from,
		const Address &to,
		LinphoneProxyConfig *cfg,
		SalCallOp *op,
		const MediaSessionParams *msp
	);
	~RemoteConferenceCall ();

	std::shared_ptr<Core> getCore () const;

private:
	L_DECLARE_PRIVATE(RemoteConferenceCall);
	L_DISABLE_COPY(RemoteConferenceCall);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_REMOTE_CONFERENCE_CALL_H_
