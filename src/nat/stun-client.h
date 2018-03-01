/*
 * stun-client.h
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

#ifndef _L_STUN_CLIENT_H_
#define _L_STUN_CLIENT_H_

#include <string>

#include <ortp/port.h>

#include "core/core.h"
#include "core/core-accessor.h"

#include "linphone/utils/general.h"

// =============================================================================

L_DECL_C_STRUCT_PREFIX_LESS(SalMediaDescription);

LINPHONE_BEGIN_NAMESPACE

class StunClient : public CoreAccessor {
	struct Candidate {
		std::string address;
		int port = 0;
	};

public:
	StunClient (const std::shared_ptr<Core> &core) : CoreAccessor(core) {}

	int run (int audioPort, int videoPort, int textPort);
	void updateMediaDescription (SalMediaDescription *md) const;

	const Candidate &getAudioCandidate () const {
		return audioCandidate;
	}

	const Candidate &getVideoCandidate () const {
		return videoCandidate;
	}

	const Candidate &getTextCandidate () const {
		return textCandidate;
	}

	ortp_socket_t createStunSocket (int localPort);
	int recvStunResponse (ortp_socket_t sock, Candidate &candidate, int &id);
	int sendStunRequest (ortp_socket_t sock, const struct sockaddr *server, socklen_t addrlen, int id, bool changeAddr);

private:
	Candidate audioCandidate;
	Candidate videoCandidate;
	Candidate textCandidate;
	bool stunDiscoveryDone = false;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_STUN_CLIENT_H_
