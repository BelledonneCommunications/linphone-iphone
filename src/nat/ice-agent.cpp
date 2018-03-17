/*
 * ice-agent.cpp
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

#include "linphone/core.h"

#include "private.h"

#include "conference/session/media-session-p.h"
#include "core/core.h"
#include "logger/logger.h"

#include "ice-agent.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

bool IceAgent::candidatesGathered () const {
	if (!iceSession)
		return false;
	return !!ice_session_candidates_gathered(iceSession);
}

void IceAgent::checkSession (IceRole role, bool isReinvite) {
	// Already created.
	if (iceSession)
		return;

	LinphoneConfig *config = linphone_core_get_config(mediaSession.getCore()->getCCore());
	
	if (lp_config_get_int(config, "net", "force_ice_disablement", 0)){
		lWarning()<<"ICE is disabled in this version";
		return;
	}
	
	if (isReinvite && (lp_config_get_int(config, "net", "allow_late_ice", 0) == 0))
		return;

	iceSession = ice_session_new();

	// For backward compatibility purposes, shall be enabled by default in the future.
	ice_session_enable_message_integrity_check(
		iceSession,
		!!lp_config_get_int(config, "net", "ice_session_enable_message_integrity_check", 1)
	);
	if (lp_config_get_int(config, "net", "dont_default_to_stun_candidates", 0)) {
		IceCandidateType types[ICT_CandidateTypeMax];
		types[0] = ICT_HostCandidate;
		types[1] = ICT_RelayedCandidate;
		types[2] = ICT_CandidateInvalid;
		ice_session_set_default_candidates_types(iceSession, types);
	}
	ice_session_set_role(iceSession, role);
}

void IceAgent::deleteSession () {
	if (!iceSession)
		return;

	ice_session_destroy(iceSession);
	iceSession = nullptr;
	mediaSession.getPrivate()->deactivateIce();
}

void IceAgent::gatheringFinished () {
	const SalMediaDescription *rmd = mediaSession.getPrivate()->getOp()->get_remote_media_description();
	if (rmd)
		clearUnusedIceCandidates(mediaSession.getPrivate()->getLocalDesc(), rmd);
	if (!iceSession)
		return;

	ice_session_compute_candidates_foundations(iceSession);
	ice_session_eliminate_redundant_candidates(iceSession);
	ice_session_choose_default_candidates(iceSession);

	int pingTime = ice_session_average_gathering_round_trip_time(iceSession);
	if (pingTime >= 0) {
		mediaSession.getPrivate()->setPingTime(pingTime);
	}
}

int IceAgent::getNbLosingPairs () const {
	if (!iceSession)
		return 0;
	return ice_session_nb_losing_pairs(iceSession);
}

bool IceAgent::hasCompleted () const {
	if (!iceSession)
		return true;
	return ice_session_state(iceSession) == IS_Completed;
}

bool IceAgent::hasCompletedCheckList () const {
	if (!iceSession)
		return false;
	switch (ice_session_state(iceSession)) {
		case IS_Completed:
		case IS_Failed:
			return !!ice_session_has_completed_check_list(iceSession);
		default:
			return false;
	}
}

bool IceAgent::isControlling () const {
	if (!iceSession)
		return false;
	return ice_session_role(iceSession) == IR_Controlling;
}

bool IceAgent::prepare (const SalMediaDescription *localDesc, bool incomingOffer) {
	if (!iceSession)
		return false;

	SalMediaDescription *remoteDesc = nullptr;
	bool hasVideo = false;
	if (incomingOffer) {
		remoteDesc = mediaSession.getPrivate()->getOp()->get_remote_media_description();
		hasVideo = linphone_core_video_enabled(mediaSession.getCore()->getCCore()) &&
			linphone_core_media_description_contains_video_stream(remoteDesc);
	} else
		hasVideo = mediaSession.getMediaParams()->videoEnabled();

	prepareIceForStream(mediaSession.getPrivate()->getMediaStream(LinphoneStreamTypeAudio), true);
	if (hasVideo)
		prepareIceForStream(mediaSession.getPrivate()->getMediaStream(LinphoneStreamTypeVideo), true);
	if (mediaSession.getMediaParams()->realtimeTextEnabled())
		prepareIceForStream(mediaSession.getPrivate()->getMediaStream(LinphoneStreamTypeText), true);

	// Start ICE gathering.
	if (incomingOffer)
		// This may delete the ice session.
		updateFromRemoteMediaDescription(localDesc, remoteDesc, true);
	if (iceSession && !ice_session_candidates_gathered(iceSession)) {
		mediaSession.getPrivate()->prepareStreamsForIceGathering(hasVideo);
		int err = gatherIceCandidates();
		if (err == 0) {
			// Ice candidates gathering wasn't started, but we can proceed with the call anyway.
			mediaSession.getPrivate()->stopStreamsForIceGathering();
			return false;
		} else if (err == -1) {
			mediaSession.getPrivate()->stopStreamsForIceGathering();
			deleteSession();
			return false;
		}
		return true;
	}
	return false;
}

void IceAgent::prepareIceForStream (MediaStream *ms, bool createChecklist) {
	if (!iceSession)
		return;

	int streamIndex = mediaSession.getPrivate()->getStreamIndex(ms);
	rtp_session_set_pktinfo(ms->sessions.rtp_session, true);
	IceCheckList *cl = ice_session_check_list(iceSession, streamIndex);
	if (!cl && createChecklist) {
		cl = ice_check_list_new();
		ice_session_add_check_list(iceSession, cl, static_cast<unsigned int>(streamIndex));
		lInfo() << "Created new ICE check list for stream [" << streamIndex << "]";
	}
	if (cl)
		media_stream_set_ice_check_list(ms, cl);
}

void IceAgent::resetSession (IceRole role) {
	if (!iceSession)
		return;
	ice_session_reset(iceSession, role);
}

void IceAgent::restartSession (IceRole role) {
	if (!iceSession)
		return;
	ice_session_restart(iceSession, role);
}

void IceAgent::startConnectivityChecks () {
	if (!iceSession)
		return;
	ice_session_start_connectivity_checks(iceSession);
}

void IceAgent::stopIceForInactiveStreams (SalMediaDescription *desc) {
	if (!iceSession)
		return;
	if (ice_session_state(iceSession) == IS_Completed)
		return;
	for (int i = 0; i < desc->nb_streams; i++) {
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		if (!sal_stream_description_active(&desc->streams[i]) && cl) {
			ice_session_remove_check_list(iceSession, cl);
			mediaSession.getPrivate()->clearIceCheckList(cl);
		}
	}
	updateIceStateInCallStats();
}

void IceAgent::updateFromRemoteMediaDescription (
	const SalMediaDescription *localDesc,
	const SalMediaDescription *remoteDesc,
	bool isOffer
) {
	if (!iceSession)
		return;

	if (!iceParamsFoundInRemoteMediaDescription(remoteDesc)) {
		// Response from remote does not contain mandatory ICE attributes, delete the session.
		deleteSession();
		mediaSession.getPrivate()->enableSymmetricRtp(!!linphone_core_symmetric_rtp_enabled(mediaSession.getCore()->getCCore()));
		return;
	}

	// Check for ICE restart and set remote credentials.
	bool iceRestarted = checkForIceRestartAndSetRemoteCredentials(remoteDesc, isOffer);

	// Create ICE check lists if needed and parse ICE attributes.
	createIceCheckListsAndParseIceAttributes(remoteDesc, iceRestarted);
	for (int i = 0; i < remoteDesc->nb_streams; i++) {
		const SalStreamDescription *stream = &remoteDesc->streams[i];
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		if (!cl) continue;
		if (!sal_stream_description_active(stream)) {
			ice_session_remove_check_list_from_idx(iceSession, static_cast<unsigned int>(i));
			mediaSession.getPrivate()->clearIceCheckList(cl);
		}
	}
	clearUnusedIceCandidates(localDesc, remoteDesc);
	ice_session_check_mismatch(iceSession);

	if (ice_session_nb_check_lists(iceSession) == 0) {
		deleteSession();
		mediaSession.getPrivate()->enableSymmetricRtp(!!linphone_core_symmetric_rtp_enabled(mediaSession.getCore()->getCCore()));
	}
}

void IceAgent::updateIceStateInCallStats () {
	if (!iceSession)
		return;
	IceCheckList *audioCheckList = ice_session_check_list(iceSession, mediaSession.getPrivate()->getStreamIndex(LinphoneStreamTypeAudio));
	IceCheckList *videoCheckList = ice_session_check_list(iceSession, mediaSession.getPrivate()->getStreamIndex(LinphoneStreamTypeVideo));
	IceCheckList *textCheckList = ice_session_check_list(iceSession, mediaSession.getPrivate()->getStreamIndex(LinphoneStreamTypeText));
	if (!audioCheckList && !videoCheckList && !textCheckList)
		return;

	LinphoneCallStats *audioStats = mediaSession.getPrivate()->getStats(LinphoneStreamTypeAudio);
	LinphoneCallStats *videoStats = mediaSession.getPrivate()->getStats(LinphoneStreamTypeVideo);
	LinphoneCallStats *textStats = mediaSession.getPrivate()->getStats(LinphoneStreamTypeText);
	IceSessionState sessionState = ice_session_state(iceSession);
	if ((sessionState == IS_Completed) || ((sessionState == IS_Failed) && ice_session_has_completed_check_list(iceSession))) {
		_linphone_call_stats_set_ice_state(audioStats, LinphoneIceStateNotActivated);
		if (audioCheckList && mediaSession.getMediaParams()->audioEnabled())
			updateIceStateInCallStatsForStream(audioStats, audioCheckList);

		_linphone_call_stats_set_ice_state(videoStats, LinphoneIceStateNotActivated);
		if (videoCheckList && mediaSession.getMediaParams()->videoEnabled())
			updateIceStateInCallStatsForStream(videoStats, videoCheckList);

		_linphone_call_stats_set_ice_state(textStats, LinphoneIceStateNotActivated);
		if (textCheckList && mediaSession.getMediaParams()->realtimeTextEnabled())
			updateIceStateInCallStatsForStream(textStats, textCheckList);
	} else if (sessionState == IS_Running) {
		if (audioCheckList && mediaSession.getMediaParams()->audioEnabled())
			_linphone_call_stats_set_ice_state(audioStats, LinphoneIceStateInProgress);
		if (videoCheckList && mediaSession.getMediaParams()->videoEnabled())
			_linphone_call_stats_set_ice_state(videoStats, LinphoneIceStateInProgress);
		if (textCheckList && mediaSession.getMediaParams()->realtimeTextEnabled())
			_linphone_call_stats_set_ice_state(textStats, LinphoneIceStateInProgress);
	} else {
		if (audioCheckList && mediaSession.getMediaParams()->audioEnabled())
			_linphone_call_stats_set_ice_state(audioStats, LinphoneIceStateFailed);
		if (videoCheckList && mediaSession.getMediaParams()->videoEnabled())
			_linphone_call_stats_set_ice_state(videoStats, LinphoneIceStateFailed);
		if (textCheckList && mediaSession.getMediaParams()->realtimeTextEnabled())
			_linphone_call_stats_set_ice_state(textStats, LinphoneIceStateFailed);
	}
	lInfo() << "CallSession [" << &mediaSession << "] New ICE state: audio: [" << linphone_ice_state_to_string(linphone_call_stats_get_ice_state(audioStats)) <<
		"]    video: [" << linphone_ice_state_to_string(linphone_call_stats_get_ice_state(videoStats)) <<
		"]    text: [" << linphone_ice_state_to_string(linphone_call_stats_get_ice_state(textStats)) << "]";
}

void IceAgent::updateLocalMediaDescriptionFromIce (SalMediaDescription *desc) {
	if (!iceSession)
		return;
	IceCandidate *rtpCandidate = nullptr;
	IceCandidate *rtcpCandidate = nullptr;
	bool result = false;
	IceSessionState sessionState = ice_session_state(iceSession);
	if (sessionState == IS_Completed) {
		IceCheckList *firstCl = nullptr;
		for (int i = 0; i < desc->nb_streams; i++) {
			IceCheckList *cl = ice_session_check_list(iceSession, i);
			if (cl) {
				firstCl = cl;
				break;
			}
		}
		if (firstCl)
			result = !!ice_check_list_selected_valid_local_candidate(firstCl, &rtpCandidate, nullptr);
		if (result)
			strncpy(desc->addr, rtpCandidate->taddr.ip, sizeof(desc->addr));
		else
			lWarning() << "If ICE has completed successfully, rtp_candidate should be set!";
	}

	strncpy(desc->ice_pwd, ice_session_local_pwd(iceSession), sizeof(desc->ice_pwd));
	strncpy(desc->ice_ufrag, ice_session_local_ufrag(iceSession), sizeof(desc->ice_ufrag));
	for (int i = 0; i < desc->nb_streams; i++) {
		SalStreamDescription *stream = &desc->streams[i];
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		rtpCandidate = rtcpCandidate = nullptr;
		if (!sal_stream_description_active(stream) || !cl)
			continue;
		if (ice_check_list_state(cl) == ICL_Completed) {
			LinphoneConfig *config = linphone_core_get_config(mediaSession.getCore()->getCCore());
			// TODO: Remove `ice_uses_nortpproxy` option, let's say in December 2018.
			bool useNoRtpProxy = !!lp_config_get_int(config, "sip", "ice_uses_nortpproxy", false);
			if (useNoRtpProxy)
				stream->set_nortpproxy = true;
			result = !!ice_check_list_selected_valid_local_candidate(ice_session_check_list(iceSession, i), &rtpCandidate, &rtcpCandidate);
		} else {
			stream->set_nortpproxy = false;
			result = !!ice_check_list_default_local_candidate(ice_session_check_list(iceSession, i), &rtpCandidate, &rtcpCandidate);
		}
		if (result) {
			strncpy(stream->rtp_addr, rtpCandidate->taddr.ip, sizeof(stream->rtp_addr));
			strncpy(stream->rtcp_addr, rtcpCandidate->taddr.ip, sizeof(stream->rtcp_addr));
			stream->rtp_port = rtpCandidate->taddr.port;
			stream->rtcp_port = rtcpCandidate->taddr.port;
		} else {
			memset(stream->rtp_addr, 0, sizeof(stream->rtp_addr));
			memset(stream->rtcp_addr, 0, sizeof(stream->rtcp_addr));
		}
		if ((strlen(ice_check_list_local_pwd(cl)) != strlen(desc->ice_pwd)) || (strcmp(ice_check_list_local_pwd(cl), desc->ice_pwd)))
			strncpy(stream->ice_pwd, ice_check_list_local_pwd(cl), sizeof(stream->ice_pwd));
		else
			memset(stream->ice_pwd, 0, sizeof(stream->ice_pwd));
		if ((strlen(ice_check_list_local_ufrag(cl)) != strlen(desc->ice_ufrag)) || (strcmp(ice_check_list_local_ufrag(cl), desc->ice_ufrag)))
			strncpy(stream->ice_ufrag, ice_check_list_local_ufrag(cl), sizeof(stream->ice_ufrag));
		else
			memset(stream->ice_pwd, 0, sizeof(stream->ice_pwd));
		stream->ice_mismatch = ice_check_list_is_mismatch(cl);
		if ((ice_check_list_state(cl) == ICL_Running) || (ice_check_list_state(cl) == ICL_Completed)) {
			memset(stream->ice_candidates, 0, sizeof(stream->ice_candidates));
			int nbCandidates = 0;
			for (int j = 0; j < MIN((int)bctbx_list_size(cl->local_candidates), SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES); j++) {
				SalIceCandidate *salCandidate = &stream->ice_candidates[nbCandidates];
				IceCandidate *iceCandidate = reinterpret_cast<IceCandidate *>(bctbx_list_nth_data(cl->local_candidates, j));
				const char *defaultAddr = nullptr;
				int defaultPort = 0;
				if (iceCandidate->componentID == 1) {
					defaultAddr = stream->rtp_addr;
					defaultPort = stream->rtp_port;
				} else if (iceCandidate->componentID == 2) {
					defaultAddr = stream->rtcp_addr;
					defaultPort = stream->rtcp_port;
				} else
					continue;
				if (defaultAddr[0] == '\0')
					defaultAddr = desc->addr;
				// Only include the candidates matching the default destination for each component of the stream if the state is Completed as specified in RFC5245 section 9.1.2.2.
				if (
					ice_check_list_state(cl) == ICL_Completed &&
					!((iceCandidate->taddr.port == defaultPort) && (strlen(iceCandidate->taddr.ip) == strlen(defaultAddr)) && (strcmp(iceCandidate->taddr.ip, defaultAddr) == 0))
				)
					continue;
				strncpy(salCandidate->foundation, iceCandidate->foundation, sizeof(salCandidate->foundation));
				salCandidate->componentID = iceCandidate->componentID;
				salCandidate->priority = iceCandidate->priority;
				strncpy(salCandidate->type, ice_candidate_type(iceCandidate), sizeof(salCandidate->type));
				strncpy(salCandidate->addr, iceCandidate->taddr.ip, sizeof(salCandidate->addr));
				salCandidate->port = iceCandidate->taddr.port;
				if (iceCandidate->base && (iceCandidate->base != iceCandidate)) {
					strncpy(salCandidate->raddr, iceCandidate->base->taddr.ip, sizeof(salCandidate->raddr));
					salCandidate->rport = iceCandidate->base->taddr.port;
				}
				nbCandidates++;
			}
		}
		if ((ice_check_list_state(cl) == ICL_Completed) && (ice_session_role(iceSession) == IR_Controlling)) {
			memset(stream->ice_remote_candidates, 0, sizeof(stream->ice_remote_candidates));
			if (ice_check_list_selected_valid_remote_candidate(cl, &rtpCandidate, &rtcpCandidate)) {
				strncpy(stream->ice_remote_candidates[0].addr, rtpCandidate->taddr.ip, sizeof(stream->ice_remote_candidates[0].addr));
				stream->ice_remote_candidates[0].port = rtpCandidate->taddr.port;
				strncpy(stream->ice_remote_candidates[1].addr, rtcpCandidate->taddr.ip, sizeof(stream->ice_remote_candidates[1].addr));
				stream->ice_remote_candidates[1].port = rtcpCandidate->taddr.port;
			} else
				lError() << "ice: Selected valid remote candidates should be present if the check list is in the Completed state";
		} else {
			for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES; j++) {
				stream->ice_remote_candidates[j].addr[0] = '\0';
				stream->ice_remote_candidates[j].port = 0;
			}
		}
	}
}

// -----------------------------------------------------------------------------

void IceAgent::addLocalIceCandidates (int family, const char *addr, IceCheckList *audioCl, IceCheckList *videoCl, IceCheckList *textCl) {
	if ((ice_check_list_state(audioCl) != ICL_Completed) && !ice_check_list_candidates_gathered(audioCl)) {
		int rtpPort = mediaSession.getPrivate()->getRtpPort(LinphoneStreamTypeAudio);
		int rtcpPort = mediaSession.getPrivate()->getRtcpPort(LinphoneStreamTypeAudio);
		ice_add_local_candidate(audioCl, "host", family, addr, rtpPort, 1, nullptr);
		ice_add_local_candidate(audioCl, "host", family, addr, rtcpPort, 2, nullptr);
		LinphoneCallStats *audioStats = mediaSession.getPrivate()->getStats(LinphoneStreamTypeAudio);
		_linphone_call_stats_set_ice_state(audioStats, LinphoneIceStateInProgress);
	}
	LinphoneCore *core = mediaSession.getCore()->getCCore();
	if (linphone_core_video_enabled(core) && videoCl && (ice_check_list_state(videoCl) != ICL_Completed) && !ice_check_list_candidates_gathered(videoCl)) {
		int rtpPort = mediaSession.getPrivate()->getRtpPort(LinphoneStreamTypeVideo);
		int rtcpPort = mediaSession.getPrivate()->getRtcpPort(LinphoneStreamTypeVideo);
		ice_add_local_candidate(videoCl, "host", family, addr, rtpPort, 1, nullptr);
		ice_add_local_candidate(videoCl, "host", family, addr, rtcpPort, 2, nullptr);
		LinphoneCallStats *videoStats = mediaSession.getPrivate()->getStats(LinphoneStreamTypeVideo);
		_linphone_call_stats_set_ice_state(videoStats, LinphoneIceStateInProgress);
	}
	if (mediaSession.getMediaParams()->realtimeTextEnabled() && textCl && (ice_check_list_state(textCl) != ICL_Completed) && !ice_check_list_candidates_gathered(textCl)) {
		int rtpPort = mediaSession.getPrivate()->getRtpPort(LinphoneStreamTypeText);
		int rtcpPort = mediaSession.getPrivate()->getRtcpPort(LinphoneStreamTypeText);
		ice_add_local_candidate(textCl, "host", family, addr, rtpPort, 1, nullptr);
		ice_add_local_candidate(textCl, "host", family, addr, rtcpPort, 2, nullptr);
		LinphoneCallStats *textStats = mediaSession.getPrivate()->getStats(LinphoneStreamTypeText);
		_linphone_call_stats_set_ice_state(textStats, LinphoneIceStateInProgress);
	}
}

bool IceAgent::checkForIceRestartAndSetRemoteCredentials (const SalMediaDescription *md, bool isOffer) {
	bool iceRestarted = false;
	string addr = md->addr;
	if ((addr == "0.0.0.0") || (addr == "::0")) {
		ice_session_restart(iceSession, isOffer ? IR_Controlled : IR_Controlling);
		iceRestarted = true;
	} else {
		for (int i = 0; i < md->nb_streams; i++) {
			const SalStreamDescription *stream = &md->streams[i];
			IceCheckList *cl = ice_session_check_list(iceSession, i);
			string rtpAddr = stream->rtp_addr;
			if (cl && (rtpAddr == "0.0.0.0")) {
				ice_session_restart(iceSession, isOffer ? IR_Controlled : IR_Controlling);
				iceRestarted = true;
				break;
			}
		}
	}
	if (!ice_session_remote_ufrag(iceSession) && !ice_session_remote_pwd(iceSession)) {
		ice_session_set_remote_credentials(iceSession, md->ice_ufrag, md->ice_pwd);
	} else if (ice_session_remote_credentials_changed(iceSession, md->ice_ufrag, md->ice_pwd)) {
		if (!iceRestarted) {
			ice_session_restart(iceSession, isOffer ? IR_Controlled : IR_Controlling);
			iceRestarted = true;
		}
		ice_session_set_remote_credentials(iceSession, md->ice_ufrag, md->ice_pwd);
	}
	for (int i = 0; i < md->nb_streams; i++) {
		const SalStreamDescription *stream = &md->streams[i];
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		if (cl && (stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0')) {
			if (ice_check_list_remote_credentials_changed(cl, stream->ice_ufrag, stream->ice_pwd)) {
				if (!iceRestarted && ice_check_list_get_remote_ufrag(cl) && ice_check_list_get_remote_pwd(cl)) {
					// Restart only if remote ufrag/paswd was already set.
					ice_session_restart(iceSession, isOffer ? IR_Controlled : IR_Controlling);
					iceRestarted = true;
				}
				ice_check_list_set_remote_credentials(cl, stream->ice_ufrag, stream->ice_pwd);
				break;
			}
		}
	}
	return iceRestarted;
}

void IceAgent::clearUnusedIceCandidates (const SalMediaDescription *localDesc, const SalMediaDescription *remoteDesc) {
	if (!localDesc)
		return;
	for (int i = 0; i < remoteDesc->nb_streams; i++) {
		const SalStreamDescription *localStream = &localDesc->streams[i];
		const SalStreamDescription *stream = &remoteDesc->streams[i];
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		if (!cl || !localStream)
			continue;
		if (stream->rtcp_mux && localStream->rtcp_mux) {
			ice_check_list_remove_rtcp_candidates(cl);
		}
	}
}

void IceAgent::createIceCheckListsAndParseIceAttributes (const SalMediaDescription *md, bool iceRestarted) {
	for (int i = 0; i < md->nb_streams; i++) {
		const SalStreamDescription *stream = &md->streams[i];
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		if (!cl)
			continue;
		if (stream->ice_mismatch) {
			ice_check_list_set_state(cl, ICL_Failed);
			continue;
		}
		if (stream->rtp_port == 0) {
			ice_session_remove_check_list(iceSession, cl);
			mediaSession.getPrivate()->clearIceCheckList(cl);
			continue;
		}
		if ((stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0'))
			ice_check_list_set_remote_credentials(cl, stream->ice_ufrag, stream->ice_pwd);
		for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES; j++) {
			bool defaultCandidate = false;
			const SalIceCandidate *candidate = &stream->ice_candidates[j];
			if (candidate->addr[0] == '\0')
				break;
			if ((candidate->componentID == 0) || (candidate->componentID > 2))
				continue;
			const char *addr = nullptr;
			int port = 0;
			getIceDefaultAddrAndPort(static_cast<uint16_t>(candidate->componentID), md, stream, &addr, &port);
			if (addr && (candidate->port == port) && (strlen(candidate->addr) == strlen(addr)) && (strcmp(candidate->addr, addr) == 0))
				defaultCandidate = true;
			int family = AF_INET;
			if (strchr(candidate->addr, ':'))
				family = AF_INET6;
			ice_add_remote_candidate(
				cl, candidate->type, family, candidate->addr, candidate->port,
				static_cast<uint16_t>(candidate->componentID),
				candidate->priority, candidate->foundation, defaultCandidate
			);
		}
		if (!iceRestarted) {
			bool_t losingPairsAdded = false;
			for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES; j++) {
				const SalIceRemoteCandidate *remoteCandidate = &stream->ice_remote_candidates[j];
				const char *addr = nullptr;
				int port = 0;
				int componentID = j + 1;
				if (remoteCandidate->addr[0] == '\0') break;
				getIceDefaultAddrAndPort(static_cast<uint16_t>(componentID), md, stream, &addr, &port);

				// If we receive a re-invite and we finished ICE processing on our side, use the candidates given by the remote.
				if (j == 0)
					ice_check_list_unselect_valid_pairs(cl);

				int remoteFamily = AF_INET;
				if (strchr(remoteCandidate->addr, ':'))
					remoteFamily = AF_INET6;
				int family = AF_INET;
				if (strchr(addr, ':'))
					family = AF_INET6;
				ice_add_losing_pair(cl, static_cast<uint16_t>(j + 1), remoteFamily, remoteCandidate->addr, remoteCandidate->port, family, addr, port);
				losingPairsAdded = true;
			}
			if (losingPairsAdded)
				ice_check_list_check_completed(cl);
		}
	}
}

/** Return values:
 *  1: STUN gathering is started
 *  0: no STUN gathering is started, but it's ok to proceed with ICE anyway (with local candidates only or because STUN gathering was already done before)
 * -1: no gathering started and something went wrong with local candidates. There is no way to start the ICE session.
 */
