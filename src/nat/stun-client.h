/*
 * stun-client.h
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

#ifndef _STUN_CLIENT_H_
#define _STUN_CLIENT_H_

#include <string>

#include <ortp/port.h>

#include "linphone/utils/general.h"

// =============================================================================

L_DECL_C_STRUCT_PREFIX_LESS(SalMediaDescription);
L_DECL_C_STRUCT(LinphoneCore);

LINPHONE_BEGIN_NAMESPACE

class StunClient {
	struct Candidate {
		std::string address;
		int port;
	};

public:
	StunClient (LinphoneCore *core) : core(core) {}

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
	LinphoneCore *core = nullptr;
	Candidate audioCandidate;
	Candidate videoCandidate;
	Candidate textCandidate;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _STUN_CLIENT_H_
