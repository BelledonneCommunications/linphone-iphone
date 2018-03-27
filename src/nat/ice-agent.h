/*
 * ice-agent.h
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

#ifndef _L_ICE_AGENT_H_
#define _L_ICE_AGENT_H_

#include <mediastreamer2/ice.h>
#include <ortp/event.h>

#include "linphone/utils/general.h"

// =============================================================================

L_DECL_C_STRUCT_PREFIX_LESS(SalMediaDescription);
L_DECL_C_STRUCT_PREFIX_LESS(SalStreamDescription);
L_DECL_C_STRUCT(LinphoneCallStats);
L_DECL_C_STRUCT(LinphoneCore);
L_DECL_C_STRUCT(MediaStream);

class MediaSession;

LINPHONE_BEGIN_NAMESPACE

class IceAgent {
public:
	explicit IceAgent (MediaSession &mediaSession) : mediaSession(mediaSession) {}

	bool candidatesGathered () const;
	void checkSession (IceRole role, bool isReinvite);
	void deleteSession ();
	void gatheringFinished ();
	int getNbLosingPairs () const;
	IceSession *getIceSession () const {
		return iceSession;
	}

	bool hasCompleted () const;
	bool hasCompletedCheckList () const;
	bool hasSession () const {
		return !!iceSession;
	}

	bool isControlling () const;
	bool prepare (const SalMediaDescription *localDesc, bool incomingOffer);
	void prepareIceForStream (MediaStream *ms, bool createChecklist);
	void resetSession (IceRole role);
	void restartSession (IceRole role);
	void startConnectivityChecks ();
	void stopIceForInactiveStreams (SalMediaDescription *desc);
	void updateFromRemoteMediaDescription (const SalMediaDescription *localDesc, const SalMediaDescription *remoteDesc, bool isOffer);
	void updateIceStateInCallStats ();
	void updateLocalMediaDescriptionFromIce (SalMediaDescription *desc);
	/*
	 * Checks if an incoming offer with ICE needs a delayed answer, because the ice session hasn't completed yet with
	 * connecvity checks.
	 */
	bool checkIceReinviteNeedsDeferedResponse(SalMediaDescription *md);

private:
	void addLocalIceCandidates (int family, const char *addr, IceCheckList *audioCl, IceCheckList *videoCl, IceCheckList *textCl);
	bool checkForIceRestartAndSetRemoteCredentials (const SalMediaDescription *md, bool isOffer);
	void clearUnusedIceCandidates (const SalMediaDescription *localDesc, const SalMediaDescription *remoteDesc);
	void createIceCheckListsAndParseIceAttributes (const SalMediaDescription *md, bool iceRestarted);
	int gatherIceCandidates ();
	void getIceDefaultAddrAndPort (uint16_t componentID, const SalMediaDescription *md, const SalStreamDescription *stream, const char **addr, int *port);
	const struct addrinfo *getIcePreferredStunServerAddrinfo (const struct addrinfo *ai);
	bool iceParamsFoundInRemoteMediaDescription (const SalMediaDescription *md);
	void updateIceStateInCallStatsForStream (LinphoneCallStats *stats, IceCheckList *cl);

private:
	MediaSession &mediaSession;
	IceSession *iceSession = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_ICE_AGENT_H_