int IceAgent::gatherIceCandidates () {
	if (!iceSession)
		return -1;
	IceCheckList *audioCl = ice_session_check_list(iceSession, mediaSession.getPrivate()->getStreamIndex(LinphoneStreamTypeAudio));
	IceCheckList *videoCl = ice_session_check_list(iceSession, mediaSession.getPrivate()->getStreamIndex(LinphoneStreamTypeVideo));
	IceCheckList *textCl = ice_session_check_list(iceSession, mediaSession.getPrivate()->getStreamIndex(LinphoneStreamTypeText));
	if (!audioCl && !videoCl && !textCl)
		return -1;

	const struct addrinfo *ai = nullptr;
	LinphoneNatPolicy *natPolicy = mediaSession.getPrivate()->getNatPolicy();
	if (natPolicy && linphone_nat_policy_stun_server_activated(natPolicy)) {
		ai = linphone_nat_policy_get_stun_server_addrinfo(natPolicy);
		if (ai)
			ai = getIcePreferredStunServerAddrinfo(ai);
		else
			lWarning() << "Failed to resolve STUN server for ICE gathering, continuing without STUN";
	} else
		lWarning() << "ICE is used without STUN server";
	LinphoneCore *core = mediaSession.getCore()->getCCore();
	ice_session_enable_forced_relay(iceSession, core->forced_ice_relay);
	ice_session_enable_short_turn_refresh(iceSession, core->short_turn_refresh);

	// Gather local host candidates.
	char localAddr[64];
	if (mediaSession.getPrivate()->getAf() == AF_INET6) {
		if (linphone_core_get_local_ip_for(AF_INET6, nullptr, localAddr) < 0) {
			lError() << "Fail to get local IPv6";
			return -1;
		} else
			addLocalIceCandidates(AF_INET6, localAddr, audioCl, videoCl, textCl);
	}
	if (linphone_core_get_local_ip_for(AF_INET, nullptr, localAddr) < 0) {
		if (mediaSession.getPrivate()->getAf() != AF_INET6) {
			lError() << "Fail to get local IPv4";
			return -1;
		}
	} else
		addLocalIceCandidates(AF_INET, localAddr, audioCl, videoCl, textCl);
	if (ai && natPolicy && linphone_nat_policy_stun_server_activated(natPolicy)) {
		string server = linphone_nat_policy_get_stun_server(natPolicy);
		lInfo() << "ICE: gathering candidates from [" << server << "] using " << (linphone_nat_policy_turn_enabled(natPolicy) ? "TURN" : "STUN");
		// Gather local srflx candidates.
		ice_session_enable_turn(iceSession, linphone_nat_policy_turn_enabled(natPolicy));
		ice_session_set_stun_auth_requested_cb(iceSession, MediaSessionPrivate::stunAuthRequestedCb, mediaSession.getPrivate());
		return ice_session_gather_candidates(iceSession, ai->ai_addr, (socklen_t)ai->ai_addrlen) ? 1 : 0;
	} else {
		lInfo() << "ICE: bypass candidates gathering";
		ice_session_compute_candidates_foundations(iceSession);
		ice_session_eliminate_redundant_candidates(iceSession);
		ice_session_choose_default_candidates(iceSession);
	}
	return 0;
}

void IceAgent::getIceDefaultAddrAndPort (
	uint16_t componentID,
	const SalMediaDescription *md,
	const SalStreamDescription *stream,
	const char **addr,
	int *port
) {
	if (componentID == 1) {
		*addr = stream->rtp_addr;
		*port = stream->rtp_port;
	} else if (componentID == 2) {
		*addr = stream->rtcp_addr;
		*port = stream->rtcp_port;
	} else
		return;
	if ((*addr)[0] == '\0') *addr = md->addr;
}

/**
 * Choose the preferred IP address to use to contact the STUN server from the list of IP addresses
 * the DNS resolution returned. If a NAT64 address is present, use it, otherwise if an IPv4 address
 * is present, use it, otherwise use an IPv6 address if it is present.
 */
const struct addrinfo *IceAgent::getIcePreferredStunServerAddrinfo (const struct addrinfo *ai) {
	// Search for NAT64 addrinfo.
	const struct addrinfo *it = ai;
	while (it) {
		if (it->ai_family == AF_INET6) {
			struct sockaddr_storage ss;
			socklen_t sslen = sizeof(ss);
			memset(&ss, 0, sizeof(ss));
			bctbx_sockaddr_remove_nat64_mapping(it->ai_addr, (struct sockaddr *)&ss, &sslen);
			if (ss.ss_family == AF_INET) break;
		}
		it = it->ai_next;
	}
	const struct addrinfo *preferredAi = it;
	if (!preferredAi) {
		// Search for IPv4 addrinfo.
		it = ai;
		while (it) {
			if (it->ai_family == AF_INET)
				break;
			if ((it->ai_family == AF_INET6) && (it->ai_flags & AI_V4MAPPED))
				break;
			it = it->ai_next;
		}
		preferredAi = it;
	}
	if (!preferredAi) {
		// Search for IPv6 addrinfo.
		it = ai;
		while (it) {
			if (it->ai_family == AF_INET6)
				break;
			it = it->ai_next;
		}
		preferredAi = it;
	}
	return preferredAi;
}

bool IceAgent::iceParamsFoundInRemoteMediaDescription (const SalMediaDescription *md) {
	if ((md->ice_pwd[0] != '\0') && (md->ice_ufrag[0] != '\0'))
		return true;
	bool found = false;
	for (int i = 0; i < md->nb_streams; i++) {
		const SalStreamDescription *stream = &md->streams[i];
		IceCheckList *cl = ice_session_check_list(iceSession, i);
		if (cl) {
			if ((stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0'))
				found = true;
			else {
				found = false;
				break;
			}
		}
	}
	return found;
}

void IceAgent::updateIceStateInCallStatsForStream (LinphoneCallStats *stats, IceCheckList *cl) {
	if (ice_check_list_state(cl) != ICL_Completed) {
		_linphone_call_stats_set_ice_state(stats, LinphoneIceStateFailed);
		return;
	}

	switch (ice_check_list_selected_valid_candidate_type(cl)) {
		case ICT_HostCandidate:
			_linphone_call_stats_set_ice_state(stats, LinphoneIceStateHostConnection);
			break;
		case ICT_ServerReflexiveCandidate:
		case ICT_PeerReflexiveCandidate:
			_linphone_call_stats_set_ice_state(stats, LinphoneIceStateReflexiveConnection);
			break;
		case ICT_RelayedCandidate:
			_linphone_call_stats_set_ice_state(stats, LinphoneIceStateRelayConnection);
			break;
		case ICT_CandidateInvalid:
		case ICT_CandidateTypeMax:
			// Shall not happen.
			L_ASSERT(false);
			break;
	}
}

bool IceAgent::checkIceReinviteNeedsDeferedResponse(SalMediaDescription *md){
	int i,j;
	IceCheckList *cl;
	
	if (!iceSession) return false;

	if (ice_session_state(iceSession) != IS_Running ) return false;
	
	for (i = 0; i < md->nb_streams; i++) {
		SalStreamDescription *stream = &md->streams[i];
		cl = ice_session_check_list(iceSession, i);

		if (cl==NULL) continue;
		if (stream->ice_mismatch == TRUE) {
			return false;
		}
		if (stream->rtp_port == 0) {
			continue;
		}
		
		if (ice_check_list_state(cl) != ICL_Running) continue;

		for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES; j++) {
			const SalIceRemoteCandidate *remote_candidate = &stream->ice_remote_candidates[j];
			if (remote_candidate->addr[0] != '\0') return true;

		}
	}
	return false;
}

LINPHONE_END_NAMESPACE
