/*
 * media-session.cpp
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

#include <iomanip>
#include <math.h>

#include "address/address-p.h"
#include "c-wrapper/c-wrapper.h"
#include "conference/session/media-session-p.h"
#include "call/call-p.h"
#include "conference/participant-p.h"
#include "conference/params/media-session-params-p.h"

#include "conference/session/media-session.h"
#include "utils/payload-type-handler.h"

#include "logger/logger.h"

#include "linphone/core.h"

#include <bctoolbox/defs.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msequalizer.h>
#include <mediastreamer2/mseventqueue.h>
#include <mediastreamer2/msfileplayer.h>
#include <mediastreamer2/msjpegwriter.h>
#include <mediastreamer2/msrtt4103.h>
#include <mediastreamer2/msvolume.h>
#include <ortp/b64.h>

#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

#define STR_REASSIGN(dest, src) { \
	if (dest) \
		ms_free(dest); \
	dest = src; \
}

inline OrtpRtcpXrStatSummaryFlag operator|(OrtpRtcpXrStatSummaryFlag a, OrtpRtcpXrStatSummaryFlag b) {
	return static_cast<OrtpRtcpXrStatSummaryFlag>(static_cast<int>(a) | static_cast<int>(b));
}

// =============================================================================

const string MediaSessionPrivate::ecStateStore = ".linphone.ecstate";
const int MediaSessionPrivate::ecStateMaxLen = 1048576; /* 1Mo */

// =============================================================================

MediaSessionPrivate::MediaSessionPrivate (const Conference &conference, const CallSessionParams *csp, CallSessionListener *listener)
	: CallSessionPrivate(conference, csp, listener) {
	if (csp)
		params = new MediaSessionParams(*(reinterpret_cast<const MediaSessionParams *>(csp)));
	currentParams = new MediaSessionParams();

	audioStats = _linphone_call_stats_new();
	initStats(audioStats, LinphoneStreamTypeAudio);
	videoStats = _linphone_call_stats_new();
	initStats(videoStats, LinphoneStreamTypeVideo);
	textStats = _linphone_call_stats_new();
	initStats(textStats, LinphoneStreamTypeText);

	int minPort, maxPort;
	linphone_core_get_audio_port_range(core, &minPort, &maxPort);
	setPortConfig(mainAudioStreamIndex, make_pair(minPort, maxPort));
	linphone_core_get_video_port_range(core, &minPort, &maxPort);
	setPortConfig(mainVideoStreamIndex, make_pair(minPort, maxPort));
	linphone_core_get_text_port_range(core, &minPort, &maxPort);
	setPortConfig(mainTextStreamIndex, make_pair(minPort, maxPort));

	memset(sessions, 0, sizeof(sessions));
}

MediaSessionPrivate::~MediaSessionPrivate () {
	if (currentParams)
		delete currentParams;
	if (params)
		delete params;
	if (remoteParams)
		delete remoteParams;
	if (audioStats)
		linphone_call_stats_unref(audioStats);
	if (videoStats)
		linphone_call_stats_unref(videoStats);
	if (textStats)
		linphone_call_stats_unref(textStats);
	if (natPolicy)
		linphone_nat_policy_unref(natPolicy);
	if (stunClient)
		delete stunClient;
	delete iceAgent;
	if (localDesc)
		sal_media_description_unref(localDesc);
	if (biggestDesc)
		sal_media_description_unref(biggestDesc);
	if (resultDesc)
		sal_media_description_unref(resultDesc);
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::stunAuthRequestedCb (void *userData, const char *realm, const char *nonce, const char **username, const char **password, const char **ha1) {
	MediaSessionPrivate *msp = reinterpret_cast<MediaSessionPrivate *>(userData);
	msp->stunAuthRequestedCb(realm, nonce, username, password, ha1);
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::accepted () {
	L_Q();
	CallSessionPrivate::accepted();
	LinphoneTaskList tl;
	linphone_task_list_init(&tl);
	/* Reset the internal call update flag, so it doesn't risk to be copied and used in further re-INVITEs */
	params->getPrivate()->setInternalCallUpdate(false);
	SalMediaDescription *rmd = op->get_remote_media_description();
	SalMediaDescription *md = op->get_final_media_description();
	if (!md && (prevState == LinphoneCallOutgoingEarlyMedia) && resultDesc) {
		lInfo() << "Using early media SDP since none was received with the 200 OK";
		md = resultDesc;
	}
	if (md && (sal_media_description_empty(md) || linphone_core_incompatible_security(core, md)))
		md = nullptr;
	if (md) {
		/* There is a valid SDP in the response, either offer or answer, and we're able to start/update the streams */
		if (rmd) {
			/* Handle remote ICE attributes if any. */
			iceAgent->updateFromRemoteMediaDescription(localDesc, rmd, !op->is_offerer());
		}
		LinphoneCallState nextState = LinphoneCallIdle;
		string nextStateMsg;
		switch (state) {
			case LinphoneCallResuming:
			case LinphoneCallConnected:
#if 0
				if (call->referer)
					linphone_core_notify_refer_state(lc,call->referer,call);
#endif
				BCTBX_NO_BREAK; /* Intentional no break */
			case LinphoneCallUpdating:
			case LinphoneCallUpdatedByRemote:
				if (!sal_media_description_has_dir(localDesc, SalStreamInactive)
					&& (sal_media_description_has_dir(md, SalStreamRecvOnly) || sal_media_description_has_dir(md, SalStreamInactive))) {
					nextState = LinphoneCallPausedByRemote;
					nextStateMsg = "Call paused by remote";
				} else {
					if (!params->getPrivate()->getInConference() && listener)
						listener->onSetCurrentSession(q->getSharedFromThis());
					nextState = LinphoneCallStreamsRunning;
					nextStateMsg = "Streams running";
				}
				break;
			case LinphoneCallEarlyUpdating:
				nextState = prevState;
				nextStateMsg = "Early update accepted";
				break;
			case LinphoneCallPausing:
				/* When we entered the pausing state, we always reach the paused state whatever the content of the remote SDP is.
				 * Our streams are all send-only (with music), soundcard and camera are never used. */
				nextState = LinphoneCallPaused;
				nextStateMsg = "Call paused";
#if 0
				if (call->refer_pending)
					linphone_task_list_add(&tl, (LinphoneCoreIterateHook)start_pending_refer, call);
#endif
				break;
			default:
				lError() << "accepted(): don't know what to do in state [" << linphone_call_state_to_string(state) << "]";
				break;
		}

		if (nextState == LinphoneCallIdle)
			lError() << "BUG: nextState is not set in accepted(), current state is " << linphone_call_state_to_string(state);
		else {
			updateRemoteSessionIdAndVer();
			iceAgent->updateIceStateInCallStats();
			updateStreams(md, nextState);
			fixCallParams(rmd);
			setState(nextState, nextStateMsg);
		}
	} else { /* Invalid or no SDP */
		switch (prevState) {
			/* Send a bye only in case of early states */
			case LinphoneCallOutgoingInit:
			case LinphoneCallOutgoingProgress:
			case LinphoneCallOutgoingRinging:
			case LinphoneCallOutgoingEarlyMedia:
			case LinphoneCallIncomingReceived:
			case LinphoneCallIncomingEarlyMedia:
				lError() << "Incompatible SDP answer received, need to abort the call";
				abort("Incompatible, check codecs or security settings...");
				break;
			/* Otherwise we are able to resume previous state */
			default:
				lError() << "Incompatible SDP answer received";
				switch(state) {
					case LinphoneCallPausedByRemote:
					case LinphoneCallPaused:
					case LinphoneCallStreamsRunning:
						break;
					default:
						lInfo() << "Incompatible SDP answer received, restoring previous state [" << linphone_call_state_to_string(prevState) << "]";
						setState(prevState, "Incompatible media parameters.");
						break;
				}
				break;
		}
	}
	linphone_task_list_run(&tl);
	linphone_task_list_free(&tl);
}

void MediaSessionPrivate::ackReceived (LinphoneHeaders *headers) {
	CallSessionPrivate::ackReceived(headers);
	if (expectMediaInAck) {
		switch (state) {
			case LinphoneCallStreamsRunning:
			case LinphoneCallPausedByRemote:
				setState(LinphoneCallUpdatedByRemote, "UpdatedByRemote");
				break;
			default:
				break;
		}
		accepted();
	}
}

bool MediaSessionPrivate::failure () {
	L_Q();
	const SalErrorInfo *ei = op->get_error_info();
	switch (ei->reason) {
		case SalReasonRedirect:
			stopStreams();
			break;
		case SalReasonUnsupportedContent: /* This is for compatibility: linphone sent 415 because of SDP offer answer failure */
		case SalReasonNotAcceptable:
			lInfo() << "Outgoing CallSession [" << q << "] failed with SRTP and/or AVPF enabled";
			if ((state == LinphoneCallOutgoingInit) || (state == LinphoneCallOutgoingProgress)
				|| (state == LinphoneCallOutgoingRinging) /* Push notification case */ || (state == LinphoneCallOutgoingEarlyMedia)) {
				for (int i = 0; i < localDesc->nb_streams; i++) {
					if (!sal_stream_description_active(&localDesc->streams[i]))
						continue;
					if (params->getMediaEncryption() == LinphoneMediaEncryptionSRTP) {
						if (params->avpfEnabled()) {
							if (i == 0)
								lInfo() << "Retrying CallSession [" << q << "] with SAVP";
							params->enableAvpf(false);
							restartInvite();
							return true;
						} else if (!linphone_core_is_media_encryption_mandatory(core)) {
							if (i == 0)
								lInfo() << "Retrying CallSession [" << q << "] with AVP";
							params->setMediaEncryption(LinphoneMediaEncryptionNone);
							memset(localDesc->streams[i].crypto, 0, sizeof(localDesc->streams[i].crypto));
							restartInvite();
							return true;
						}
					} else if (params->avpfEnabled()) {
						if (i == 0)
							lInfo() << "Retrying CallSession [" << q << "] with AVP";
						params->enableAvpf(false);
						restartInvite();
						return true;
					}
				}
			}
			break;
		default:
			break;
	}

	bool stop = CallSessionPrivate::failure();
	if (stop)
		return true;

#if 0
	/* Stop ringing */
	bool_t ring_during_early_media = linphone_core_get_ring_during_incoming_early_media(lc);
	bool_t stop_ringing = TRUE;
	bctbx_list_t *calls = lc->calls;
	while(calls) {
		if (((LinphoneCall *)calls->data)->state == LinphoneCallIncomingReceived || (ring_during_early_media && ((LinphoneCall *)calls->data)->state == LinphoneCallIncomingEarlyMedia)) {
			stop_ringing = FALSE;
			break;
		}
		calls = calls->next;
	}
	if(stop_ringing) {
		linphone_core_stop_ringing(lc);
	}
#endif
	stopStreams();
	return false;
}

void MediaSessionPrivate::pausedByRemote () {
	MediaSessionParams *newParams = new MediaSessionParams(*params);
	if (lp_config_get_int(linphone_core_get_config(core), "sip", "inactive_video_on_pause", 0))
		newParams->setVideoDirection(LinphoneMediaDirectionInactive);
	acceptUpdate(newParams, LinphoneCallPausedByRemote, "Call paused by remote");
}

void MediaSessionPrivate::remoteRinging () {
	L_Q();
	/* Set privacy */
	q->getCurrentParams()->setPrivacy((LinphonePrivacyMask)op->get_privacy());
	SalMediaDescription *md = op->get_final_media_description();
	if (md) {
		/* Initialize the remote call params by invoking linphone_call_get_remote_params(). This is useful as the SDP may not be present in the 200Ok */
		q->getRemoteParams();
		/* Accept early media */
		if ((audioStream && audio_stream_started(audioStream))
#ifdef VIDEO_ENABLED
			|| (videoStream && video_stream_started(videoStream))
#endif
			) {
			/* Streams already started */
			tryEarlyMediaForking(md);
#ifdef VIDEO_ENABLED
			if (videoStream)
				video_stream_send_vfu(videoStream); /* Request for iframe */
#endif
			return;
		}

		setState(LinphoneCallOutgoingEarlyMedia, "Early media");
#if 0
		linphone_core_stop_ringing(lc);
#endif
		lInfo() << "Doing early media...";
		updateStreams(md, state);
		if ((q->getCurrentParams()->getAudioDirection() == LinphoneMediaDirectionInactive) && audioStream) {
#if 0
			if (lc->ringstream != NULL) return; /* Already ringing! */
			start_remote_ring(lc, call);
#endif
		}
	} else {
		linphone_core_stop_dtmf_stream(core);
		if (state == LinphoneCallOutgoingEarlyMedia) {
			/* Already doing early media */
			return;
		}
#if 0
		if (lc->ringstream == NULL) start_remote_ring(lc, call);
#endif
		lInfo() << "Remote ringing...";
		setState(LinphoneCallOutgoingRinging, "Remote ringing");
	}
}

void MediaSessionPrivate::resumed () {
	acceptUpdate(nullptr, LinphoneCallStreamsRunning, "Connected (streams running)");
}

void MediaSessionPrivate::terminated () {
	stopStreams();
	CallSessionPrivate::terminated();
}

/* This callback is called when an incoming re-INVITE/ SIP UPDATE modifies the session */
void MediaSessionPrivate::updated (bool isUpdate) {
	SalMediaDescription *rmd = op->get_remote_media_description();
	switch (state) {
		case LinphoneCallPausedByRemote:
			if (sal_media_description_has_dir(rmd, SalStreamSendRecv) || sal_media_description_has_dir(rmd, SalStreamRecvOnly)) {
				resumed();
				return;
			}
			break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallConnected:
		case LinphoneCallUpdatedByRemote: /* Can happen on UAC connectivity loss */
			if (sal_media_description_has_dir(rmd, SalStreamSendOnly) || sal_media_description_has_dir(rmd, SalStreamInactive)) {
				pausedByRemote();
				return;
			}
			break;
		default:
			/* The other cases are handled in CallSessionPrivate::updated */
			break;
	}
	CallSessionPrivate::updated(isUpdate);
}

void MediaSessionPrivate::updating (bool isUpdate) {
	L_Q();
	SalMediaDescription *rmd = op->get_remote_media_description();
	fixCallParams(rmd);
	if (state != LinphoneCallPaused) {
		/* Refresh the local description, but in paused state, we don't change anything. */
		if (!rmd && lp_config_get_int(linphone_core_get_config(core), "sip", "sdp_200_ack_follow_video_policy", 0)) {
			lInfo() << "Applying default policy for offering SDP on CallSession [" << q << "]";
			params = new MediaSessionParams();
			params->initDefault(core);
		}
		makeLocalMediaDescription();
		op->set_local_media_description(localDesc);
	}
	if (rmd) {
		SalErrorInfo sei;
		memset(&sei, 0, sizeof(sei));
		expectMediaInAck = false;
		SalMediaDescription *md = op->get_final_media_description();
		if (md && (sal_media_description_empty(md) || linphone_core_incompatible_security(core, md))) {
			sal_error_info_set(&sei, SalReasonNotAcceptable, "SIP", 0, nullptr, nullptr);
			op->decline_with_error_info(&sei, nullptr);
			sal_error_info_reset(&sei);
			return;
		}
		SalMediaDescription *prevResultDesc = resultDesc;
		if (isUpdate && prevResultDesc && md){
			int diff = sal_media_description_equals(prevResultDesc, md);
			if (diff & (SAL_MEDIA_DESCRIPTION_CRYPTO_POLICY_CHANGED | SAL_MEDIA_DESCRIPTION_STREAMS_CHANGED)) {
				lWarning() << "Cannot accept this update, it is changing parameters that require user approval";
				sal_error_info_set(&sei, SalReasonUnknown, "SIP", 504, "Cannot change the session parameters without prompting the user", nullptr);
				op->decline_with_error_info(&sei, nullptr);
				sal_error_info_reset(&sei);
				return;
			}
		}
		updated(isUpdate);
	} else {
		/* Case of a reINVITE or UPDATE without SDP */
		expectMediaInAck = true;
		op->accept(); /* Respond with an offer */
		/* Don't do anything else in this case, wait for the ACK to receive to notify the app */
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::enableSymmetricRtp (bool value) {
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (sessions[i].rtp_session)
			rtp_session_set_symmetric_rtp(sessions[i].rtp_session, value);
	}
}

void MediaSessionPrivate::sendVfu () {
#ifdef VIDEO_ENABLED
	if (videoStream)
		video_stream_send_vfu(videoStream);
#endif
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::clearIceCheckList (IceCheckList *cl) {
	if (audioStream && audioStream->ms.ice_check_list == cl)
		audioStream->ms.ice_check_list = nullptr;
	if (videoStream && videoStream->ms.ice_check_list == cl)
		videoStream->ms.ice_check_list = nullptr;
	if (textStream && textStream->ms.ice_check_list == cl)
		textStream->ms.ice_check_list = nullptr;
}

void MediaSessionPrivate::deactivateIce () {
	if (audioStream)
		audioStream->ms.ice_check_list = nullptr;
	if (videoStream)
		videoStream->ms.ice_check_list = nullptr;
	if (textStream)
		textStream->ms.ice_check_list = nullptr;
	_linphone_call_stats_set_ice_state(audioStats, LinphoneIceStateNotActivated);
	_linphone_call_stats_set_ice_state(videoStats, LinphoneIceStateNotActivated);
	_linphone_call_stats_set_ice_state(textStats, LinphoneIceStateNotActivated);
	stopStreamsForIceGathering();
}

void MediaSessionPrivate::prepareStreamsForIceGathering (bool hasVideo) {
	if (audioStream->ms.state == MSStreamInitialized)
		audio_stream_prepare_sound(audioStream, nullptr, nullptr);
#ifdef VIDEO_ENABLED
	if (hasVideo && videoStream && (videoStream->ms.state == MSStreamInitialized))
		video_stream_prepare_video(videoStream);
#endif
	if (params->realtimeTextEnabled() && (textStream->ms.state == MSStreamInitialized))
		text_stream_prepare_text(textStream);
}

void MediaSessionPrivate::stopStreamsForIceGathering () {
	if (audioStream && (audioStream->ms.state == MSStreamPreparing))
		audio_stream_unprepare_sound(audioStream);
#ifdef VIDEO_ENABLED
	if (videoStream && (videoStream->ms.state == MSStreamPreparing))
		video_stream_unprepare_video(videoStream);
#endif
	if (textStream && (textStream->ms.state == MSStreamPreparing))
		text_stream_unprepare_text(textStream);
}

// -----------------------------------------------------------------------------

MediaStream * MediaSessionPrivate::getMediaStream (LinphoneStreamType type) const {
	switch (type) {
		case LinphoneStreamTypeAudio:
			return &audioStream->ms;
		case LinphoneStreamTypeVideo:
			return &videoStream->ms;
		case LinphoneStreamTypeText:
			return &textStream->ms;
		case LinphoneStreamTypeUnknown:
		default:
			return nullptr;
	}
}

int MediaSessionPrivate::getRtcpPort (LinphoneStreamType type) const  {
	return mediaPorts[getStreamIndex(getMediaStream(type))].rtcpPort;
}

int MediaSessionPrivate::getRtpPort (LinphoneStreamType type) const {
	return mediaPorts[getStreamIndex(getMediaStream(type))].rtpPort;
}

LinphoneCallStats * MediaSessionPrivate::getStats (LinphoneStreamType type) const {
	switch (type) {
		case LinphoneStreamTypeAudio:
			return audioStats;
		case LinphoneStreamTypeVideo:
			return videoStats;
		case LinphoneStreamTypeText:
			return textStats;
		case LinphoneStreamTypeUnknown:
		default:
			return nullptr;
	}
}

int MediaSessionPrivate::getStreamIndex (LinphoneStreamType type) const {
	return getStreamIndex(getMediaStream(type));
}

int MediaSessionPrivate::getStreamIndex (MediaStream *ms) const {
	if (ms == &audioStream->ms)
		return mainAudioStreamIndex;
	else if (ms == &videoStream->ms)
		return mainVideoStreamIndex;
	else if (ms == &textStream->ms)
		return mainTextStreamIndex;
	return -1;
}

// -----------------------------------------------------------------------------

OrtpJitterBufferAlgorithm MediaSessionPrivate::jitterBufferNameToAlgo (const string &name) {
	if (name == "basic") return OrtpJitterBufferBasic;
	if (name == "rls") return OrtpJitterBufferRecursiveLeastSquare;
	lError() << "Invalid jitter buffer algorithm: " << name;
	return OrtpJitterBufferRecursiveLeastSquare;
}

#ifdef VIDEO_ENABLED
void MediaSessionPrivate::videoStreamEventCb (void *userData, const MSFilter *f, const unsigned int eventId, const void *args) {
	MediaSessionPrivate *msp = reinterpret_cast<MediaSessionPrivate *>(userData);
	msp->videoStreamEventCb(f, eventId, args);
}
#endif

#ifdef TEST_EXT_RENDERER
void MediaSessionPrivate::extRendererCb (void *userData, const MSPicture *local, const MSPicture *remote) {
	lInfo() << "extRendererCb, local buffer=" << local ? local->planes[0] : nullptr
		<< ", remote buffer=" << remote ? remote->planes[0] : nullptr);
}
#endif

void MediaSessionPrivate::realTimeTextCharacterReceived (void *userData, MSFilter *f, unsigned int id, void *arg) {
	MediaSessionPrivate *msp = reinterpret_cast<MediaSessionPrivate *>(userData);
	msp->realTimeTextCharacterReceived(f, id, arg);
}

// -----------------------------------------------------------------------------

float MediaSessionPrivate::aggregateQualityRatings (float audioRating, float videoRating) {
	float result;
	if ((audioRating < 0) && (videoRating < 0))
		result = -1;
	else if (audioRating < 0)
		result = videoRating * 5.0f;
	else if (videoRating < 0)
		result = audioRating * 5.0f;
	else
		result = audioRating * videoRating * 5.0f;
	return result;
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::setState (LinphoneCallState newState, const string &message) {
	L_Q();
	/* Take a ref on the session otherwise it might get destroyed during the call to setState */
	shared_ptr<CallSession> sessionRef = q->getSharedFromThis();
	CallSessionPrivate::setState(newState, message);
	updateReportingCallState();
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::computeStreamsIndexes (const SalMediaDescription *md) {
	bool audioFound = false;
	bool videoFound = false;
	bool textFound = false;
	for (int i = 0; i < md->nb_streams; i++) {
		if (md->streams[i].type == SalAudio) {
			if (audioFound)
				lInfo() << "audio stream index found: " << i << ", but main audio stream already set to " << mainAudioStreamIndex;
			else {
				mainAudioStreamIndex = i;
				audioFound = true;
				lInfo() << "audio stream index found: " << i << ", updating main audio stream index";
			}
			/* Check that the default value of a another stream doesn't match the new one */
			if (i == mainVideoStreamIndex) {
				for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j]))
						continue;
					if ((j != mainVideoStreamIndex) && (j != mainTextStreamIndex)) {
						lInfo() << i << " was used for video stream ; now using " << j;
						mainVideoStreamIndex = j;
						break;
					}
				}
			}
			if (i == mainTextStreamIndex) {
				for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j]))
						continue;
					if ((j != mainVideoStreamIndex) && (j != mainTextStreamIndex)) {
						lInfo() << i << " was used for text stream ; now using " << j;
						mainTextStreamIndex = j;
						break;
					}
				}
			}
		} else if (md->streams[i].type == SalVideo) {
			if (videoFound)
				lInfo() << "video stream index found: " << i << ", but main video stream already set to " << mainVideoStreamIndex;
			else {
				mainVideoStreamIndex = i;
				videoFound = true;
				lInfo() << "video stream index found: " << i << ", updating main video stream index";
			}
			/* Check that the default value of a another stream doesn't match the new one */
			if (i == mainAudioStreamIndex) {
				for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j]))
						continue;
					if ((j != mainAudioStreamIndex) && (j != mainTextStreamIndex)) {
						lInfo() << i << " was used for audio stream ; now using " << j;
						mainAudioStreamIndex = j;
						break;
					}
				}
			}
			if (i == mainTextStreamIndex) {
				for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j]))
						continue;
					if ((j != mainAudioStreamIndex) && (j != mainTextStreamIndex)) {
						lInfo() << i << " was used for text stream ; now using " << j;
						mainTextStreamIndex = j;
						break;
					}
				}
			}
		} else if (md->streams[i].type == SalText) {
			if (textFound)
				lInfo() << "text stream index found: " << i << ", but main text stream already set to " << mainTextStreamIndex;
			else {
				mainTextStreamIndex = i;
				textFound = true;
				lInfo() << "text stream index found: " << i << ", updating main text stream index";
			}
			/* Check that the default value of a another stream doesn't match the new one */
			if (i == mainAudioStreamIndex) {
				for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j]))
						continue;
					if ((j != mainVideoStreamIndex) && (j != mainAudioStreamIndex)) {
						lInfo() << i << " was used for audio stream ; now using " << j;
						mainAudioStreamIndex = j;
						break;
					}
				}
			}
			if (i == mainVideoStreamIndex) {
				for (int j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j]))
						continue;
					if ((j != mainVideoStreamIndex) && (j != mainAudioStreamIndex)) {
						lInfo() << i << " was used for video stream ; now using " << j;
						mainVideoStreamIndex = j;
						break;
					}
				}
			}
		}
	}
}

/*
 * This method needs to be called at each incoming reINVITE, in order to adjust various local parameters to what is being offered by remote:
 * - the stream indexes.
 * - the video enablement parameter according to what is offered and our local policy.
 * Fixing the params to proper values avoid request video by accident during internal call updates, pauses and resumes
 */
void MediaSessionPrivate::fixCallParams (SalMediaDescription *rmd) {
	L_Q();
	if (rmd) {
		computeStreamsIndexes(rmd);
		updateBiggestDesc(rmd);
		/* Why disabling implicit_rtcp_fb ? It is a local policy choice actually. It doesn't disturb to propose it again and again
		 * even if the other end apparently doesn't support it.
		 * The following line of code is causing trouble, while for example making an audio call, then adding video.
		 * Due to the 200Ok response of the audio-only offer where no rtcp-fb attribute is present, implicit_rtcp_fb is set to
		 * false, which is then preventing it to be eventually used when video is later added to the call.
		 * I did the choice of commenting it out.
		 */
		/*params.getPrivate()->enableImplicitRtcpFb(params.getPrivate()->implicitRtcpFbEnabled() & sal_media_description_has_implicit_avpf(rmd));*/
	}
	const MediaSessionParams *rcp = q->getRemoteParams();
	if (rcp) {
		if (params->audioEnabled() && !rcp->audioEnabled()) {
			lInfo() << "CallSession [" << q << "]: disabling audio in our call params because the remote doesn't want it";
			params->enableAudio(false);
		}
		if (params->videoEnabled() && !rcp->videoEnabled()) {
			lInfo() << "CallSession [" << q << "]: disabling video in our call params because the remote doesn't want it";
			params->enableVideo(false);
		}
		if (rcp->videoEnabled() && core->video_policy.automatically_accept && linphone_core_video_enabled(core) && !params->videoEnabled()) {
			lInfo() << "CallSession [" << q << "]: re-enabling video in our call params because the remote wants it and the policy allows to automatically accept";
			params->enableVideo(true);
		}
		if (rcp->realtimeTextEnabled() && !params->realtimeTextEnabled())
			params->enableRealtimeText(true);
	}
}

void MediaSessionPrivate::initializeParamsAccordingToIncomingCallParams () {
	CallSessionPrivate::initializeParamsAccordingToIncomingCallParams();
	currentParams->getPrivate()->setUpdateCallWhenIceCompleted(params->getPrivate()->getUpdateCallWhenIceCompleted());
	params->enableVideo(linphone_core_video_enabled(core) && core->video_policy.automatically_accept);
	SalMediaDescription *md = op->get_remote_media_description();
	if (md) {
		/* It is licit to receive an INVITE without SDP, in this case WE choose the media parameters according to policy */
		setCompatibleIncomingCallParams(md);
		/* Set multicast role & address if any */
		if (!op->is_offerer()) {
			for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
				if (md->streams[i].dir == SalStreamInactive)
					continue;
				if ((md->streams[i].rtp_addr[0] != '\0') && ms_is_multicast(md->streams[i].rtp_addr)) {
					md->streams[i].multicast_role = SalMulticastReceiver;
					mediaPorts[i].multicastIp = md->streams[i].rtp_addr;
				}
			}
		}
	}
}

/**
 * Fix call parameters on incoming call to eg. enable AVPF if the incoming call propose it and it is not enabled locally.
 */
void MediaSessionPrivate::setCompatibleIncomingCallParams (SalMediaDescription *md) {
	/* Handle AVPF, SRTP and DTLS */
	params->enableAvpf(!!sal_media_description_has_avpf(md));
	if (destProxy)
		params->setAvpfRrInterval(static_cast<uint16_t>(linphone_proxy_config_get_avpf_rr_interval(destProxy) * 1000));
	else
		params->setAvpfRrInterval(static_cast<uint16_t>(linphone_core_get_avpf_rr_interval(core) * 1000));
	if (sal_media_description_has_zrtp(md) && linphone_core_media_encryption_supported(core, LinphoneMediaEncryptionZRTP))
		params->setMediaEncryption(LinphoneMediaEncryptionZRTP);
	else if (sal_media_description_has_dtls(md) && media_stream_dtls_supported())
		params->setMediaEncryption(LinphoneMediaEncryptionDTLS);
	else if (sal_media_description_has_srtp(md) && ms_srtp_supported())
		params->setMediaEncryption(LinphoneMediaEncryptionSRTP);
	else if (params->getMediaEncryption() != LinphoneMediaEncryptionZRTP)
		params->setMediaEncryption(LinphoneMediaEncryptionNone);
	/* In case of nat64, even ipv4 addresses are reachable from v6. Should be enhanced to manage stream by stream connectivity (I.E v6 or v4) */
	/*if (!sal_media_description_has_ipv6(md)){
		lInfo() << "The remote SDP doesn't seem to offer any IPv6 connectivity, so disabling IPv6 for this call";
		af = AF_INET;
	}*/
	fixCallParams(md);
}

void MediaSessionPrivate::updateBiggestDesc (SalMediaDescription *md) {
	if (!biggestDesc || (md->nb_streams > biggestDesc->nb_streams)) {
		/* We have been offered and now are ready to proceed, or we added a new stream,
		 * store the media description to remember the mapping of calls */
		if (biggestDesc) {
			sal_media_description_unref(biggestDesc);
			biggestDesc = nullptr;
		}
		biggestDesc = sal_media_description_ref(md);
	}
}

void MediaSessionPrivate::updateRemoteSessionIdAndVer () {
	SalMediaDescription *desc = op->get_remote_media_description();
	if (desc) {
		remoteSessionId = desc->session_id;
		remoteSessionVer = desc->session_ver;
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::initStats (LinphoneCallStats *stats, LinphoneStreamType type) {
	_linphone_call_stats_set_type(stats, type);
	_linphone_call_stats_set_received_rtcp(stats, nullptr);
	_linphone_call_stats_set_sent_rtcp(stats, nullptr);
	_linphone_call_stats_set_ice_state(stats, LinphoneIceStateNotActivated);
}

void MediaSessionPrivate::notifyStatsUpdated (int streamIndex) const {
	LinphoneCallStats *stats = nullptr;
	if (streamIndex == mainAudioStreamIndex)
		stats = audioStats;
	else if (streamIndex == mainVideoStreamIndex)
		stats = videoStats;
	else if (streamIndex == mainTextStreamIndex)
		stats = textStats;
	else
		return;
	if (_linphone_call_stats_get_updated(stats)) {
		switch (_linphone_call_stats_get_updated(stats)) {
			case LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE:
			case LINPHONE_CALL_STATS_SENT_RTCP_UPDATE:
#if 0
				linphone_reporting_on_rtcp_update(call, stream_index == call->main_audio_stream_index ? SalAudio : stream_index == call->main_video_stream_index ? SalVideo : SalText);
#endif
				break;
			default:
				break;
		}
		if (listener)
			listener->onStatsUpdated(stats);
		_linphone_call_stats_set_updated(stats, 0);
	}
}

// -----------------------------------------------------------------------------

OrtpEvQueue * MediaSessionPrivate::getEventQueue (int streamIndex) const {
	if (streamIndex == mainAudioStreamIndex)
		return audioStreamEvQueue;
	if (streamIndex == mainVideoStreamIndex)
		return videoStreamEvQueue;
	if (streamIndex == mainTextStreamIndex)
		return textStreamEvQueue;
	lError() << "getEventQueue(): no stream index " << streamIndex;
	return nullptr;
}

MediaStream * MediaSessionPrivate::getMediaStream (int streamIndex) const {
	if (streamIndex == mainAudioStreamIndex)
		return &audioStream->ms;
	if (streamIndex == mainVideoStreamIndex)
		return &videoStream->ms;
	if (streamIndex == mainTextStreamIndex)
		return &textStream->ms;
	lError() << "getMediaStream(): no stream index " << streamIndex;
	return nullptr;
}

MSWebCam * MediaSessionPrivate::getVideoDevice () const {
	bool paused = (state == LinphoneCallPausing) || (state == LinphoneCallPaused);
	if (paused || allMuted || !cameraEnabled)
		return get_nowebcam_device(core->factory);
	else
		return core->video_conf.device;
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::fillMulticastMediaAddresses () {
	if (params->audioMulticastEnabled())
		mediaPorts[mainAudioStreamIndex].multicastIp = linphone_core_get_audio_multicast_addr(core);
	if (params->videoMulticastEnabled())
		mediaPorts[mainVideoStreamIndex].multicastIp = linphone_core_get_video_multicast_addr(core);
}

int MediaSessionPrivate::selectFixedPort (int streamIndex, pair<int, int> portRange) {
	for (int triedPort = portRange.first; triedPort < (portRange.first + 100); triedPort += 2) {
		bool alreadyUsed = false;
		for (const bctbx_list_t *elem = linphone_core_get_calls(core); elem != nullptr; elem = bctbx_list_next(elem)) {
			LinphoneCall *lcall = reinterpret_cast<LinphoneCall *>(bctbx_list_get_data(elem));
			MediaSession *session = dynamic_cast<MediaSession *>(L_GET_PRIVATE_FROM_C_OBJECT(lcall)->getConference()->getActiveParticipant()->getPrivate()->getSession().get());
			int existingPort = session->getPrivate()->mediaPorts[streamIndex].rtpPort;
			if (existingPort == triedPort) {
				alreadyUsed = true;
				break;
			}
		}
		if (!alreadyUsed)
			return triedPort;
	}

	lError() << "Could not find any free port !";
	return -1;
}

int MediaSessionPrivate::selectRandomPort (int streamIndex, pair<int, int> portRange) {
	for (int nbTries = 0; nbTries < 100; nbTries++) {
		bool alreadyUsed = false;
		int triedPort = static_cast<int>((ortp_random() % (portRange.second - portRange.first) + portRange.first) & ~0x1);
		if (triedPort < portRange.first) triedPort = portRange.first + 2;
		for (const bctbx_list_t *elem = linphone_core_get_calls(core); elem != nullptr; elem = bctbx_list_next(elem)) {
			LinphoneCall *lcall = reinterpret_cast<LinphoneCall *>(bctbx_list_get_data(elem));
			MediaSession *session = dynamic_cast<MediaSession *>(L_GET_PRIVATE_FROM_C_OBJECT(lcall)->getConference()->getActiveParticipant()->getPrivate()->getSession().get());
			int existingPort = session->getPrivate()->mediaPorts[streamIndex].rtpPort;
			if (existingPort == triedPort) {
				alreadyUsed = true;
				break;
			}
		}
		if (!alreadyUsed)
			return triedPort;
	}

	lError() << "Could not find any free port!";
	return -1;
}

void MediaSessionPrivate::setPortConfig(int streamIndex, pair<int, int> portRange) {
	if ((portRange.first <= 0) && (portRange.second <= 0)) {
		setRandomPortConfig(streamIndex);
	} else {
		if (portRange.first == portRange.second) {
			/* Fixed port */
			int port = selectFixedPort(streamIndex, portRange);
			if (port == -1) {
				setRandomPortConfig(streamIndex);
				return;
			}
			mediaPorts[streamIndex].rtpPort = port;
		} else {
			/* Select random port in the specified range */
			mediaPorts[streamIndex].rtpPort = selectRandomPort(streamIndex, portRange);
		}
		mediaPorts[streamIndex].rtcpPort = mediaPorts[streamIndex].rtpPort + 1;
	}
}

void MediaSessionPrivate::setPortConfigFromRtpSession (int streamIndex, RtpSession *session) {
	mediaPorts[streamIndex].rtpPort = rtp_session_get_local_port(session);
	mediaPorts[streamIndex].rtcpPort = rtp_session_get_local_rtcp_port(session);
}

void MediaSessionPrivate::setRandomPortConfig (int streamIndex) {
	mediaPorts[streamIndex].rtpPort = -1;
	mediaPorts[streamIndex].rtcpPort = -1;
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::discoverMtu (const Address &remoteAddr) {
	if (core->net_conf.mtu == 0) {
		/* Attempt to discover mtu */
		int mtu = ms_discover_mtu(remoteAddr.getDomain().c_str());
		if (mtu > 0) {
			ms_factory_set_mtu(core->factory, mtu);
			lInfo() << "Discovered mtu is " << mtu << ", RTP payload max size is " << ms_factory_get_payload_max_size(core->factory);
		}
	}
}

string MediaSessionPrivate::getBindIpForStream (int streamIndex) {
	string bindIp = lp_config_get_string(linphone_core_get_config(core), "rtp", "bind_address", (af == AF_INET6) ? "::0" : "0.0.0.0");
	PortConfig *pc = &mediaPorts[streamIndex];
	if (!pc->multicastIp.empty()){
		if (direction == LinphoneCallOutgoing) {
			/* As multicast sender, we must decide a local interface to use to send multicast, and bind to it */
			char multicastBindIp[LINPHONE_IPADDR_SIZE];
			memset(multicastBindIp, 0, sizeof(multicastBindIp));
			linphone_core_get_local_ip_for(pc->multicastIp.find_first_of(':') ? AF_INET6 : AF_INET, nullptr, multicastBindIp);
			bindIp = pc->multicastBindIp = multicastBindIp;
		} else {
			/* Otherwise we shall use an address family of the same family of the multicast address, because
			 * dual stack socket and multicast don't work well on Mac OS (linux is OK, as usual). */
			bindIp = pc->multicastIp.find_first_of(':') ? "::0" : "0.0.0.0";
		}
	}
	return bindIp;
}

/**
 * Fill the local ip that routes to the internet according to the destination, or guess it by other special means.
 */
void MediaSessionPrivate::getLocalIp (const Address &remoteAddr) {
	/* Next, sometime, override from config */
	const char *ip = lp_config_get_string(linphone_core_get_config(core), "rtp", "bind_address", nullptr);
	if (ip) {
		mediaLocalIp = ip;
		return;
	}

	/* If a known proxy was identified for this call, then we may have a chance to take the local ip address
	 * from the socket that connects to this proxy */
	if (destProxy && destProxy->op) {
		ip = destProxy->op->get_local_address(nullptr);
		if (ip) {
			lInfo() << "Found media local-ip from signaling.";
			mediaLocalIp = ip;
			return;
		}
	}

	/* In last resort, attempt to find the local ip that routes to destination if given as an IP address,
	   or the default route (dest == nullptr) */
	const char *dest = nullptr;
	if (!destProxy) {
		struct addrinfo hints;
		struct addrinfo *res = nullptr;
		int err;
		/* FIXME the following doesn't work for IPv6 address because of brakets */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_NUMERICHOST;
		err = getaddrinfo(remoteAddr.getDomain().c_str(), NULL, &hints, &res);
		if (err == 0)
			dest = remoteAddr.getDomain().c_str();
		if (res) freeaddrinfo(res);
	}

	if (dest || mediaLocalIp.empty() || needMediaLocalIpRefresh) {
		needMediaLocalIpRefresh = false;
		char ip[LINPHONE_IPADDR_SIZE];
		linphone_core_get_local_ip(core, af, dest, ip);
		mediaLocalIp = ip;
	}
}

string MediaSessionPrivate::getPublicIpForStream (int streamIndex) {
	if (!mediaPorts[streamIndex].multicastIp.empty())
		return mediaPorts[streamIndex].multicastIp;
	return mediaLocalIp;
}

void MediaSessionPrivate::runStunTestsIfNeeded () {
	if (linphone_nat_policy_stun_enabled(natPolicy) && !(linphone_nat_policy_ice_enabled(natPolicy) || linphone_nat_policy_turn_enabled(natPolicy))) {
		stunClient = new StunClient(core);
		int ret = stunClient->run(mediaPorts[mainAudioStreamIndex].rtpPort, mediaPorts[mainVideoStreamIndex].rtpPort, mediaPorts[mainTextStreamIndex].rtpPort);
		if (ret >= 0)
			pingTime = ret;
	}
}

/*
 * Select IP version to use for advertising local addresses of RTP streams, for an incoming call.
 * If the call is received through a know proxy that is IPv6, use IPv6.
 * Otherwise check the remote contact address.
 * If later the resulting media description tells that we have to send IPv4, it won't be a problem because the RTP sockets
 * are dual stack.
 */
void MediaSessionPrivate::selectIncomingIpVersion () {
	if (linphone_core_ipv6_enabled(core)) {
		if (destProxy && destProxy->op)
			af = destProxy->op->get_address_family();
		else
			af = op->get_address_family();
	} else
		af = AF_INET;
}

/*
 * Choose IP version we are going to use for RTP streams IP address advertised in SDP.
 * The algorithm is as follows:
 * - if ipv6 is disabled at the core level, it is always AF_INET
 * - Otherwise, if the destination address for the call is an IPv6 address, use IPv6.
 * - Otherwise, if the call is done through a known proxy config, then use the information obtained during REGISTER
 * to know if IPv6 is supported by the server.
**/
void MediaSessionPrivate::selectOutgoingIpVersion () {
	if (!linphone_core_ipv6_enabled(core)) {
		af = AF_INET;
		return;
	}

	const LinphoneAddress *to = linphone_call_log_get_to_address(log);
	if (sal_address_is_ipv6(L_GET_PRIVATE_FROM_C_OBJECT(to)->getInternalAddress()))
		af = AF_INET6;
	else if (destProxy && destProxy->op)
		af = destProxy->op->get_address_family();
	else {
		char ipv4[LINPHONE_IPADDR_SIZE];
		char ipv6[LINPHONE_IPADDR_SIZE];
		bool haveIpv6 = false;
		bool haveIpv4 = false;
		/* Check connectivity for IPv4 and IPv6 */
		if (linphone_core_get_local_ip_for(AF_INET6, NULL, ipv6) == 0)
			haveIpv6 = true;
		if (linphone_core_get_local_ip_for(AF_INET, NULL, ipv4) == 0)
			haveIpv4 = true;
		if (haveIpv6) {
			if (!haveIpv4)
				af = AF_INET6;
			else if (lp_config_get_int(linphone_core_get_config(core), "rtp", "prefer_ipv6", 1)) /* This property tells whether ipv6 is prefered if two versions are available */
				af = AF_INET6;
			else
				af = AF_INET;
		} else
			af = AF_INET;
		/* Fill the media_localip default value since we have it here */
		mediaLocalIp = (af == AF_INET6) ? ipv6 : ipv4;
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::forceStreamsDirAccordingToState (SalMediaDescription *md) {
	L_Q();
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		SalStreamDescription *sd = &md->streams[i];
		switch (state) {
			case LinphoneCallPausing:
			case LinphoneCallPaused:
				if (sd->dir != SalStreamInactive) {
					sd->dir = SalStreamSendOnly;
					if ((sd->type == SalVideo) && lp_config_get_int(linphone_core_get_config(core), "sip", "inactive_video_on_pause", 0))
						sd->dir = SalStreamInactive;
				}
				break;
			default:
				break;
		}
		/* Reflect the stream directions in the call params */
		if (i == mainAudioStreamIndex)
			q->getCurrentParams()->setAudioDirection(MediaSessionParamsPrivate::salStreamDirToMediaDirection(sd->dir));
		else if (i == mainVideoStreamIndex)
			q->getCurrentParams()->setVideoDirection(MediaSessionParamsPrivate::salStreamDirToMediaDirection(sd->dir));
	}
}

bool MediaSessionPrivate::generateB64CryptoKey (size_t keyLength, char *keyOut, size_t keyOutSize) {
	uint8_t *tmp = (uint8_t *)ms_malloc0(keyLength);
	if (!sal_get_random_bytes(tmp, keyLength)) {
		lError() << "Failed to generate random key";
		ms_free(tmp);
		return false;
	}
	size_t b64Size = b64::b64_encode((const char *)tmp, keyLength, nullptr, 0);
	if (b64Size == 0) {
		lError() << "Failed to get b64 result size";
		ms_free(tmp);
		return false;
	}
	if (b64Size >= keyOutSize) {
		lError() << "Insufficient room for writing base64 SRTP key";
		ms_free(tmp);
		return false;
	}
	b64Size = b64::b64_encode((const char *)tmp, keyLength, keyOut, keyOutSize);
	if (b64Size == 0) {
		lError() << "Failed to b64 encode key";
		ms_free(tmp);
		return false;
	}
	keyOut[b64Size] = '\0';
	ms_free(tmp);
	return true;
}

void MediaSessionPrivate::makeLocalMediaDescription () {
	L_Q();
	int maxIndex = 0;
	bool rtcpMux = !!lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_mux", 0);
	SalMediaDescription *md = sal_media_description_new();
	SalMediaDescription *oldMd = localDesc;

	/* Multicast is only set in case of outgoing call */
	if (direction == LinphoneCallOutgoing) {
		if (params->audioMulticastEnabled()) {
			md->streams[mainAudioStreamIndex].ttl = linphone_core_get_audio_multicast_ttl(core);
			md->streams[mainAudioStreamIndex].multicast_role = SalMulticastSender;
		}
		if (params->videoMulticastEnabled()) {
			md->streams[mainVideoStreamIndex].ttl = linphone_core_get_video_multicast_ttl(core);
			md->streams[mainVideoStreamIndex].multicast_role = SalMulticastSender;
		}
	}

	params->getPrivate()->adaptToNetwork(core, pingTime);

	string subject = q->getParams()->getSessionName();
	if (!subject.empty())
		strncpy(md->name, subject.c_str(), sizeof(md->name));
	md->session_id = (oldMd ? oldMd->session_id : (rand() & 0xfff));
	md->session_ver = (oldMd ? (oldMd->session_ver + 1) : (rand() & 0xfff));
	md->nb_streams = (biggestDesc ? biggestDesc->nb_streams : 1);

	/* Re-check local ip address each time we make a new offer, because it may change in case of network reconnection */
	{
		LinphoneAddress *address = (direction == LinphoneCallOutgoing ? log->to : log->from);
		getLocalIp(*L_GET_CPP_PTR_FROM_C_OBJECT(address));
	}

	strncpy(md->addr, mediaLocalIp.c_str(), sizeof(md->addr));
	LinphoneAddress *addr = nullptr;
	if (destProxy) {
		addr = linphone_address_clone(linphone_proxy_config_get_identity_address(destProxy));
	} else {
		addr = linphone_address_new(linphone_core_get_identity(core));
	}
	if (linphone_address_get_username(addr)) /* Might be null in case of identity without userinfo */
		strncpy(md->username, linphone_address_get_username(addr), sizeof(md->username));
	linphone_address_unref(addr);

	int bandwidth = params->getPrivate()->getDownBandwidth();
	if (bandwidth)
		md->bandwidth = bandwidth;
	else
		md->bandwidth = linphone_core_get_download_bandwidth(core);

	SalCustomSdpAttribute *customSdpAttributes = params->getPrivate()->getCustomSdpAttributes();
	if (customSdpAttributes)
		md->custom_sdp_attributes = sal_custom_sdp_attribute_clone(customSdpAttributes);

	PayloadTypeHandler pth(core);

	bctbx_list_t *l = pth.makeCodecsList(SalAudio, params->getAudioBandwidthLimit(), -1,
		oldMd ? oldMd->streams[mainAudioStreamIndex].already_assigned_payloads : nullptr);
	if (l && params->audioEnabled()) {
		strncpy(md->streams[mainAudioStreamIndex].rtp_addr, getPublicIpForStream(mainAudioStreamIndex).c_str(), sizeof(md->streams[mainAudioStreamIndex].rtp_addr));
		strncpy(md->streams[mainAudioStreamIndex].rtcp_addr, getPublicIpForStream(mainAudioStreamIndex).c_str(), sizeof(md->streams[mainAudioStreamIndex].rtcp_addr));
		strncpy(md->streams[mainAudioStreamIndex].name, "Audio", sizeof(md->streams[mainAudioStreamIndex].name) - 1);
		md->streams[mainAudioStreamIndex].rtp_port = mediaPorts[mainAudioStreamIndex].rtpPort;
		md->streams[mainAudioStreamIndex].rtcp_port = mediaPorts[mainAudioStreamIndex].rtcpPort;
		md->streams[mainAudioStreamIndex].proto = params->getMediaProto();
		md->streams[mainAudioStreamIndex].dir = params->getPrivate()->getSalAudioDirection();
		md->streams[mainAudioStreamIndex].type = SalAudio;
		md->streams[mainAudioStreamIndex].rtcp_mux = rtcpMux;
		int downPtime = params->getPrivate()->getDownPtime();
		if (downPtime)
			md->streams[mainAudioStreamIndex].ptime = downPtime;
		else
			md->streams[mainAudioStreamIndex].ptime = linphone_core_get_download_ptime(core);
		md->streams[mainAudioStreamIndex].max_rate = pth.getMaxCodecSampleRate(l);
		md->streams[mainAudioStreamIndex].payloads = l;
		if (audioStream && audioStream->ms.sessions.rtp_session) {
			md->streams[mainAudioStreamIndex].rtp_ssrc = rtp_session_get_send_ssrc(audioStream->ms.sessions.rtp_session);
			strncpy(md->streams[mainAudioStreamIndex].rtcp_cname, conference.getMe()->getAddress().asString().c_str(), sizeof(md->streams[mainAudioStreamIndex].rtcp_cname));
		}
		else
			lWarning() << "Cannot get audio local ssrc for CallSession [" << q << "]";
		if (mainAudioStreamIndex > maxIndex)
			maxIndex = mainAudioStreamIndex;
	} else {
		lInfo() << "Don't put audio stream on local offer for CallSession [" << q << "]";
		md->streams[mainAudioStreamIndex].dir = SalStreamInactive;
		if(l)
			l = bctbx_list_free_with_data(l, (bctbx_list_free_func)payload_type_destroy);
	}
	SalCustomSdpAttribute *sdpMediaAttributes = params->getPrivate()->getCustomSdpMediaAttributes(LinphoneStreamTypeAudio);
	if (sdpMediaAttributes)
		md->streams[mainAudioStreamIndex].custom_sdp_attributes = sal_custom_sdp_attribute_clone(sdpMediaAttributes);

	md->streams[mainVideoStreamIndex].proto = md->streams[mainAudioStreamIndex].proto;
	md->streams[mainVideoStreamIndex].dir = params->getPrivate()->getSalVideoDirection();
	md->streams[mainVideoStreamIndex].type = SalVideo;
	md->streams[mainVideoStreamIndex].rtcp_mux = rtcpMux;
	strncpy(md->streams[mainVideoStreamIndex].name, "Video", sizeof(md->streams[mainVideoStreamIndex].name) - 1);

	l = pth.makeCodecsList(SalVideo, 0, -1,
		oldMd ? oldMd->streams[mainVideoStreamIndex].already_assigned_payloads : nullptr);
	if (l && params->videoEnabled()){
		strncpy(md->streams[mainVideoStreamIndex].rtp_addr, getPublicIpForStream(mainVideoStreamIndex).c_str(), sizeof(md->streams[mainVideoStreamIndex].rtp_addr));
		strncpy(md->streams[mainVideoStreamIndex].rtcp_addr, getPublicIpForStream(mainVideoStreamIndex).c_str(), sizeof(md->streams[mainVideoStreamIndex].rtcp_addr));
		md->streams[mainVideoStreamIndex].rtp_port = mediaPorts[mainVideoStreamIndex].rtpPort;
		md->streams[mainVideoStreamIndex].rtcp_port = mediaPorts[mainVideoStreamIndex].rtcpPort;
		md->streams[mainVideoStreamIndex].payloads = l;
		if (videoStream && videoStream->ms.sessions.rtp_session) {
			md->streams[mainVideoStreamIndex].rtp_ssrc = rtp_session_get_send_ssrc(videoStream->ms.sessions.rtp_session);
			strncpy(md->streams[mainVideoStreamIndex].rtcp_cname, conference.getMe()->getAddress().asString().c_str(), sizeof(md->streams[mainVideoStreamIndex].rtcp_cname));
		} else
			lWarning() << "Cannot get video local ssrc for CallSession [" << q << "]";
		if (mainVideoStreamIndex > maxIndex)
			maxIndex = mainVideoStreamIndex;
	} else {
		lInfo() << "Don't put video stream on local offer for CallSession [" << q << "]";
		md->streams[mainVideoStreamIndex].dir = SalStreamInactive;
		if(l)
			l = bctbx_list_free_with_data(l, (bctbx_list_free_func)payload_type_destroy);
	}
	sdpMediaAttributes = params->getPrivate()->getCustomSdpMediaAttributes(LinphoneStreamTypeVideo);
	if (sdpMediaAttributes)
		md->streams[mainVideoStreamIndex].custom_sdp_attributes = sal_custom_sdp_attribute_clone(sdpMediaAttributes);

	md->streams[mainTextStreamIndex].proto = md->streams[mainAudioStreamIndex].proto;
	md->streams[mainTextStreamIndex].dir = SalStreamSendRecv;
	md->streams[mainTextStreamIndex].type = SalText;
	md->streams[mainTextStreamIndex].rtcp_mux = rtcpMux;
	strncpy(md->streams[mainTextStreamIndex].name, "Text", sizeof(md->streams[mainTextStreamIndex].name) - 1);
	if (params->realtimeTextEnabled()) {
		strncpy(md->streams[mainTextStreamIndex].rtp_addr, getPublicIpForStream(mainTextStreamIndex).c_str(), sizeof(md->streams[mainTextStreamIndex].rtp_addr));
		strncpy(md->streams[mainTextStreamIndex].rtcp_addr, getPublicIpForStream(mainTextStreamIndex).c_str(), sizeof(md->streams[mainTextStreamIndex].rtcp_addr));

		md->streams[mainTextStreamIndex].rtp_port = mediaPorts[mainTextStreamIndex].rtpPort;
		md->streams[mainTextStreamIndex].rtcp_port = mediaPorts[mainTextStreamIndex].rtcpPort;

		l = pth.makeCodecsList(SalText, 0, -1,
			oldMd ? oldMd->streams[mainTextStreamIndex].already_assigned_payloads : nullptr);
		md->streams[mainTextStreamIndex].payloads = l;
		if (textStream && textStream->ms.sessions.rtp_session) {
			md->streams[mainTextStreamIndex].rtp_ssrc = rtp_session_get_send_ssrc(textStream->ms.sessions.rtp_session);
			strncpy(md->streams[mainTextStreamIndex].rtcp_cname, conference.getMe()->getAddress().asString().c_str(), sizeof(md->streams[mainTextStreamIndex].rtcp_cname));
		} else
			lWarning() << "Cannot get text local ssrc for CallSession [" << q << "]";
		if (mainTextStreamIndex > maxIndex)
			maxIndex = mainTextStreamIndex;
	} else {
		lInfo() << "Don't put text stream on local offer for CallSession [" << q << "]";
		md->streams[mainTextStreamIndex].dir = SalStreamInactive;
	}
	sdpMediaAttributes = params->getPrivate()->getCustomSdpMediaAttributes(LinphoneStreamTypeText);
	if (sdpMediaAttributes)
		md->streams[mainTextStreamIndex].custom_sdp_attributes = sal_custom_sdp_attribute_clone(sdpMediaAttributes);

	md->nb_streams = MAX(md->nb_streams, maxIndex + 1);

	/* Deactivate unused streams */
	for (int i = md->nb_streams; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (md->streams[i].rtp_port == 0) {
			md->streams[i].dir = SalStreamInactive;
			if (biggestDesc && (i < biggestDesc->nb_streams)) {
				md->streams[i].proto = biggestDesc->streams[i].proto;
				md->streams[i].type = biggestDesc->streams[i].type;
			}
		}
	}
	setupEncryptionKeys(md);
	setupDtlsKeys(md);
	setupZrtpHash(md);
	setupRtcpFb(md);
	setupRtcpXr(md);
	if (stunClient)
		stunClient->updateMediaDescription(md);
	localDesc = md;
	updateLocalMediaDescriptionFromIce();
	if (oldMd) {
		transferAlreadyAssignedPayloadTypes(oldMd, md);
		localDescChanged = sal_media_description_equals(md, oldMd);
		sal_media_description_unref(oldMd);
		if (params->getPrivate()->getInternalCallUpdate()) {
			/*
			 * An internal call update (ICE reINVITE) is not expected to modify the actual media stream parameters.
			 * However, the localDesc may change between first INVITE and ICE reINVITE, for example if the remote party has declined a video stream.
			 * We use the internalCallUpdate flag to prevent trigger an unnecessary media restart.
			 */
			localDescChanged = 0;
		}
	}
	forceStreamsDirAccordingToState(md);
}

void MediaSessionPrivate::setupDtlsKeys (SalMediaDescription *md) {
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i]))
			continue;
		/* If media encryption is set to DTLS check presence of fingerprint in the call which shall have been set at stream init
		 * but it may have failed when retrieving certificate resulting in no fingerprint present and then DTLS not usable */
		if (sal_stream_description_has_dtls(&md->streams[i])) {
			/* Get the self fingerprint from call (it's computed at stream init) */
			strncpy(md->streams[i].dtls_fingerprint, dtlsCertificateFingerprint.c_str(), sizeof(md->streams[i].dtls_fingerprint));
			/* If we are offering, SDP will have actpass setup attribute when role is unset, if we are responding the result mediadescription will be set to SalDtlsRoleIsClient */
			md->streams[i].dtls_role = SalDtlsRoleUnset;
		} else {
			md->streams[i].dtls_fingerprint[0] = '\0';
			md->streams[i].dtls_role = SalDtlsRoleInvalid;
		}
	}
}

int MediaSessionPrivate::setupEncryptionKey (SalSrtpCryptoAlgo *crypto, MSCryptoSuite suite, unsigned int tag) {
	crypto->tag = tag;
	crypto->algo = suite;
	size_t keylen = 0;
	switch (suite) {
		case MS_AES_128_SHA1_80:
		case MS_AES_128_SHA1_32:
		case MS_AES_128_NO_AUTH:
		case MS_NO_CIPHER_SHA1_80: /* Not sure for this one */
			keylen = 30;
			break;
		case MS_AES_256_SHA1_80:
		case MS_AES_CM_256_SHA1_80:
		case MS_AES_256_SHA1_32:
			keylen = 46;
			break;
		case MS_CRYPTO_SUITE_INVALID:
			break;
	}
	if ((keylen == 0) || !generateB64CryptoKey(keylen, crypto->master_key, SAL_SRTP_KEY_SIZE)) {
		lError() << "Could not generate SRTP key";
		crypto->algo = MS_CRYPTO_SUITE_INVALID;
		return -1;
	}
	return 0;
}

void MediaSessionPrivate::setupRtcpFb (SalMediaDescription *md) {
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i]))
			continue;
		md->streams[i].rtcp_fb.generic_nack_enabled = !!lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_fb_generic_nack_enabled", 0);
		md->streams[i].rtcp_fb.tmmbr_enabled = !!lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_fb_tmmbr_enabled", 1);
		md->streams[i].implicit_rtcp_fb = params->getPrivate()->implicitRtcpFbEnabled();
		for (const bctbx_list_t *it = md->streams[i].payloads; it != nullptr; it = bctbx_list_next(it)) {
			OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(it));
			PayloadTypeAvpfParams avpf_params;
			if (!params->avpfEnabled() && !params->getPrivate()->implicitRtcpFbEnabled()) {
				payload_type_unset_flag(pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
				memset(&avpf_params, 0, sizeof(avpf_params));
			} else {
				payload_type_set_flag(pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
				avpf_params = payload_type_get_avpf_params(pt);
				avpf_params.trr_interval = params->getAvpfRrInterval();
			}
			payload_type_set_avpf_params(pt, avpf_params);
		}
	}
}

void MediaSessionPrivate::setupRtcpXr (SalMediaDescription *md) {
	md->rtcp_xr.enabled = !!lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_xr_enabled", 1);
	if (md->rtcp_xr.enabled) {
		const char *rcvr_rtt_mode = lp_config_get_string(linphone_core_get_config(core), "rtp", "rtcp_xr_rcvr_rtt_mode", "all");
		if (strcasecmp(rcvr_rtt_mode, "all") == 0)
			md->rtcp_xr.rcvr_rtt_mode = OrtpRtcpXrRcvrRttAll;
		else if (strcasecmp(rcvr_rtt_mode, "sender") == 0)
			md->rtcp_xr.rcvr_rtt_mode = OrtpRtcpXrRcvrRttSender;
		else
			md->rtcp_xr.rcvr_rtt_mode = OrtpRtcpXrRcvrRttNone;
		if (md->rtcp_xr.rcvr_rtt_mode != OrtpRtcpXrRcvrRttNone)
			md->rtcp_xr.rcvr_rtt_max_size = lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_xr_rcvr_rtt_max_size", 10000);
		md->rtcp_xr.stat_summary_enabled = !!lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_xr_stat_summary_enabled", 1);
		if (md->rtcp_xr.stat_summary_enabled)
			md->rtcp_xr.stat_summary_flags = OrtpRtcpXrStatSummaryLoss | OrtpRtcpXrStatSummaryDup | OrtpRtcpXrStatSummaryJitt | OrtpRtcpXrStatSummaryTTL;
		md->rtcp_xr.voip_metrics_enabled = !!lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_xr_voip_metrics_enabled", 1);
	}
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i]))
			continue;
		memcpy(&md->streams[i].rtcp_xr, &md->rtcp_xr, sizeof(md->streams[i].rtcp_xr));
	}
}

void MediaSessionPrivate::setupZrtpHash (SalMediaDescription *md) {
	if (linphone_core_media_encryption_supported(core, LinphoneMediaEncryptionZRTP)) {
		/* Set the hello hash for all streams */
		for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
			if (!sal_stream_description_active(&md->streams[i]))
				continue;
			if (sessions[i].zrtp_context) {
				ms_zrtp_getHelloHash(sessions[i].zrtp_context, md->streams[i].zrtphash, 128);
				/* Turn on the flag to use it if ZRTP is set */
				md->streams[i].haveZrtpHash = (params->getMediaEncryption() == LinphoneMediaEncryptionZRTP);
			} else
				md->streams[i].haveZrtpHash = 0;
		}
	}
}

void MediaSessionPrivate::setupEncryptionKeys (SalMediaDescription *md) {
	SalMediaDescription *oldMd = localDesc;
	bool keepSrtpKeys = !!lp_config_get_int(linphone_core_get_config(core), "sip", "keep_srtp_keys", 1);
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i]))
			continue;
		if (sal_stream_description_has_srtp(&md->streams[i])) {
			if (keepSrtpKeys && oldMd && sal_stream_description_active(&oldMd->streams[i]) && sal_stream_description_has_srtp(&oldMd->streams[i])) {
				lInfo() << "Keeping same crypto keys";
				for (int j = 0; j < SAL_CRYPTO_ALGO_MAX; j++) {
					memcpy(&md->streams[i].crypto[j], &oldMd->streams[i].crypto[j], sizeof(SalSrtpCryptoAlgo));
				}
			} else {
				const MSCryptoSuite *suites = linphone_core_get_srtp_crypto_suites(core);
				for (int j = 0; (suites != nullptr) && (suites[j] != MS_CRYPTO_SUITE_INVALID) && (j < SAL_CRYPTO_ALGO_MAX); j++) {
					setupEncryptionKey(&md->streams[i].crypto[j], suites[j], static_cast<unsigned int>(j) + 1);
				}
			}
		}
	}
}

void MediaSessionPrivate::transferAlreadyAssignedPayloadTypes (SalMediaDescription *oldMd, SalMediaDescription *md) {
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		md->streams[i].already_assigned_payloads = oldMd->streams[i].already_assigned_payloads;
		oldMd->streams[i].already_assigned_payloads = nullptr;
	}
}

void MediaSessionPrivate::updateLocalMediaDescriptionFromIce () {
	iceAgent->updateLocalMediaDescriptionFromIce(localDesc);
	iceAgent->updateIceStateInCallStats();
}

// -----------------------------------------------------------------------------

SalMulticastRole MediaSessionPrivate::getMulticastRole (SalStreamType type) {
	L_Q();
	SalMulticastRole multicastRole = SalMulticastInactive;
	if (op) {
		SalStreamDescription *streamDesc = nullptr;
		SalMediaDescription *remoteDesc = op->get_remote_media_description();
		if (!localDesc && !remoteDesc && (direction == LinphoneCallOutgoing)) {
			/* Well using call dir */
			if (((type == SalAudio) && params->audioMulticastEnabled())
				|| ((type == SalVideo) && params->videoMulticastEnabled()))
				multicastRole = SalMulticastSender;
		} else if (localDesc && (!remoteDesc || op->is_offerer())) {
			streamDesc = sal_media_description_find_best_stream(localDesc, type);
		} else if (!op->is_offerer() && remoteDesc) {
			streamDesc = sal_media_description_find_best_stream(remoteDesc, type);
		}

		if (streamDesc)
			multicastRole = streamDesc->multicast_role;
	}
	lInfo() << "CallSession [" << q << "], stream type [" << sal_stream_type_to_string(type) << "], multicast role is ["
		<< sal_multicast_role_to_string(multicastRole) << "]";
	return multicastRole;
}

void MediaSessionPrivate::joinMulticastGroup (int streamIndex, MediaStream *ms) {
	L_Q();
	if (!mediaPorts[streamIndex].multicastIp.empty())
		media_stream_join_multicast_group(ms, mediaPorts[streamIndex].multicastIp.c_str());
	else
		lError() << "Cannot join multicast group if multicast ip is not set for call [" << q << "]";
}

// -----------------------------------------------------------------------------

int MediaSessionPrivate::findCryptoIndexFromTag (const SalSrtpCryptoAlgo crypto[], unsigned char tag) {
	for (int i = 0; i < SAL_CRYPTO_ALGO_MAX; i++) {
		if (crypto[i].tag == tag)
			return i;
	}
	return -1;
}

void MediaSessionPrivate::setDtlsFingerprint (MSMediaStreamSessions *sessions, const SalStreamDescription *sd, const SalStreamDescription *remote) {
	if (sal_stream_description_has_dtls(sd)) {
		if (sd->dtls_role == SalDtlsRoleInvalid)
			lWarning() << "Unable to start DTLS engine on stream session [" << sessions << "], Dtls role in resulting media description is invalid";
		else { /* If DTLS is available at both end points */
			/* Give the peer certificate fingerprint to dtls context */
			ms_dtls_srtp_set_peer_fingerprint(sessions->dtls_context, remote->dtls_fingerprint);
		}
	}
}

void MediaSessionPrivate::setDtlsFingerprintOnAllStreams () {
	SalMediaDescription *remote = op->get_remote_media_description();
	SalMediaDescription *result = op->get_final_media_description();
	if (!remote || !result) {
		/* This can happen in some tricky cases (early-media without SDP in the 200). In that case, simply skip DTLS code */
		return;
	}
	if (audioStream && (media_stream_get_state(&audioStream->ms) == MSStreamStarted))
		setDtlsFingerprint(&audioStream->ms.sessions, sal_media_description_find_best_stream(result, SalAudio), sal_media_description_find_best_stream(remote, SalAudio));
#if VIDEO_ENABLED
	if (videoStream && (media_stream_get_state(&videoStream->ms) == MSStreamStarted))
		setDtlsFingerprint(&videoStream->ms.sessions, sal_media_description_find_best_stream(result, SalVideo), sal_media_description_find_best_stream(remote, SalVideo));
#endif
	if (textStream && (media_stream_get_state(&textStream->ms) == MSStreamStarted))
		setDtlsFingerprint(&textStream->ms.sessions, sal_media_description_find_best_stream(result, SalText), sal_media_description_find_best_stream(remote, SalText));
}

void MediaSessionPrivate::setupDtlsParams (MediaStream *ms) {
	if (params->getMediaEncryption() == LinphoneMediaEncryptionDTLS) {
		MSDtlsSrtpParams params;
		memset(&params, 0, sizeof(MSDtlsSrtpParams));
		/* TODO : search for a certificate with CNAME=sip uri(retrieved from variable me) or default : linphone-dtls-default-identity */
		/* This will parse the directory to find a matching fingerprint or generate it if not found */
		/* returned string must be freed */
		char *certificate = nullptr;
		char *key = nullptr;
		char *fingerprint = nullptr;
		sal_certificates_chain_parse_directory(&certificate, &key, &fingerprint,
			linphone_core_get_user_certificates_path(core), "linphone-dtls-default-identity", SAL_CERTIFICATE_RAW_FORMAT_PEM, true, true);
		if (fingerprint) {
			dtlsCertificateFingerprint = fingerprint;
			ms_free(fingerprint);
		}
		if (key && certificate) {
			params.pem_certificate = certificate;
			params.pem_pkey = key;
			params.role = MSDtlsSrtpRoleUnset; /* Default is unset, then check if we have a result SalMediaDescription */
			media_stream_enable_dtls(ms, &params);
			ms_free(certificate);
			ms_free(key);
		} else {
			lError() << "Unable to retrieve or generate DTLS certificate and key - DTLS disabled";
			/* TODO : check if encryption forced, if yes, stop call */
		}
	}
}

void MediaSessionPrivate::setZrtpCryptoTypesParameters (MSZrtpParams *params) {
	if (!params)
		return;

	const MSCryptoSuite *srtpSuites = linphone_core_get_srtp_crypto_suites(core);
	if (srtpSuites) {
		for(int i = 0; (srtpSuites[i] != MS_CRYPTO_SUITE_INVALID) && (i < SAL_CRYPTO_ALGO_MAX) && (i < MS_MAX_ZRTP_CRYPTO_TYPES); i++) {
			switch (srtpSuites[i]) {
				case MS_AES_128_SHA1_32:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES1;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS32;
					break;
				case MS_AES_128_NO_AUTH:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES1;
					break;
				case MS_NO_CIPHER_SHA1_80:
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS80;
					break;
				case MS_AES_128_SHA1_80:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES1;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS80;
					break;
				case MS_AES_CM_256_SHA1_80:
					lWarning() << "Deprecated crypto suite MS_AES_CM_256_SHA1_80, use MS_AES_256_SHA1_80 instead";
					BCTBX_NO_BREAK;
				case MS_AES_256_SHA1_80:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES3;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS80;
					break;
				case MS_AES_256_SHA1_32:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES3;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS32;
					break;
				case MS_CRYPTO_SUITE_INVALID:
					break;
			}
		}
	}

	/* linphone_core_get_srtp_crypto_suites is used to determine sensible defaults; here each can be overridden */
	MsZrtpCryptoTypesCount ciphersCount = linphone_core_get_zrtp_cipher_suites(core, params->ciphers); /* if not present in config file, params->ciphers is not modified */
	if (ciphersCount != 0) /* Use zrtp_cipher_suites config only when present, keep config from srtp_crypto_suite otherwise */
		params->ciphersCount = ciphersCount;
	params->hashesCount = linphone_core_get_zrtp_hash_suites(core, params->hashes);
	MsZrtpCryptoTypesCount authTagsCount = linphone_core_get_zrtp_auth_suites(core, params->authTags); /* If not present in config file, params->authTags is not modified */
	if (authTagsCount != 0)
		params->authTagsCount = authTagsCount; /* Use zrtp_auth_suites config only when present, keep config from srtp_crypto_suite otherwise */
	params->sasTypesCount = linphone_core_get_zrtp_sas_suites(core, params->sasTypes);
	params->keyAgreementsCount = linphone_core_get_zrtp_key_agreement_suites(core, params->keyAgreements);
}

void MediaSessionPrivate::startDtls (MSMediaStreamSessions *sessions, const SalStreamDescription *sd, const SalStreamDescription *remote) {
	if (sal_stream_description_has_dtls(sd)) {
		if (sd->dtls_role == SalDtlsRoleInvalid)
			lWarning() << "Unable to start DTLS engine on stream session [" << sessions << "], Dtls role in resulting media description is invalid";
		else { /* If DTLS is available at both end points */
			/* Give the peer certificate fingerprint to dtls context */
			ms_dtls_srtp_set_peer_fingerprint(sessions->dtls_context, remote->dtls_fingerprint);
			ms_dtls_srtp_set_role(sessions->dtls_context, (sd->dtls_role == SalDtlsRoleIsClient) ? MSDtlsSrtpRoleIsClient : MSDtlsSrtpRoleIsServer); /* Set the role to client */
			ms_dtls_srtp_start(sessions->dtls_context); /* Then start the engine, it will send the DTLS client Hello */
		}
	}
}

void MediaSessionPrivate::startDtlsOnAllStreams () {
	SalMediaDescription *remote = op->get_remote_media_description();
	SalMediaDescription *result = op->get_final_media_description();
	if (!remote || !result) {
		/* This can happen in some tricky cases (early-media without SDP in the 200). In that case, simply skip DTLS code */
		return;
	}
	if (audioStream && (media_stream_get_state(&audioStream->ms) == MSStreamStarted))
		startDtls(&audioStream->ms.sessions, sal_media_description_find_best_stream(result, SalAudio), sal_media_description_find_best_stream(remote, SalAudio));
#if VIDEO_ENABLED
	if (videoStream && (media_stream_get_state(&videoStream->ms) == MSStreamStarted))
		startDtls(&videoStream->ms.sessions, sal_media_description_find_best_stream(result, SalVideo), sal_media_description_find_best_stream(remote, SalVideo));
#endif
	if (textStream && (media_stream_get_state(&textStream->ms) == MSStreamStarted))
		startDtls(&textStream->ms.sessions, sal_media_description_find_best_stream(result, SalText), sal_media_description_find_best_stream(remote, SalText));
}

void MediaSessionPrivate::updateCryptoParameters (SalMediaDescription *oldMd, SalMediaDescription *newMd) {
	const SalStreamDescription *localStreamDesc = sal_media_description_find_secure_stream_of_type(localDesc, SalAudio);
	SalStreamDescription *oldStream = sal_media_description_find_secure_stream_of_type(oldMd, SalAudio);
	SalStreamDescription *newStream = sal_media_description_find_secure_stream_of_type(newMd, SalAudio);
	if (audioStream && localStreamDesc && oldStream && newStream)
		updateStreamCryptoParameters(localStreamDesc, oldStream, newStream, &audioStream->ms);
#ifdef VIDEO_ENABLED
	localStreamDesc = sal_media_description_find_secure_stream_of_type(localDesc, SalVideo);
	oldStream = sal_media_description_find_secure_stream_of_type(oldMd, SalVideo);
	newStream = sal_media_description_find_secure_stream_of_type(newMd, SalVideo);
	if (videoStream && localStreamDesc && oldStream && newStream)
		updateStreamCryptoParameters(localStreamDesc, oldStream, newStream, &videoStream->ms);
#endif
	localStreamDesc = sal_media_description_find_secure_stream_of_type(localDesc, SalText);
	oldStream = sal_media_description_find_secure_stream_of_type(oldMd, SalText);
	newStream = sal_media_description_find_secure_stream_of_type(newMd, SalText);
	if (textStream && localStreamDesc && oldStream && newStream)
		updateStreamCryptoParameters(localStreamDesc, oldStream, newStream, &textStream->ms);
	startDtlsOnAllStreams();
}

bool MediaSessionPrivate::updateStreamCryptoParameters (const SalStreamDescription *localStreamDesc, SalStreamDescription *oldStream, SalStreamDescription *newStream, MediaStream *ms) {
	int cryptoIdx = findCryptoIndexFromTag(localStreamDesc->crypto, static_cast<unsigned char>(newStream->crypto_local_tag));
	if (cryptoIdx >= 0) {
		if (localDescChanged & SAL_MEDIA_DESCRIPTION_CRYPTO_KEYS_CHANGED)
			ms_media_stream_sessions_set_srtp_send_key_b64(&ms->sessions, newStream->crypto[0].algo, localStreamDesc->crypto[cryptoIdx].master_key);
		if (strcmp(oldStream->crypto[0].master_key, newStream->crypto[0].master_key) != 0)
			ms_media_stream_sessions_set_srtp_recv_key_b64(&ms->sessions, newStream->crypto[0].algo, newStream->crypto[0].master_key);
		return true;
	} else
		lWarning() << "Failed to find local crypto algo with tag: " << newStream->crypto_local_tag;
	return false;
}

// -----------------------------------------------------------------------------

int MediaSessionPrivate::getIdealAudioBandwidth (const SalMediaDescription *md, const SalStreamDescription *desc) {
	int remoteBandwidth = 0;
	if (desc->bandwidth > 0)
		remoteBandwidth = desc->bandwidth;
	else if (md->bandwidth > 0) {
		/* Case where b=AS is given globally, not per stream */
		remoteBandwidth = md->bandwidth;
	}
	int uploadBandwidth = 0;
	bool forced = false;
	if (params->getPrivate()->getUpBandwidth() > 0) {
		forced = true;
		uploadBandwidth = params->getPrivate()->getUpBandwidth();
	} else
		uploadBandwidth = linphone_core_get_upload_bandwidth(core);
	uploadBandwidth = PayloadTypeHandler::getMinBandwidth(uploadBandwidth, remoteBandwidth);
	if (!linphone_core_media_description_contains_video_stream(md) || forced)
		return uploadBandwidth;
	if (PayloadTypeHandler::bandwidthIsGreater(uploadBandwidth, 512))
		uploadBandwidth = 100;
	else if (PayloadTypeHandler::bandwidthIsGreater(uploadBandwidth, 256))
		uploadBandwidth = 64;
	else if (PayloadTypeHandler::bandwidthIsGreater(uploadBandwidth, 128))
		uploadBandwidth = 40;
	else if (PayloadTypeHandler::bandwidthIsGreater(uploadBandwidth, 0))
		uploadBandwidth = 24;
	return uploadBandwidth;
}

int MediaSessionPrivate::getVideoBandwidth (const SalMediaDescription *md, const SalStreamDescription *desc) {
	int remoteBandwidth = 0;
	if (desc->bandwidth > 0)
		remoteBandwidth = desc->bandwidth;
	else if (md->bandwidth > 0) {
		/* Case where b=AS is given globally, not per stream */
		remoteBandwidth = PayloadTypeHandler::getRemainingBandwidthForVideo(md->bandwidth, audioBandwidth);
	} else
		remoteBandwidth = lp_config_get_int(linphone_core_get_config(core), "net", "default_max_bandwidth", 1500);
	return PayloadTypeHandler::getMinBandwidth(PayloadTypeHandler::getRemainingBandwidthForVideo(linphone_core_get_upload_bandwidth(core), audioBandwidth), remoteBandwidth);
}

RtpProfile * MediaSessionPrivate::makeProfile (const SalMediaDescription *md, const SalStreamDescription *desc, int *usedPt) {
	*usedPt = -1;
	int bandwidth = 0;
	if (desc->type == SalAudio)
		bandwidth = getIdealAudioBandwidth(md, desc);
	else if (desc->type == SalVideo)
		bandwidth = getVideoBandwidth(md, desc);

	bool first = true;
	RtpProfile *profile = rtp_profile_new("Call profile");
	for (const bctbx_list_t *elem = desc->payloads; elem != nullptr; elem = bctbx_list_next(elem)) {
		OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(elem));
		/* Make a copy of the payload type, so that we left the ones from the SalStreamDescription unchanged.
		 * If the SalStreamDescription is freed, this will have no impact on the running streams. */
		pt = payload_type_clone(pt);
		int upPtime = 0;
		if ((pt->flags & PAYLOAD_TYPE_FLAG_CAN_SEND) && first) {
			/* First codec in list is the selected one */
			if (desc->type == SalAudio) {
				updateAllocatedAudioBandwidth(pt, bandwidth);
				bandwidth = audioBandwidth;
				upPtime = params->getPrivate()->getUpPtime();
				if (!upPtime)
					upPtime = linphone_core_get_upload_ptime(core);
			}
			first = false;
		}
		if (*usedPt == -1) {
			/* Don't select telephone-event as a payload type */
			if (strcasecmp(pt->mime_type, "telephone-event") != 0)
				*usedPt = payload_type_get_number(pt);
		}
		if (pt->flags & PAYLOAD_TYPE_BITRATE_OVERRIDE) {
			lInfo() << "Payload type [" << pt->mime_type << "/" << pt->clock_rate << "] has explicit bitrate [" << (pt->normal_bitrate / 1000) << "] kbit/s";
			pt->normal_bitrate = PayloadTypeHandler::getMinBandwidth(pt->normal_bitrate, bandwidth * 1000);
		} else
			pt->normal_bitrate = bandwidth * 1000;
		if (desc->ptime > 0)
			upPtime = desc->ptime;
		if (upPtime > 0) {
			ostringstream os;
			os << "ptime=" << upPtime;
			payload_type_append_send_fmtp(pt, os.str().c_str());
		}
		int number = payload_type_get_number(pt);
		if (rtp_profile_get_payload(profile, number))
			lWarning() << "A payload type with number " << number << " already exists in profile!";
		else
			rtp_profile_set_payload(profile, number, pt);
	}
	return profile;
}

void MediaSessionPrivate::unsetRtpProfile (int streamIndex) {
	if (sessions[streamIndex].rtp_session)
		rtp_session_set_profile(sessions[streamIndex].rtp_session, &av_profile);
}

void MediaSessionPrivate::updateAllocatedAudioBandwidth (const PayloadType *pt, int maxbw) {
	L_Q();
	audioBandwidth = PayloadTypeHandler::getAudioPayloadTypeBandwidth(pt, maxbw);
	lInfo() << "Audio bandwidth for CallSession [" << q << "] is " << audioBandwidth;
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::applyJitterBufferParams (RtpSession *session, LinphoneStreamType type) {
	LinphoneConfig *config = linphone_core_get_config(core);
	JBParameters params;
	rtp_session_get_jitter_buffer_params(session, &params);
	params.min_size = lp_config_get_int(config, "rtp", "jitter_buffer_min_size", 40);
	params.max_size = lp_config_get_int(config, "rtp", "jitter_buffer_max_size", 250);
	params.max_packets = params.max_size * 200 / 1000; /* Allow 200 packet per seconds, quite large */
	const char *algo = lp_config_get_string(config, "rtp", "jitter_buffer_algorithm", "rls");
	params.buffer_algorithm = jitterBufferNameToAlgo(algo ? algo : "");
	params.refresh_ms = lp_config_get_int(config, "rtp", "jitter_buffer_refresh_period", 5000);
	params.ramp_refresh_ms = lp_config_get_int(config, "rtp", "jitter_buffer_ramp_refresh_period", 5000);
	params.ramp_step_ms = lp_config_get_int(config, "rtp", "jitter_buffer_ramp_step", 20);
	params.ramp_threshold = lp_config_get_int(config, "rtp", "jitter_buffer_ramp_threshold", 70);

	switch (type) {
		case LinphoneStreamTypeAudio:
		case LinphoneStreamTypeText: /* Let's use the same params for text as for audio */
			params.nom_size = linphone_core_get_audio_jittcomp(core);
			params.adaptive = linphone_core_audio_adaptive_jittcomp_enabled(core);
			break;
		case LinphoneStreamTypeVideo:
			params.nom_size = linphone_core_get_video_jittcomp(core);
			params.adaptive = linphone_core_video_adaptive_jittcomp_enabled(core);
			break;
		case LinphoneStreamTypeUnknown:
			lFatal() << "applyJitterBufferParams: should not happen";
			break;
	}
	params.enabled = params.nom_size > 0;
	if (params.enabled) {
		if (params.min_size > params.nom_size)
			params.min_size = params.nom_size;
		if (params.max_size < params.nom_size)
			params.max_size = params.nom_size;
	}
	rtp_session_set_jitter_buffer_params(session, &params);
}

void MediaSessionPrivate::clearEarlyMediaDestination (MediaStream *ms) {
	RtpSession *session = ms->sessions.rtp_session;
	rtp_session_clear_aux_remote_addr(session);
	/* Restore symmetric rtp if ICE is not used */
	if (!iceAgent->hasSession())
		rtp_session_set_symmetric_rtp(session, linphone_core_symmetric_rtp_enabled(core));
}

void MediaSessionPrivate::clearEarlyMediaDestinations () {
	if (audioStream)
		clearEarlyMediaDestination(&audioStream->ms);
	if (videoStream)
		clearEarlyMediaDestination(&videoStream->ms);
}

void MediaSessionPrivate::configureAdaptiveRateControl (MediaStream *ms, const OrtpPayloadType *pt, bool videoWillBeUsed) {
	L_Q();
	bool enabled = !!linphone_core_adaptive_rate_control_enabled(core);
	if (!enabled) {
		media_stream_enable_adaptive_bitrate_control(ms, false);
		return;
	}
	bool isAdvanced = true;
	string algo = linphone_core_get_adaptive_rate_algorithm(core);
	if (algo == "basic")
		isAdvanced = false;
	else if (algo == "advanced")
		isAdvanced = true;
	if (isAdvanced) {
		/* We can't use media_stream_avpf_enabled() here because the active PayloadType is not set yet in the MediaStream */
		if (!pt || !(pt->flags & PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED)) {
			lWarning() << "CallSession [" << q << "] - advanced adaptive rate control requested but avpf is not activated in this stream. Reverting to basic rate control instead";
			isAdvanced = false;
		} else
			lInfo() << "CallSession [" << q << "] - setting up advanced rate control";
	}
	if (isAdvanced) {
		ms_bandwidth_controller_add_stream(core->bw_controller, ms);
		media_stream_enable_adaptive_bitrate_control(ms, false);
	} else {
		media_stream_set_adaptive_bitrate_algorithm(ms, MSQosAnalyzerAlgorithmSimple);
		if ((ms->type == MSAudio) && videoWillBeUsed) {
			/* If this is an audio stream but video is going to be used, there is no need to perform
			 * basic rate control on the audio stream, just the video stream. */
			enabled = false;
		}
		media_stream_enable_adaptive_bitrate_control(ms, enabled);
	}
}

void MediaSessionPrivate::configureRtpSessionForRtcpFb (const SalStreamDescription *stream) {
	RtpSession *session = nullptr;
	if (stream->type == SalAudio)
		session = audioStream->ms.sessions.rtp_session;
	else if (stream->type == SalVideo)
		session = videoStream->ms.sessions.rtp_session;
	else
		return; /* Do nothing for streams that are not audio or video */
	if (stream->rtcp_fb.generic_nack_enabled)
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_GENERIC_NACK, true);
	else
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_GENERIC_NACK, false);
	if (stream->rtcp_fb.tmmbr_enabled)
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_TMMBR, true);
	else
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_TMMBR, false);
}

void MediaSessionPrivate::configureRtpSessionForRtcpXr (SalStreamType type) {
	SalMediaDescription *remote = op->get_remote_media_description();
	if (!remote)
		return;
	const SalStreamDescription *localStream = sal_media_description_find_best_stream(localDesc, type);
	if (!localStream)
		return;
	const SalStreamDescription *remoteStream = sal_media_description_find_best_stream(remote, type);
	if (!remoteStream)
		return;
	OrtpRtcpXrConfiguration currentConfig;
	const OrtpRtcpXrConfiguration *remoteConfig = &remoteStream->rtcp_xr;
	if (localStream->dir == SalStreamInactive)
		return;
	else if (localStream->dir == SalStreamRecvOnly) {
		/* Use local config for unilateral parameters and remote config for collaborative parameters */
		memcpy(&currentConfig, &localStream->rtcp_xr, sizeof(currentConfig));
		currentConfig.rcvr_rtt_mode = remoteConfig->rcvr_rtt_mode;
		currentConfig.rcvr_rtt_max_size = remoteConfig->rcvr_rtt_max_size;
	} else
		memcpy(&currentConfig, remoteConfig, sizeof(currentConfig));
	RtpSession *session = nullptr;
	if (type == SalAudio) {
		session = audioStream->ms.sessions.rtp_session;
	} else if (type == SalVideo) {
		session = videoStream->ms.sessions.rtp_session;
	} else if (type == SalText) {
		session = textStream->ms.sessions.rtp_session;
	}
	rtp_session_configure_rtcp_xr(session, &currentConfig);
}

RtpSession * MediaSessionPrivate::createAudioRtpIoSession () {
	LinphoneConfig *config = linphone_core_get_config(core);
	const char *rtpmap = lp_config_get_string(config, "sound", "rtp_map", "pcmu/8000/1");
	OrtpPayloadType *pt = rtp_profile_get_payload_from_rtpmap(audioProfile, rtpmap);
	if (!pt)
		return nullptr;
	rtpIoAudioProfile = rtp_profile_new("RTP IO audio profile");
	int ptnum = lp_config_get_int(config, "sound", "rtp_ptnum", 0);
	rtp_profile_set_payload(rtpIoAudioProfile, ptnum, payload_type_clone(pt));
	const char *localIp = lp_config_get_string(config, "sound", "rtp_local_addr", "127.0.0.1");
	int localPort = lp_config_get_int(config, "sound", "rtp_local_port", 17076);
	RtpSession *rtpSession = ms_create_duplex_rtp_session(localIp, localPort, -1, ms_factory_get_mtu(core->factory));
	rtp_session_set_profile(rtpSession, rtpIoAudioProfile);
	const char *remoteIp = lp_config_get_string(config, "sound", "rtp_remote_addr", "127.0.0.1");
	int remotePort = lp_config_get_int(config, "sound", "rtp_remote_port", 17078);
	rtp_session_set_remote_addr_and_port(rtpSession, remoteIp, remotePort, -1);
	rtp_session_enable_rtcp(rtpSession, false);
	rtp_session_set_payload_type(rtpSession, ptnum);
	int jittcomp = lp_config_get_int(config, "sound", "rtp_jittcomp", 0); /* 0 means no jitter buffer */
	rtp_session_set_jitter_compensation(rtpSession, jittcomp);
	rtp_session_enable_jitter_buffer(rtpSession, (jittcomp > 0));
	bool symmetric = !!lp_config_get_int(config, "sound", "rtp_symmetric", 0);
	rtp_session_set_symmetric_rtp(rtpSession, symmetric);
	return rtpSession;
}

RtpSession * MediaSessionPrivate::createVideoRtpIoSession () {
#ifdef VIDEO_ENABLED
	LinphoneConfig *config = linphone_core_get_config(core);
	const char *rtpmap = lp_config_get_string(config, "video", "rtp_map", "vp8/90000/1");
	OrtpPayloadType *pt = rtp_profile_get_payload_from_rtpmap(videoProfile, rtpmap);
	if (!pt)
		return nullptr;
	rtpIoVideoProfile = rtp_profile_new("RTP IO video profile");
	int ptnum = lp_config_get_int(config, "video", "rtp_ptnum", 0);
	rtp_profile_set_payload(rtpIoVideoProfile, ptnum, payload_type_clone(pt));
	const char *localIp = lp_config_get_string(config, "video", "rtp_local_addr", "127.0.0.1");
	int localPort = lp_config_get_int(config, "video", "rtp_local_port", 19076);
	RtpSession *rtpSession = ms_create_duplex_rtp_session(localIp, localPort, -1, ms_factory_get_mtu(core->factory));
	rtp_session_set_profile(rtpSession, rtpIoVideoProfile);
	const char *remoteIp = lp_config_get_string(config, "video", "rtp_remote_addr", "127.0.0.1");
	int remotePort = lp_config_get_int(config, "video", "rtp_remote_port", 19078);
	rtp_session_set_remote_addr_and_port(rtpSession, remoteIp, remotePort, -1);
	rtp_session_enable_rtcp(rtpSession, false);
	rtp_session_set_payload_type(rtpSession, ptnum);
	bool symmetric = lp_config_get_int(config, "video", "rtp_symmetric", 0);
	rtp_session_set_symmetric_rtp(rtpSession, symmetric);
	int jittcomp = lp_config_get_int(config, "video", "rtp_jittcomp", 0); /* 0 means no jitter buffer */
	rtp_session_set_jitter_compensation(rtpSession, jittcomp);
	rtp_session_enable_jitter_buffer(rtpSession, (jittcomp > 0));
	return rtpSession;
#else
	return nullptr;
#endif
}

/*
 * Frees the media resources of the call.
 * This has to be done at the earliest, unlike signaling resources that sometimes need to be kept a bit more longer.
 * It is called by setTerminated() (for termination of calls signaled to the application), or directly by the destructor of the session
 * if it was never notified to the application.
 */
void MediaSessionPrivate::freeResources () {
	stopStreams();
	iceAgent->deleteSession();
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++)
		ms_media_stream_sessions_uninit(&sessions[i]);
	_linphone_call_stats_uninit(audioStats);
	_linphone_call_stats_uninit(videoStats);
	_linphone_call_stats_uninit(textStats);
}

void MediaSessionPrivate::handleIceEvents (OrtpEvent *ev) {
	L_Q();
	OrtpEventType evt = ortp_event_get_type(ev);
	OrtpEventData *evd = ortp_event_get_data(ev);
	if (evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) {
		if (iceAgent->hasCompletedCheckList()) {
			/* At least one ICE session has succeeded, so perform a call update */
			if (iceAgent->isControlling() && q->getCurrentParams()->getPrivate()->getUpdateCallWhenIceCompleted()) {
				MediaSessionParams *newParams = new MediaSessionParams(*params);
				newParams->getPrivate()->setInternalCallUpdate(true);
				q->update(newParams);
			}
			startDtlsOnAllStreams();
		}
		iceAgent->updateIceStateInCallStats();
	} else if (evt == ORTP_EVENT_ICE_GATHERING_FINISHED) {
		if (!evd->info.ice_processing_successful)
			lWarning() << "No STUN answer from [" << linphone_core_get_stun_server(core) << "], continuing without STUN";
		iceAgent->gatheringFinished();
		switch (state) {
			case LinphoneCallUpdating:
				startUpdate();
				break;
			case LinphoneCallUpdatedByRemote:
				startAcceptUpdate(prevState, linphone_call_state_to_string(prevState));
				break;
			case LinphoneCallOutgoingInit:
				stopStreamsForIceGathering();
				if (isReadyForInvite())
					q->startInvite(nullptr, "");
				break;
			case LinphoneCallIdle:
				stopStreamsForIceGathering();
				updateLocalMediaDescriptionFromIce();
				op->set_local_media_description(localDesc);
				deferIncomingNotification = false;
				startIncomingNotification();
				break;
			default:
				break;
		}
	} else if (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) {
		if (state == LinphoneCallUpdatedByRemote) {
			startAcceptUpdate(prevState, linphone_call_state_to_string(prevState));
			iceAgent->updateIceStateInCallStats();
		}
	} else if (evt == ORTP_EVENT_ICE_RESTART_NEEDED) {
		iceAgent->restartSession(IR_Controlling);
		q->update(currentParams);
	}
}

void MediaSessionPrivate::handleStreamEvents (int streamIndex) {
	MediaStream *ms = (streamIndex == mainAudioStreamIndex) ? &audioStream->ms :
		(streamIndex == mainVideoStreamIndex ? &videoStream->ms : &textStream->ms);
	if (ms) {
		/* Ensure there is no dangling ICE check list */
		if (!iceAgent->hasSession())
			media_stream_set_ice_check_list(ms, nullptr);
		switch(ms->type){
			case MSAudio:
				audio_stream_iterate((AudioStream *)ms);
				break;
			case MSVideo:
#ifdef VIDEO_ENABLED
				video_stream_iterate((VideoStream *)ms);
#endif
				break;
			case MSText:
				text_stream_iterate((TextStream *)ms);
				break;
			default:
				lError() << "handleStreamEvents(): unsupported stream type";
				return;
		}
	}
	OrtpEvQueue *evq;
	OrtpEvent *ev;
	/* Yes the event queue has to be taken at each iteration, because ice events may perform operations re-creating the streams */
	while ((evq = getEventQueue(streamIndex)) && (ev = ortp_ev_queue_get(evq))) {
		LinphoneCallStats *stats = nullptr;
		if (streamIndex == mainAudioStreamIndex)
			stats = audioStats;
		else if (streamIndex == mainVideoStreamIndex)
			stats = videoStats;
		else
			stats = textStats;
		/* And yes the MediaStream must be taken at each iteration, because it may have changed due to the handling of events
		 * in this loop*/
		ms = getMediaStream(streamIndex);
		if (ms)
			linphone_call_stats_fill(stats, ms, ev);
		notifyStatsUpdated(streamIndex);
		OrtpEventType evt = ortp_event_get_type(ev);
		OrtpEventData *evd = ortp_event_get_data(ev);
		if (evt == ORTP_EVENT_ZRTP_ENCRYPTION_CHANGED) {
			if (streamIndex == mainAudioStreamIndex)
				audioStreamEncryptionChanged(!!evd->info.zrtp_stream_encrypted);
			else if (streamIndex == mainVideoStreamIndex)
				propagateEncryptionChanged();
		} else if (evt == ORTP_EVENT_ZRTP_SAS_READY) {
			if (streamIndex == mainAudioStreamIndex)
				audioStreamAuthTokenReady(evd->info.zrtp_info.sas, !!evd->info.zrtp_info.verified);
		} else if (evt == ORTP_EVENT_DTLS_ENCRYPTION_CHANGED) {
			if (streamIndex == mainAudioStreamIndex)
				audioStreamEncryptionChanged(!!evd->info.dtls_stream_encrypted);
			else if (streamIndex == mainVideoStreamIndex)
				propagateEncryptionChanged();
		} else if ((evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) || (evt == ORTP_EVENT_ICE_GATHERING_FINISHED)
			|| (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) || (evt == ORTP_EVENT_ICE_RESTART_NEEDED)) {
			if (ms)
				handleIceEvents(ev);
		} else if (evt == ORTP_EVENT_TELEPHONE_EVENT) {
#if 0
			linphone_core_dtmf_received(call, evd->info.telephone_event);
#endif
		} else if (evt == ORTP_EVENT_NEW_VIDEO_BANDWIDTH_ESTIMATION_AVAILABLE) {
			lInfo() << "Video bandwidth estimation is " << (int)(evd->info.video_bandwidth_available / 1000.) << " kbit/s";
			// TODO
		}
		ortp_event_destroy(ev);
	}
}

void MediaSessionPrivate::initializeAudioStream () {
	L_Q();
	if (audioStream)
		return;
	if (!sessions[mainAudioStreamIndex].rtp_session) {
		SalMulticastRole multicastRole = getMulticastRole(SalAudio);
		SalMediaDescription *remoteDesc = nullptr;
		SalStreamDescription *streamDesc = nullptr;
		if (op)
			remoteDesc = op->get_remote_media_description();
		if (remoteDesc)
			streamDesc = sal_media_description_find_best_stream(remoteDesc, SalAudio);

		audioStream = audio_stream_new2(core->factory, getBindIpForStream(mainAudioStreamIndex).c_str(),
			(multicastRole ==  SalMulticastReceiver) ? streamDesc->rtp_port : mediaPorts[mainAudioStreamIndex].rtpPort,
			(multicastRole ==  SalMulticastReceiver) ? 0 /* Disabled for now */ : mediaPorts[mainAudioStreamIndex].rtcpPort);
		if (multicastRole == SalMulticastReceiver)
			joinMulticastGroup(mainAudioStreamIndex, &audioStream->ms);
		rtp_session_enable_network_simulation(audioStream->ms.sessions.rtp_session, &core->net_conf.netsim_params);
		applyJitterBufferParams(audioStream->ms.sessions.rtp_session, LinphoneStreamTypeAudio);
		string userAgent = linphone_core_get_user_agent(core);
		audio_stream_set_rtcp_information(audioStream, conference.getMe()->getAddress().asString().c_str(), userAgent.c_str());
		rtp_session_set_symmetric_rtp(audioStream->ms.sessions.rtp_session, linphone_core_symmetric_rtp_enabled(core));
		setupDtlsParams(&audioStream->ms);

		/* Initialize zrtp even if we didn't explicitely set it, just in case peer offers it */
		if (linphone_core_media_encryption_supported(core, LinphoneMediaEncryptionZRTP)) {
			char *peerUri = linphone_address_as_string_uri_only((direction == LinphoneCallIncoming) ? log->from : log->to);
			char *selfUri = linphone_address_as_string_uri_only((direction == LinphoneCallIncoming) ? log->to : log->from);
			MSZrtpParams params;
			memset(&params, 0, sizeof(MSZrtpParams));
			/* media encryption of current params will be set later when zrtp is activated */
			params.zidCacheDB = linphone_core_get_zrtp_cache_db(core);
			params.peerUri = peerUri;
			params.selfUri = selfUri;
			/* Get key lifespan from config file, default is 0:forever valid */
			params.limeKeyTimeSpan = bctbx_time_string_to_sec(lp_config_get_string(linphone_core_get_config(core), "sip", "lime_key_validity", "0"));
			setZrtpCryptoTypesParameters(&params);
			audio_stream_enable_zrtp(audioStream, &params);
			if (peerUri != NULL) ms_free(peerUri);
			if (selfUri != NULL) ms_free(selfUri);
		}

		media_stream_reclaim_sessions(&audioStream->ms, &sessions[mainAudioStreamIndex]);
	} else {
		audioStream = audio_stream_new_with_sessions(core->factory, &sessions[mainAudioStreamIndex]);
	}

	if (mediaPorts[mainAudioStreamIndex].rtpPort == -1)
		setPortConfigFromRtpSession(mainAudioStreamIndex, audioStream->ms.sessions.rtp_session);
	int dscp = linphone_core_get_audio_dscp(core);
	if (dscp != -1)
		audio_stream_set_dscp(audioStream, dscp);
	if (linphone_core_echo_limiter_enabled(core)) {
		string type = lp_config_get_string(linphone_core_get_config(core), "sound", "el_type", "mic");
		if (type == "mic")
			audio_stream_enable_echo_limiter(audioStream, ELControlMic);
		else if (type == "full")
			audio_stream_enable_echo_limiter(audioStream, ELControlFull);
	}

	/* Equalizer location in the graph: 'mic' = in input graph, otherwise in output graph.
	   Any other value than mic will default to output graph for compatibility */
	string location = lp_config_get_string(linphone_core_get_config(core), "sound", "eq_location", "hp");
	audioStream->eq_loc = (location == "mic") ? MSEqualizerMic : MSEqualizerHP;
	lInfo() << "Equalizer location: " << location;

	audio_stream_enable_gain_control(audioStream, true);
	if (linphone_core_echo_cancellation_enabled(core)) {
		int len = lp_config_get_int(linphone_core_get_config(core), "sound", "ec_tail_len", 0);
		int delay = lp_config_get_int(linphone_core_get_config(core), "sound", "ec_delay", 0);
		int framesize = lp_config_get_int(linphone_core_get_config(core), "sound", "ec_framesize", 0);
		audio_stream_set_echo_canceller_params(audioStream, len, delay, framesize);
		if (audioStream->ec) {
			char *statestr=reinterpret_cast<char *>(ms_malloc0(ecStateMaxLen));
			if (lp_config_relative_file_exists(linphone_core_get_config(core), ecStateStore.c_str())
				&& (lp_config_read_relative_file(linphone_core_get_config(core), ecStateStore.c_str(), statestr, ecStateMaxLen) == 0)) {
				ms_filter_call_method(audioStream->ec, MS_ECHO_CANCELLER_SET_STATE_STRING, statestr);
			}
			ms_free(statestr);
		}
	}
	audio_stream_enable_automatic_gain_control(audioStream, linphone_core_agc_enabled(core));
	bool_t enabled = !!lp_config_get_int(linphone_core_get_config(core), "sound", "noisegate", 0);
	audio_stream_enable_noise_gate(audioStream, enabled);
	audio_stream_set_features(audioStream, linphone_core_get_audio_features(core));

	if (core->rtptf) {
		RtpTransport *meta_rtp;
		RtpTransport *meta_rtcp;
		rtp_session_get_transports(audioStream->ms.sessions.rtp_session, &meta_rtp, &meta_rtcp);
		if (!meta_rtp_transport_get_endpoint(meta_rtp)) {
			lInfo() << "CallSession [" << q << "] using custom audio RTP transport endpoint";
			meta_rtp_transport_set_endpoint(meta_rtp, core->rtptf->audio_rtp_func(core->rtptf->audio_rtp_func_data, mediaPorts[mainAudioStreamIndex].rtpPort));
		}
		if (!meta_rtp_transport_get_endpoint(meta_rtcp))
			meta_rtp_transport_set_endpoint(meta_rtcp, core->rtptf->audio_rtcp_func(core->rtptf->audio_rtcp_func_data, mediaPorts[mainAudioStreamIndex].rtcpPort));
	}

	audioStreamEvQueue = ortp_ev_queue_new();
	rtp_session_register_event_queue(audioStream->ms.sessions.rtp_session, audioStreamEvQueue);
	iceAgent->prepareIceForStream(&audioStream->ms, false);
}

void MediaSessionPrivate::initializeStreams () {
	initializeAudioStream();
	initializeVideoStream();
	initializeTextStream();
}

void MediaSessionPrivate::initializeTextStream () {
	if (textStream)
		return;
	if (!sessions[mainTextStreamIndex].rtp_session) {
		SalMulticastRole multicastRole = getMulticastRole(SalText);
		SalMediaDescription *remoteDesc = nullptr;
		SalStreamDescription *streamDesc = nullptr;
		if (op)
			remoteDesc = op->get_remote_media_description();
		if (remoteDesc)
			streamDesc = sal_media_description_find_best_stream(remoteDesc, SalText);

		textStream = text_stream_new2(core->factory, getBindIpForStream(mainTextStreamIndex).c_str(),
			(multicastRole ==  SalMulticastReceiver) ? streamDesc->rtp_port : mediaPorts[mainTextStreamIndex].rtpPort,
			(multicastRole ==  SalMulticastReceiver) ? 0 /* Disabled for now */ : mediaPorts[mainTextStreamIndex].rtcpPort);
		if (multicastRole == SalMulticastReceiver)
			joinMulticastGroup(mainTextStreamIndex, &textStream->ms);
		rtp_session_enable_network_simulation(textStream->ms.sessions.rtp_session, &core->net_conf.netsim_params);
		applyJitterBufferParams(textStream->ms.sessions.rtp_session, LinphoneStreamTypeText);
		rtp_session_set_symmetric_rtp(textStream->ms.sessions.rtp_session, linphone_core_symmetric_rtp_enabled(core));
		setupDtlsParams(&textStream->ms);
		media_stream_reclaim_sessions(&textStream->ms, &sessions[mainTextStreamIndex]);
	} else
		textStream = text_stream_new_with_sessions(core->factory, &sessions[mainTextStreamIndex]);
	if (mediaPorts[mainTextStreamIndex].rtpPort == -1)
		setPortConfigFromRtpSession(mainTextStreamIndex, textStream->ms.sessions.rtp_session);

	if (core->rtptf) {
		RtpTransport *meta_rtp;
		RtpTransport *meta_rtcp;
		rtp_session_get_transports(textStream->ms.sessions.rtp_session, &meta_rtp, &meta_rtcp);
		if (!meta_rtp_transport_get_endpoint(meta_rtp))
			meta_rtp_transport_set_endpoint(meta_rtp, core->rtptf->audio_rtp_func(core->rtptf->audio_rtp_func_data, mediaPorts[mainTextStreamIndex].rtpPort));
		if (!meta_rtp_transport_get_endpoint(meta_rtcp))
			meta_rtp_transport_set_endpoint(meta_rtcp, core->rtptf->audio_rtcp_func(core->rtptf->audio_rtcp_func_data, mediaPorts[mainTextStreamIndex].rtcpPort));
	}

	textStreamEvQueue = ortp_ev_queue_new();
	rtp_session_register_event_queue(textStream->ms.sessions.rtp_session, textStreamEvQueue);
	iceAgent->prepareIceForStream(&textStream->ms, false);
}

void MediaSessionPrivate::initializeVideoStream () {
#ifdef VIDEO_ENABLED
	L_Q();
	if (videoStream)
		return;
	if (!sessions[mainVideoStreamIndex].rtp_session) {
		SalMulticastRole multicastRole = getMulticastRole(SalVideo);
		SalMediaDescription *remoteDesc = nullptr;
		SalStreamDescription *streamDesc = nullptr;
		if (op)
			remoteDesc = op->get_remote_media_description();
		if (remoteDesc)
			streamDesc = sal_media_description_find_best_stream(remoteDesc, SalVideo);

		videoStream = video_stream_new2(core->factory, getBindIpForStream(mainVideoStreamIndex).c_str(),
			(multicastRole ==  SalMulticastReceiver) ? streamDesc->rtp_port : mediaPorts[mainVideoStreamIndex].rtpPort,
			(multicastRole ==  SalMulticastReceiver) ?  0 /* Disabled for now */ : mediaPorts[mainVideoStreamIndex].rtcpPort);
		if (multicastRole == SalMulticastReceiver)
			joinMulticastGroup(mainVideoStreamIndex, &videoStream->ms);
		rtp_session_enable_network_simulation(videoStream->ms.sessions.rtp_session, &core->net_conf.netsim_params);
		applyJitterBufferParams(videoStream->ms.sessions.rtp_session, LinphoneStreamTypeVideo);
		string userAgent = linphone_core_get_user_agent(core);
		video_stream_set_rtcp_information(videoStream, conference.getMe()->getAddress().asString().c_str(), userAgent.c_str());
		rtp_session_set_symmetric_rtp(videoStream->ms.sessions.rtp_session, linphone_core_symmetric_rtp_enabled(core));
		setupDtlsParams(&videoStream->ms);
		/* Initialize zrtp even if we didn't explicitely set it, just in case peer offers it */
		if (linphone_core_media_encryption_supported(core, LinphoneMediaEncryptionZRTP))
			video_stream_enable_zrtp(videoStream, audioStream);

		media_stream_reclaim_sessions(&videoStream->ms, &sessions[mainVideoStreamIndex]);
	} else
		videoStream = video_stream_new_with_sessions(core->factory, &sessions[mainVideoStreamIndex]);

	if (mediaPorts[mainVideoStreamIndex].rtpPort == -1)
		setPortConfigFromRtpSession(mainVideoStreamIndex, videoStream->ms.sessions.rtp_session);
	int dscp = linphone_core_get_video_dscp(core);
	if (dscp!=-1)
		video_stream_set_dscp(videoStream, dscp);
	video_stream_enable_display_filter_auto_rotate(
		videoStream,
		!!lp_config_get_int(linphone_core_get_config(core), "video", "display_filter_auto_rotate", 0)
	);
	int videoRecvBufSize = lp_config_get_int(linphone_core_get_config(core), "video", "recv_buf_size", 0);
	if (videoRecvBufSize > 0)
		rtp_session_set_recv_buf_size(videoStream->ms.sessions.rtp_session, videoRecvBufSize);

	const char *displayFilter = linphone_core_get_video_display_filter(core);
	if (displayFilter)
		video_stream_set_display_filter_name(videoStream, displayFilter);
	video_stream_set_event_callback(videoStream, videoStreamEventCb, this);

	if (core->rtptf) {
		RtpTransport *meta_rtp;
		RtpTransport *meta_rtcp;
		rtp_session_get_transports(videoStream->ms.sessions.rtp_session, &meta_rtp, &meta_rtcp);
		if (!meta_rtp_transport_get_endpoint(meta_rtp)) {
			lInfo() << "CallSession [" << q << "] using custom video RTP transport endpoint";
			meta_rtp_transport_set_endpoint(meta_rtp, core->rtptf->video_rtp_func(core->rtptf->video_rtp_func_data, mediaPorts[mainVideoStreamIndex].rtpPort));
		}
		if (!meta_rtp_transport_get_endpoint(meta_rtcp))
			meta_rtp_transport_set_endpoint(meta_rtcp, core->rtptf->video_rtcp_func(core->rtptf->video_rtcp_func_data, mediaPorts[mainVideoStreamIndex].rtcpPort));
	}
	videoStreamEvQueue = ortp_ev_queue_new();
	rtp_session_register_event_queue(videoStream->ms.sessions.rtp_session, videoStreamEvQueue);
	iceAgent->prepareIceForStream(&videoStream->ms, false);
#ifdef TEST_EXT_RENDERER
	video_stream_set_render_callback(videoStream, extRendererCb, nullptr);
#endif
#else
	videoStream = nullptr;
#endif
}

void MediaSessionPrivate::parameterizeEqualizer (AudioStream *stream) {
	LinphoneConfig *config = linphone_core_get_config(core);
	const char *eqActive = lp_config_get_string(config, "sound", "eq_active", nullptr);
	if (eqActive)
		lWarning() << "'eq_active' linphonerc parameter has no effect anymore. Please use 'mic_eq_active' or 'spk_eq_active' instead";
	const char *eqGains = lp_config_get_string(config, "sound", "eq_gains", nullptr);
	if(eqGains)
		lWarning() << "'eq_gains' linphonerc parameter has no effect anymore. Please use 'mic_eq_gains' or 'spk_eq_gains' instead";
	if (stream->mic_equalizer) {
		MSFilter *f = stream->mic_equalizer;
		bool enabled = !!lp_config_get_int(config, "sound", "mic_eq_active", 0);
		ms_filter_call_method(f, MS_EQUALIZER_SET_ACTIVE, &enabled);
		const char *gains = lp_config_get_string(config, "sound", "mic_eq_gains", nullptr);
		if (enabled && gains) {
			bctbx_list_t *gainsList = ms_parse_equalizer_string(gains);
			for (bctbx_list_t *it = gainsList; it; it = bctbx_list_next(it)) {
				MSEqualizerGain *g = reinterpret_cast<MSEqualizerGain *>(bctbx_list_get_data(it));
				lInfo() << "Read microphone equalizer gains: " << g->frequency << "(~" << g->width << ") --> " << g->gain;
				ms_filter_call_method(f, MS_EQUALIZER_SET_GAIN, g);
			}
			if (gainsList)
				bctbx_list_free_with_data(gainsList, ms_free);
		}
	}
	if (stream->spk_equalizer) {
		MSFilter *f = stream->spk_equalizer;
		bool enabled = !!lp_config_get_int(config, "sound", "spk_eq_active", 0);
		ms_filter_call_method(f, MS_EQUALIZER_SET_ACTIVE, &enabled);
		const char *gains = lp_config_get_string(config, "sound", "spk_eq_gains", nullptr);
		if (enabled && gains) {
			bctbx_list_t *gainsList = ms_parse_equalizer_string(gains);
			for (bctbx_list_t *it = gainsList; it; it = bctbx_list_next(it)) {
				MSEqualizerGain *g = reinterpret_cast<MSEqualizerGain *>(bctbx_list_get_data(it));
				lInfo() << "Read speaker equalizer gains: " << g->frequency << "(~" << g->width << ") --> " << g->gain;
				ms_filter_call_method(f, MS_EQUALIZER_SET_GAIN, g);
			}
			if (gainsList)
				bctbx_list_free_with_data(gainsList, ms_free);
		}
	}
}

void MediaSessionPrivate::prepareEarlyMediaForking () {
	/* We need to disable symmetric rtp otherwise our outgoing streams will be switching permanently between the multiple destinations */
	if (audioStream)
		rtp_session_set_symmetric_rtp(audioStream->ms.sessions.rtp_session, false);
	if (videoStream)
		rtp_session_set_symmetric_rtp(videoStream->ms.sessions.rtp_session, false);
}

void MediaSessionPrivate::postConfigureAudioStream (AudioStream *stream, bool muted) {
	float micGain = core->sound_conf.soft_mic_lev;
	if (muted)
		audio_stream_set_mic_gain(stream, 0);
	else
		audio_stream_set_mic_gain_db(stream, micGain);
	float recvGain = core->sound_conf.soft_play_lev;
	if (static_cast<int>(recvGain))
		setPlaybackGainDb(stream, recvGain);
	LinphoneConfig *config = linphone_core_get_config(core);
	float ngThres = lp_config_get_float(config, "sound", "ng_thres", 0.05f);
	float ngFloorGain = lp_config_get_float(config, "sound", "ng_floorgain", 0);
	if (stream->volsend) {
		int dcRemoval = lp_config_get_int(config, "sound", "dc_removal", 0);
		ms_filter_call_method(stream->volsend, MS_VOLUME_REMOVE_DC, &dcRemoval);
		float speed = lp_config_get_float(config, "sound", "el_speed", -1);
		float thres = lp_config_get_float(config, "sound", "el_thres", -1);
		float force = lp_config_get_float(config, "sound", "el_force", -1);
		int sustain = lp_config_get_int(config, "sound", "el_sustain", -1);
		float transmitThres = lp_config_get_float(config, "sound", "el_transmit_thres", -1);
		if (static_cast<int>(speed) == -1)
			speed = 0.03f;
		if (static_cast<int>(force) == -1)
			force = 25;
		MSFilter *f = stream->volsend;
		ms_filter_call_method(f, MS_VOLUME_SET_EA_SPEED, &speed);
		ms_filter_call_method(f, MS_VOLUME_SET_EA_FORCE, &force);
		if (static_cast<int>(thres) != -1)
			ms_filter_call_method(f, MS_VOLUME_SET_EA_THRESHOLD, &thres);
		if (static_cast<int>(sustain) != -1)
			ms_filter_call_method(f, MS_VOLUME_SET_EA_SUSTAIN, &sustain);
		if (static_cast<int>(transmitThres) != -1)
			ms_filter_call_method(f, MS_VOLUME_SET_EA_TRANSMIT_THRESHOLD, &transmitThres);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_THRESHOLD, &ngThres);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_FLOORGAIN, &ngFloorGain);
	}
	if (stream->volrecv) {
		/* Parameters for a limited noise-gate effect, using echo limiter threshold */
		float floorGain = (float)(1 / pow(10, micGain / 10));
		int spkAgc = lp_config_get_int(config, "sound", "speaker_agc_enabled", 0);
		MSFilter *f = stream->volrecv;
		ms_filter_call_method(f, MS_VOLUME_ENABLE_AGC, &spkAgc);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_THRESHOLD, &ngThres);
		ms_filter_call_method(f, MS_VOLUME_SET_NOISE_GATE_FLOORGAIN, &floorGain);
	}
	parameterizeEqualizer(stream);
}

void MediaSessionPrivate::postConfigureAudioStreams (bool muted) {
	L_Q();
	postConfigureAudioStream(audioStream, muted);
	if (linphone_core_dtmf_received_has_listener(core))
		audio_stream_play_received_dtmfs(audioStream, false);
	if (recordActive)
		q->startRecording();
}

void MediaSessionPrivate::setPlaybackGainDb (AudioStream *stream, float gain) {
	if (stream->volrecv)
		ms_filter_call_method(stream->volrecv, MS_VOLUME_SET_DB_GAIN, &gain);
	else
		lWarning() << "Could not apply playback gain: gain control wasn't activated";
}

void MediaSessionPrivate::setSymmetricRtp (bool value) {
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		MSMediaStreamSessions *mss = &sessions[i];
		if (mss->rtp_session)
			rtp_session_set_symmetric_rtp(mss->rtp_session, value);
	}
}

void MediaSessionPrivate::startAudioStream (LinphoneCallState targetState, bool videoWillBeUsed) {
	const SalStreamDescription *stream = sal_media_description_find_best_stream(resultDesc, SalAudio);
	if (stream && (stream->dir != SalStreamInactive) && (stream->rtp_port != 0)) {
		int usedPt = -1;
		onHoldFile = "";
		audioProfile = makeProfile(resultDesc, stream, &usedPt);
		if (usedPt == -1)
			lWarning() << "No audio stream accepted?";
		else {
			const char *rtpAddr = (stream->rtp_addr[0] != '\0') ? stream->rtp_addr : resultDesc->addr;
			bool isMulticast = !!ms_is_multicast(rtpAddr);
			bool ok = true;
			currentParams->getPrivate()->setUsedAudioCodec(rtp_profile_get_payload(audioProfile, usedPt));
			currentParams->enableAudio(true);
			MSSndCard *playcard = core->sound_conf.lsd_card ? core->sound_conf.lsd_card : core->sound_conf.play_sndcard;
			if (!playcard)
				lWarning() << "No card defined for playback!";
			MSSndCard *captcard = core->sound_conf.capt_sndcard;
			if (!captcard)
				lWarning() << "No card defined for capture!";
			string playfile = L_C_TO_STRING(core->play_file);
			string recfile = L_C_TO_STRING(core->rec_file);
			/* Don't use file or soundcard capture when placed in recv-only mode */
			if ((stream->rtp_port == 0) || (stream->dir == SalStreamRecvOnly) || ((stream->multicast_role == SalMulticastReceiver) && isMulticast)) {
				captcard = nullptr;
				playfile = "";
			}
			if (targetState == LinphoneCallPaused) {
				/* In paused state, we never use soundcard */
				playcard = captcard = nullptr;
				recfile = "";
				/* And we will eventually play "playfile" if set by the user */
			}
			if (playingRingbackTone) {
				captcard = nullptr;
				playfile = ""; /* It is setup later */
				if (lp_config_get_int(linphone_core_get_config(core), "sound", "send_ringback_without_playback", 0) == 1) {
					playcard = nullptr;
					recfile = "";
				}
			}
			/* If playfile are supplied don't use soundcards */
			bool useRtpIo = !!lp_config_get_int(linphone_core_get_config(core), "sound", "rtp_io", false);
			bool useRtpIoEnableLocalOutput = !!lp_config_get_int(linphone_core_get_config(core), "sound", "rtp_io_enable_local_output", false);
			if (core->use_files || (useRtpIo && !useRtpIoEnableLocalOutput)) {
				captcard = playcard = nullptr;
			}
			if (params->getPrivate()->getInConference()) {
				/* First create the graph without soundcard resources */
				captcard = playcard = nullptr;
			}
#if 0
			if (!linphone_call_sound_resources_available(call)){
				ms_message("Sound resources are used by another call, not using soundcard.");
				captcard = playcard = nullptr;
			}
#endif

			if (playcard) {
				ms_snd_card_set_stream_type(playcard, MS_SND_CARD_STREAM_VOICE);
			}

			bool useEc = captcard && linphone_core_echo_cancellation_enabled(core);
			audio_stream_enable_echo_canceller(audioStream, useEc);
			if (playcard && (stream->max_rate > 0))
				ms_snd_card_set_preferred_sample_rate(playcard, stream->max_rate);
			if (captcard && (stream->max_rate > 0))
				ms_snd_card_set_preferred_sample_rate(captcard, stream->max_rate);
			rtp_session_enable_rtcp_mux(audioStream->ms.sessions.rtp_session, stream->rtcp_mux);
			if (!params->getPrivate()->getInConference() && !params->getRecordFilePath().empty()) {
				audio_stream_mixed_record_open(audioStream, params->getRecordFilePath().c_str());
				currentParams->setRecordFilePath(params->getRecordFilePath());
			}
			/* Valid local tags are > 0 */
			if (sal_stream_description_has_srtp(stream)) {
				const SalStreamDescription *localStreamDesc = sal_media_description_find_stream(localDesc, stream->proto, SalAudio);
				int cryptoIdx = findCryptoIndexFromTag(localStreamDesc->crypto, static_cast<unsigned char>(stream->crypto_local_tag));
				if (cryptoIdx >= 0) {
					ms_media_stream_sessions_set_srtp_recv_key_b64(&audioStream->ms.sessions, stream->crypto[0].algo, stream->crypto[0].master_key);
					ms_media_stream_sessions_set_srtp_send_key_b64(&audioStream->ms.sessions, stream->crypto[0].algo, localStreamDesc->crypto[cryptoIdx].master_key);
				} else
					lWarning() << "Failed to find local crypto algo with tag: " << stream->crypto_local_tag;
			}
			configureRtpSessionForRtcpFb(stream);
			configureRtpSessionForRtcpXr(SalAudio);
			configureAdaptiveRateControl(&audioStream->ms, currentParams->getUsedAudioCodec(), videoWillBeUsed);
			if (isMulticast)
				rtp_session_set_multicast_ttl(audioStream->ms.sessions.rtp_session, stream->ttl);
			MSMediaStreamIO io = MS_MEDIA_STREAM_IO_INITIALIZER;
			if (useRtpIo) {
				if (useRtpIoEnableLocalOutput) {
					io.input.type = MSResourceRtp;
					io.input.session = createAudioRtpIoSession();
					if (playcard) {
						io.output.type = MSResourceSoundcard;
						io.output.soundcard = playcard;
					} else {
						io.output.type = MSResourceFile;
						io.output.file = recfile.c_str();
					}
				} else {
					io.input.type = io.output.type = MSResourceRtp;
					io.input.session = io.output.session = createAudioRtpIoSession();
				}
				if (!io.input.session)
					ok = false;
			} else {
				if (playcard) {
					io.output.type = MSResourceSoundcard;
					io.output.soundcard = playcard;
				} else {
					io.output.type = MSResourceFile;
					io.output.file = recfile.c_str();
				}
				if (captcard) {
					io.input.type = MSResourceSoundcard;
					io.input.soundcard = captcard;
				} else {
					io.input.type = MSResourceFile;
					onHoldFile = playfile;
					io.input.file = nullptr; /* We prefer to use the remote_play api, that allows to play multimedia files */
				}
			}
			if (ok) {
				int err = audio_stream_start_from_io(audioStream, audioProfile, rtpAddr, stream->rtp_port,
					(stream->rtcp_addr[0] != '\0') ? stream->rtcp_addr : resultDesc->addr,
					(linphone_core_rtcp_enabled(core) && !isMulticast) ? (stream->rtcp_port ? stream->rtcp_port : stream->rtp_port + 1) : 0,
					usedPt, &io);
				if (err == 0)
					postConfigureAudioStreams((allMuted || audioMuted) && !playingRingbackTone);
			}
			ms_media_stream_sessions_set_encryption_mandatory(&audioStream->ms.sessions, isEncryptionMandatory());
			if ((targetState == LinphoneCallPaused) && !captcard && !playfile.empty()) {
				int pauseTime = 500;
				ms_filter_call_method(audioStream->soundread, MS_FILE_PLAYER_LOOP, &pauseTime);
			}
#if 0
			if (playingRingbacktone) {
				setup_ring_player(lc,call);
			}
#endif
			if (params->getPrivate()->getInConference() && core->conf_ctx) {
				/* Transform the graph to connect it to the conference filter */
#if 0
				bool mute = (stream->dir == SalStreamRecvOnly);
				linphone_conference_on_call_stream_starting(core->conf_ctx, call, mute);
#endif
			}
			currentParams->getPrivate()->setInConference(params->getPrivate()->getInConference());
			currentParams->enableLowBandwidth(params->lowBandwidthEnabled());
			/* Start ZRTP engine if needed : set here or remote have a zrtp-hash attribute */
			SalMediaDescription *remote = op->get_remote_media_description();
			const SalStreamDescription *remoteStream = sal_media_description_find_best_stream(remote, SalAudio);
			if (linphone_core_media_encryption_supported(core, LinphoneMediaEncryptionZRTP)
				&& ((params->getMediaEncryption() == LinphoneMediaEncryptionZRTP) || (remoteStream->haveZrtpHash == 1))) {
				audio_stream_start_zrtp(audioStream);
				if (remoteStream->haveZrtpHash == 1) {
					int retval = ms_zrtp_setPeerHelloHash(audioStream->ms.sessions.zrtp_context, (uint8_t *)remoteStream->zrtphash, strlen((const char *)(remoteStream->zrtphash)));
					if (retval != 0)
						lError() << "Zrtp hash mismatch 0x" << hex << retval;
				}
			}
		}
	}
}

void MediaSessionPrivate::startStreams (LinphoneCallState targetState) {
	L_Q();
	switch (targetState) {
		case LinphoneCallIncomingEarlyMedia:
			if (linphone_core_get_remote_ringback_tone(core)) {
				playingRingbackTone = true;
			}
			BCTBX_NO_BREAK;
		case LinphoneCallOutgoingEarlyMedia:
			if (!params->earlyMediaSendingEnabled())
				allMuted = true;
			break;
		default:
			playingRingbackTone = false;
			allMuted = false;
			break;
	}

	currentParams->getPrivate()->setUsedAudioCodec(nullptr);
	currentParams->getPrivate()->setUsedVideoCodec(nullptr);
	currentParams->getPrivate()->setUsedRealtimeTextCodec(nullptr);

	if (!audioStream && !videoStream) {
		lFatal() << "startStreams() called without prior init!";
		return;
	}
	if (iceAgent->hasSession()) {
		/* If there is an ICE session when we are about to start streams, then ICE will conduct the media path checking and authentication properly.
		 * Symmetric RTP must be turned off */
		setSymmetricRtp(false);
	}

	nbMediaStarts++;
	bool videoWillBeUsed = false;
#if defined(VIDEO_ENABLED)
	const SalStreamDescription *vstream = sal_media_description_find_best_stream(resultDesc, SalVideo);
	if (vstream && (vstream->dir != SalStreamInactive) && vstream->payloads) {
		/* When video is used, do not make adaptive rate control on audio, it is stupid */
		videoWillBeUsed = true;
	}
#endif
	lInfo() << "startStreams() CallSession=[" << q << "] local upload_bandwidth=[" << linphone_core_get_upload_bandwidth(core)
		<< "] kbit/s; local download_bandwidth=[" << linphone_core_get_download_bandwidth(core) << "] kbit/s";
	currentParams->enableAudio(false);
	if (audioStream)
		startAudioStream(targetState, videoWillBeUsed);
	else
		lWarning() << "startStreams(): no audio stream!";
	currentParams->enableVideo(false);
	if (videoStream) {
		if (audioStream)
			audio_stream_link_video(audioStream, videoStream);
		startVideoStream(targetState);
	}
	/* The on-hold file is to be played once both audio and video are ready */
	if (!onHoldFile.empty() && !params->getPrivate()->getInConference() && audioStream) {
		MSFilter *player = audio_stream_open_remote_play(audioStream, onHoldFile.c_str());
		if (player) {
			int pauseTime = 500;
			ms_filter_call_method(player, MS_PLAYER_SET_LOOP, &pauseTime);
			ms_filter_call_method_noarg(player, MS_PLAYER_START);
		}
	}
	upBandwidth = linphone_core_get_upload_bandwidth(core);
	if (params->realtimeTextEnabled())
		startTextStream();

	setDtlsFingerprintOnAllStreams();
	if (!iceAgent->hasCompleted()) {
		if (params->getMediaEncryption() == LinphoneMediaEncryptionDTLS) {
			currentParams->getPrivate()->setUpdateCallWhenIceCompleted(false);
			lInfo() << "Disabling update call when ice completed on call [" << q << "]";
		}
		iceAgent->startConnectivityChecks();
	} else {
		/* Should not start dtls until ice is completed */
		startDtlsOnAllStreams();
	}
}

void MediaSessionPrivate::startTextStream () {
	L_Q();
	const SalStreamDescription *tstream = sal_media_description_find_best_stream(resultDesc, SalText);
	if (tstream && (tstream->dir != SalStreamInactive) && (tstream->rtp_port != 0)) {
		const char *rtpAddr = tstream->rtp_addr[0] != '\0' ? tstream->rtp_addr : resultDesc->addr;
		const char *rtcpAddr = tstream->rtcp_addr[0] != '\0' ? tstream->rtcp_addr : resultDesc->addr;
		const SalStreamDescription *localStreamDesc = sal_media_description_find_stream(localDesc, tstream->proto, SalText);
		int usedPt = -1;
		textProfile = makeProfile(resultDesc, tstream, &usedPt);
		if (usedPt == -1)
			lWarning() << "No text stream accepted";
		else {
			currentParams->getPrivate()->setUsedRealtimeTextCodec(rtp_profile_get_payload(textProfile, usedPt));
			currentParams->enableRealtimeText(true);
			if (sal_stream_description_has_srtp(tstream)) {
				int cryptoIdx = findCryptoIndexFromTag(localStreamDesc->crypto, static_cast<unsigned char>(tstream->crypto_local_tag));
				if (cryptoIdx >= 0) {
					ms_media_stream_sessions_set_srtp_recv_key_b64(&textStream->ms.sessions, tstream->crypto[0].algo, tstream->crypto[0].master_key);
					ms_media_stream_sessions_set_srtp_send_key_b64(&textStream->ms.sessions, tstream->crypto[0].algo, localStreamDesc->crypto[cryptoIdx].master_key);
				}
			}
			configureRtpSessionForRtcpFb(tstream);
			configureRtpSessionForRtcpXr(SalText);
			rtp_session_enable_rtcp_mux(textStream->ms.sessions.rtp_session, tstream->rtcp_mux);
			bool_t isMulticast = ms_is_multicast(rtpAddr);
			if (isMulticast)
				rtp_session_set_multicast_ttl(textStream->ms.sessions.rtp_session, tstream->ttl);
			text_stream_start(textStream, textProfile, rtpAddr, tstream->rtp_port, rtcpAddr,
				(linphone_core_rtcp_enabled(core) && !isMulticast) ? (tstream->rtcp_port ? tstream->rtcp_port : tstream->rtp_port + 1) : 0, usedPt);
			ms_filter_add_notify_callback(textStream->rttsink, realTimeTextCharacterReceived, q, false);
			ms_media_stream_sessions_set_encryption_mandatory(&textStream->ms.sessions, isEncryptionMandatory());
		}
	} else
		lInfo() << "No valid text stream defined";
}

void MediaSessionPrivate::startVideoStream (LinphoneCallState targetState) {
#ifdef VIDEO_ENABLED
	L_Q();
	bool reusedPreview = false;
	/* Shutdown preview */
	MSFilter *source = nullptr;
	if (core->previewstream) {
		if (core->video_conf.reuse_preview_source)
			source = video_preview_stop_reuse_source(core->previewstream);
		else
			video_preview_stop(core->previewstream);
		core->previewstream = nullptr;
	}
	const SalStreamDescription *vstream = sal_media_description_find_best_stream(resultDesc, SalVideo);
	if (vstream && (vstream->dir != SalStreamInactive) && (vstream->rtp_port != 0)) {
		int usedPt = -1;
		videoProfile = makeProfile(resultDesc, vstream, &usedPt);
		if (usedPt == -1)
			lWarning() << "No video stream accepted";
		else {
			currentParams->getPrivate()->setUsedVideoCodec(rtp_profile_get_payload(videoProfile, usedPt));
			currentParams->enableVideo(true);
			rtp_session_enable_rtcp_mux(videoStream->ms.sessions.rtp_session, vstream->rtcp_mux);
			if (core->video_conf.preview_vsize.width != 0)
				video_stream_set_preview_size(videoStream, core->video_conf.preview_vsize);
			video_stream_set_fps(videoStream, linphone_core_get_preferred_framerate(core));
			if (lp_config_get_int(linphone_core_get_config(core), "video", "nowebcam_uses_normal_fps", 0))
				videoStream->staticimage_webcam_fps_optimization = false;
			const LinphoneVideoDefinition *vdef = linphone_core_get_preferred_video_definition(core);
			MSVideoSize vsize;
			vsize.width = static_cast<int>(linphone_video_definition_get_width(vdef));
			vsize.height = static_cast<int>(linphone_video_definition_get_height(vdef));
			video_stream_set_sent_video_size(videoStream, vsize);
			video_stream_enable_self_view(videoStream, core->video_conf.selfview);
			if (videoWindowId)
				video_stream_set_native_window_id(videoStream, videoWindowId);
			else if (core->video_window_id)
				video_stream_set_native_window_id(videoStream, core->video_window_id);
			if (core->preview_window_id)
				video_stream_set_native_preview_window_id(videoStream, core->preview_window_id);
			video_stream_use_preview_video_window(videoStream, core->use_preview_window);
			const char *rtpAddr = (vstream->rtp_addr[0] != '\0') ? vstream->rtp_addr : resultDesc->addr;
			const char *rtcpAddr = (vstream->rtcp_addr[0] != '\0') ? vstream->rtcp_addr : resultDesc->addr;
			bool isMulticast = ms_is_multicast(rtpAddr);
			MediaStreamDir dir = MediaStreamSendRecv;
			bool isActive = true;
			if (isMulticast) {
				if (vstream->multicast_role == SalMulticastReceiver)
					dir = MediaStreamRecvOnly;
				else
					dir = MediaStreamSendOnly;
			} else if ((vstream->dir == SalStreamSendOnly) && core->video_conf.capture)
				dir = MediaStreamSendOnly;
			else if ((vstream->dir == SalStreamRecvOnly) && core->video_conf.display)
				dir = MediaStreamRecvOnly;
			else if (vstream->dir == SalStreamSendRecv) {
				if (core->video_conf.display && core->video_conf.capture)
					dir = MediaStreamSendRecv;
				else if (core->video_conf.display)
					dir = MediaStreamRecvOnly;
				else
					dir = MediaStreamSendOnly;
			} else {
				lWarning() << "Video stream is inactive";
				/* Either inactive or incompatible with local capabilities */
				isActive = false;
			}
			MSWebCam *cam = getVideoDevice();
			if (isActive) {
				if (sal_stream_description_has_srtp(vstream)) {
					const SalStreamDescription *localStreamDesc = sal_media_description_find_stream(localDesc, vstream->proto, SalVideo);
					int cryptoIdx = findCryptoIndexFromTag(localStreamDesc->crypto, static_cast<unsigned char>(vstream->crypto_local_tag));
					if (cryptoIdx >= 0) {
						ms_media_stream_sessions_set_srtp_recv_key_b64(&videoStream->ms.sessions, vstream->crypto[0].algo, vstream->crypto[0].master_key);
						ms_media_stream_sessions_set_srtp_send_key_b64(&videoStream->ms.sessions, vstream->crypto[0].algo, localStreamDesc->crypto[cryptoIdx].master_key);
					}
				}
				configureRtpSessionForRtcpFb(vstream);
				configureRtpSessionForRtcpXr(SalVideo);
				configureAdaptiveRateControl(&videoStream->ms, currentParams->getUsedVideoCodec(), true);
				log->video_enabled = true;
				video_stream_set_direction(videoStream, dir);
				lInfo() << "startVideoStream: device_rotation=" << core->device_rotation;
				video_stream_set_device_rotation(videoStream, core->device_rotation);
				video_stream_set_freeze_on_error(videoStream, !!lp_config_get_int(linphone_core_get_config(core), "video", "freeze_on_error", 1));
				if (isMulticast)
					rtp_session_set_multicast_ttl(videoStream->ms.sessions.rtp_session, vstream->ttl);
				video_stream_use_video_preset(videoStream, lp_config_get_string(linphone_core_get_config(core), "video", "preset", nullptr));
				if (core->video_conf.reuse_preview_source && source) {
					lInfo() << "video_stream_start_with_source kept: " << source;
					video_stream_start_with_source(videoStream, videoProfile, rtpAddr, vstream->rtp_port, rtcpAddr,
						linphone_core_rtcp_enabled(core) ? (vstream->rtcp_port ? vstream->rtcp_port : vstream->rtp_port + 1) : 0,
						usedPt, -1, cam, source);
					reusedPreview = true;
				} else {
					bool ok = true;
					MSMediaStreamIO io = MS_MEDIA_STREAM_IO_INITIALIZER;
					bool useRtpIo = lp_config_get_int(linphone_core_get_config(core), "video", "rtp_io", false);
					if (useRtpIo) {
						io.input.type = io.output.type = MSResourceRtp;
						io.input.session = io.output.session = createVideoRtpIoSession();
						if (!io.input.session) {
							ok = false;
							lWarning() << "Cannot create video RTP IO session";
						}
					} else {
						io.input.type = MSResourceCamera;
						io.input.camera = cam;
						io.output.type = MSResourceDefault;
					}
					if (ok) {
						video_stream_start_from_io(videoStream, videoProfile, rtpAddr, vstream->rtp_port, rtcpAddr,
							(linphone_core_rtcp_enabled(core) && !isMulticast)  ? (vstream->rtcp_port ? vstream->rtcp_port : vstream->rtp_port + 1) : 0,
							usedPt, &io);
					}
				}
				ms_media_stream_sessions_set_encryption_mandatory(&videoStream->ms.sessions, isEncryptionMandatory());
				if (listener)
					listener->onResetFirstVideoFrameDecoded(q->getSharedFromThis());
				/* Start ZRTP engine if needed : set here or remote have a zrtp-hash attribute */
				SalMediaDescription *remote = op->get_remote_media_description();
				const SalStreamDescription *remoteStream = sal_media_description_find_best_stream(remote, SalVideo);
				if ((params->getMediaEncryption() == LinphoneMediaEncryptionZRTP) || (remoteStream->haveZrtpHash == 1)) {
					/* Audio stream is already encrypted and video stream is active */
					if (media_stream_secured(&audioStream->ms) && (media_stream_get_state(&videoStream->ms) == MSStreamStarted)) {
						video_stream_start_zrtp(videoStream);
						if (remoteStream->haveZrtpHash == 1) {
							int retval = ms_zrtp_setPeerHelloHash(videoStream->ms.sessions.zrtp_context, (uint8_t *)remoteStream->zrtphash, strlen((const char *)(remoteStream->zrtphash)));
							if (retval != 0)
								lError() << "Video stream ZRTP hash mismatch 0x" << hex << retval;
						}
					}
				}
			}
		}
	} else
		lInfo() << "No valid video stream defined";
	if (!reusedPreview && source) {
		/* Destroy not-reused source filter */
		lWarning() << "Video preview (" << source << ") not reused: destroying it";
		ms_filter_destroy(source);
	}
#endif
}

void MediaSessionPrivate::stopAudioStream () {
	L_Q();
	if (audioStream) {
		updateReportingMediaInfo(LINPHONE_CALL_STATS_AUDIO);
		media_stream_reclaim_sessions(&audioStream->ms, &sessions[mainAudioStreamIndex]);
		if (audioStream->ec) {
			char *stateStr = nullptr;
			ms_filter_call_method(audioStream->ec, MS_ECHO_CANCELLER_GET_STATE_STRING, &stateStr);
			if (stateStr) {
				lInfo() << "Writing echo canceler state, " << (int)strlen(stateStr) << " bytes";
				lp_config_write_relative_file(linphone_core_get_config(core), ecStateStore.c_str(), stateStr);
			}
		}
		audio_stream_get_local_rtp_stats(audioStream, &log->local_stats);
		fillLogStats(&audioStream->ms);
#if 0
		if (call->endpoint) {
			linphone_conference_on_call_stream_stopping(lc->conf_ctx, call);
		}
#endif
		ms_bandwidth_controller_remove_stream(core->bw_controller, &audioStream->ms);
		audio_stream_stop(audioStream);
		updateRtpStats(audioStats, mainAudioStreamIndex);
		audioStream = nullptr;
		handleStreamEvents(mainAudioStreamIndex);
		rtp_session_unregister_event_queue(sessions[mainAudioStreamIndex].rtp_session, audioStreamEvQueue);
		ortp_ev_queue_flush(audioStreamEvQueue);
		ortp_ev_queue_destroy(audioStreamEvQueue);
		audioStreamEvQueue = nullptr;

		q->getCurrentParams()->getPrivate()->setUsedAudioCodec(nullptr);
	}
}

void MediaSessionPrivate::stopStreams () {
	if (audioStream || videoStream || textStream) {
		if (audioStream && videoStream)
			audio_stream_unlink_video(audioStream, videoStream);
		stopAudioStream();
		stopVideoStream();
		stopTextStream();
		if (core->msevq)
			ms_event_queue_skip(core->msevq);
	}

	if (audioProfile) {
		rtp_profile_destroy(audioProfile);
		audioProfile = nullptr;
		unsetRtpProfile(mainAudioStreamIndex);
	}
	if (videoProfile) {
		rtp_profile_destroy(videoProfile);
		videoProfile = nullptr;
		unsetRtpProfile(mainVideoStreamIndex);
	}
	if (textProfile) {
		rtp_profile_destroy(textProfile);
		textProfile = nullptr;
		unsetRtpProfile(mainTextStreamIndex);
	}
	if (rtpIoAudioProfile) {
		rtp_profile_destroy(rtpIoAudioProfile);
		rtpIoAudioProfile = nullptr;
	}
	if (rtpIoVideoProfile) {
		rtp_profile_destroy(rtpIoVideoProfile);
		rtpIoVideoProfile = nullptr;
	}

	linphone_core_soundcard_hint_check(core);
}

void MediaSessionPrivate::stopTextStream () {
	L_Q();
	if (textStream) {
		updateReportingMediaInfo(LINPHONE_CALL_STATS_TEXT);
		media_stream_reclaim_sessions(&textStream->ms, &sessions[mainTextStreamIndex]);
		fillLogStats(&textStream->ms);
		text_stream_stop(textStream);
		updateRtpStats(textStats, mainTextStreamIndex);
		textStream = nullptr;
		handleStreamEvents(mainTextStreamIndex);
		rtp_session_unregister_event_queue(sessions[mainTextStreamIndex].rtp_session, textStreamEvQueue);
		ortp_ev_queue_flush(textStreamEvQueue);
		ortp_ev_queue_destroy(textStreamEvQueue);
		textStreamEvQueue = nullptr;
		q->getCurrentParams()->getPrivate()->setUsedRealtimeTextCodec(nullptr);
	}
}

void MediaSessionPrivate::stopVideoStream () {
#ifdef VIDEO_ENABLED
	L_Q();
	if (videoStream) {
		updateReportingMediaInfo(LINPHONE_CALL_STATS_VIDEO);
		media_stream_reclaim_sessions(&videoStream->ms, &sessions[mainVideoStreamIndex]);
		fillLogStats(&videoStream->ms);
		ms_bandwidth_controller_remove_stream(core->bw_controller, &videoStream->ms);
		video_stream_stop(videoStream);
		updateRtpStats(videoStats, mainVideoStreamIndex);
		videoStream = nullptr;
		handleStreamEvents(mainVideoStreamIndex);
		rtp_session_unregister_event_queue(sessions[mainVideoStreamIndex].rtp_session, videoStreamEvQueue);
		ortp_ev_queue_flush(videoStreamEvQueue);
		ortp_ev_queue_destroy(videoStreamEvQueue);
		videoStreamEvQueue = nullptr;
		q->getCurrentParams()->getPrivate()->setUsedVideoCodec(nullptr);
	}
#endif
}

void MediaSessionPrivate::tryEarlyMediaForking (SalMediaDescription *md) {
	L_Q();
	lInfo() << "Early media response received from another branch, checking if media can be forked to this new destination";
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&resultDesc->streams[i]))
			continue;
		SalStreamDescription *refStream = &resultDesc->streams[i];
		SalStreamDescription *newStream = &md->streams[i];
		if ((refStream->type == newStream->type) && refStream->payloads && newStream->payloads) {
			OrtpPayloadType *refpt = reinterpret_cast<OrtpPayloadType *>(refStream->payloads->data);
			OrtpPayloadType *newpt = reinterpret_cast<OrtpPayloadType *>(newStream->payloads->data);
			if ((strcmp(refpt->mime_type, newpt->mime_type) == 0) && (refpt->clock_rate == newpt->clock_rate)
				&& (payload_type_get_number(refpt) == payload_type_get_number(newpt))) {
				MediaStream *ms = nullptr;
				if (refStream->type == SalAudio)
					ms = &audioStream->ms;
				else if (refStream->type == SalVideo)
					ms = &videoStream->ms;
				if (ms) {
					RtpSession *session = ms->sessions.rtp_session;
					const char *rtpAddr = (newStream->rtp_addr[0] != '\0') ? newStream->rtp_addr : md->addr;
					const char *rtcpAddr = (newStream->rtcp_addr[0] != '\0') ? newStream->rtcp_addr : md->addr;
					if (ms_is_multicast(rtpAddr))
						lInfo() << "Multicast addr [" << rtpAddr << "/" << newStream->rtp_port << "] does not need auxiliary rtp's destination for CallSession [" << q << "]";
					else
						rtp_session_add_aux_remote_addr_full(session, rtpAddr, newStream->rtp_port, rtcpAddr, newStream->rtcp_port);
				}
			}
		}
	}
}

void MediaSessionPrivate::updateFrozenPayloads (SalMediaDescription *result) {
	L_Q();
	for (int i = 0; i < result->nb_streams; i++) {
		for (bctbx_list_t *elem = result->streams[i].payloads; elem != nullptr; elem = bctbx_list_next(elem)) {
			OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(elem));
			if (PayloadTypeHandler::isPayloadTypeNumberAvailable(localDesc->streams[i].already_assigned_payloads, payload_type_get_number(pt), nullptr)) {
				/* New codec, needs to be added to the list */
				localDesc->streams[i].already_assigned_payloads = bctbx_list_append(localDesc->streams[i].already_assigned_payloads, payload_type_clone(pt));
				lInfo() << "CallSession[" << q << "] : payload type " << payload_type_get_number(pt) << " " << pt->mime_type << "/" << pt->clock_rate
					<< " fmtp=" << L_C_TO_STRING(pt->recv_fmtp) << " added to frozen list";
			}
		}
	}
}

void MediaSessionPrivate::updateStreams (SalMediaDescription *newMd, LinphoneCallState targetState) {
	L_Q();
	if (!((state == LinphoneCallIncomingEarlyMedia) && linphone_core_get_ring_during_incoming_early_media(core)))
		linphone_core_stop_ringing(core);
	if (!newMd) {
		lError() << "updateStreams() called with null media description";
		return;
	}
	updateBiggestDesc(localDesc);
	sal_media_description_ref(newMd);
	SalMediaDescription *oldMd = resultDesc;
	resultDesc = newMd;
	if ((audioStream && (audioStream->ms.state == MSStreamStarted)) || (videoStream && (videoStream->ms.state == MSStreamStarted))) {
		clearEarlyMediaDestinations();

		/* We already started media: check if we really need to restart it */
		int mdChanged = 0;
		if (oldMd) {
			mdChanged = mediaParametersChanged(oldMd, newMd);
			/* Might not be mandatory to restart stream for each ice restart as it leads bad user experience, specially in video. See 0002495 for better background on this */
			if (mdChanged & (SAL_MEDIA_DESCRIPTION_CODEC_CHANGED
				| SAL_MEDIA_DESCRIPTION_STREAMS_CHANGED
				| SAL_MEDIA_DESCRIPTION_NETWORK_XXXCAST_CHANGED
				| SAL_MEDIA_DESCRIPTION_ICE_RESTART_DETECTED
				| SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION))
				lInfo() << "Media descriptions are different, need to restart the streams";
#if 0
			else if (call->playing_ringbacktone)
				ms_message("Playing ringback tone, will restart the streams.");
#endif
			else {
				if (allMuted && (targetState == LinphoneCallStreamsRunning)) {
					lInfo() << "Early media finished, unmuting inputs...";
					/* We were in early media, now we want to enable real media */
					allMuted = false;
					if (audioStream)
						linphone_core_enable_mic(core, linphone_core_mic_enabled(core));
#ifdef VIDEO_ENABLED
					if (videoStream && cameraEnabled)
						q->enableCamera(q->cameraEnabled());
#endif
				}
				if (mdChanged == SAL_MEDIA_DESCRIPTION_UNCHANGED) {
					/* FIXME ZRTP, might be restarted in any cases? */
					lInfo() << "No need to restart streams, SDP is unchanged";
					if (oldMd)
						sal_media_description_unref(oldMd);
					return;
				} else {
					if (mdChanged & SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED) {
						lInfo() << "Network parameters have changed, update them";
						updateStreamsDestinations(oldMd, newMd);
					}
					if (mdChanged & SAL_MEDIA_DESCRIPTION_CRYPTO_KEYS_CHANGED) {
						lInfo() << "Crypto parameters have changed, update them";
						updateCryptoParameters(oldMd, newMd);
					}
					if (oldMd)
						sal_media_description_unref(oldMd);
					return;
				}
			}
		}
		stopStreams();
		if (mdChanged & SAL_MEDIA_DESCRIPTION_NETWORK_XXXCAST_CHANGED) {
			lInfo() << "Media ip type has changed, destroying sessions context on CallSession [" << q << "]";
			ms_media_stream_sessions_uninit(&sessions[mainAudioStreamIndex]);
			ms_media_stream_sessions_uninit(&sessions[mainVideoStreamIndex]);
			ms_media_stream_sessions_uninit(&sessions[mainTextStreamIndex]);
		}
		initializeStreams();
	}

	if (!audioStream) {
		/* This happens after pausing the call locally. The streams are destroyed and then we wait the 200Ok to recreate them */
		initializeStreams();
	}

	if (params->earlyMediaSendingEnabled() && (state == LinphoneCallOutgoingEarlyMedia))
		prepareEarlyMediaForking();
	startStreams(targetState);
	if ((state == LinphoneCallPausing) && pausedByApp && (bctbx_list_size(core->calls) == 1))
		linphone_core_play_named_tone(core, LinphoneToneCallOnHold);
	updateFrozenPayloads(newMd);

	if (oldMd)
		sal_media_description_unref(oldMd);
}

void MediaSessionPrivate::updateStreamsDestinations (SalMediaDescription *oldMd, SalMediaDescription *newMd) {
	SalStreamDescription *newAudioDesc = nullptr;

	#ifdef VIDEO_ENABLED
		SalStreamDescription *newVideoDesc = nullptr;
	#endif

	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&newMd->streams[i]))
			continue;
		if (newMd->streams[i].type == SalAudio)
			newAudioDesc = &newMd->streams[i];

		#ifdef VIDEO_ENABLED
			else if (newMd->streams[i].type == SalVideo)
				newVideoDesc = &newMd->streams[i];
		#endif
	}
	if (audioStream && newAudioDesc) {
		const char *rtpAddr = (newAudioDesc->rtp_addr[0] != '\0') ? newAudioDesc->rtp_addr : newMd->addr;
		const char *rtcpAddr = (newAudioDesc->rtcp_addr[0] != '\0') ? newAudioDesc->rtcp_addr : newMd->addr;
		lInfo() << "Change audio stream destination: RTP=" << rtpAddr << ":" << newAudioDesc->rtp_port << " RTCP=" << rtcpAddr << ":" << newAudioDesc->rtcp_port;
		rtp_session_set_remote_addr_full(audioStream->ms.sessions.rtp_session, rtpAddr, newAudioDesc->rtp_port, rtcpAddr, newAudioDesc->rtcp_port);
	}
#ifdef VIDEO_ENABLED
	if (videoStream && newVideoDesc) {
		const char *rtpAddr = (newVideoDesc->rtp_addr[0] != '\0') ? newVideoDesc->rtp_addr : newMd->addr;
		const char *rtcpAddr = (newVideoDesc->rtcp_addr[0] != '\0') ? newVideoDesc->rtcp_addr : newMd->addr;
		lInfo() << "Change video stream destination: RTP=" << rtpAddr << ":" << newVideoDesc->rtp_port << " RTCP=" << rtcpAddr << ":" << newVideoDesc->rtcp_port;
		rtp_session_set_remote_addr_full(videoStream->ms.sessions.rtp_session, rtpAddr, newVideoDesc->rtp_port, rtcpAddr, newVideoDesc->rtcp_port);
	}
#endif
}

// -----------------------------------------------------------------------------

bool MediaSessionPrivate::allStreamsAvpfEnabled () const {
	int nbActiveStreams = 0;
	int nbAvpfEnabledStreams = 0;
	if (audioStream && media_stream_get_state(&audioStream->ms) == MSStreamStarted) {
		nbActiveStreams++;
		if (media_stream_avpf_enabled(&audioStream->ms))
			nbAvpfEnabledStreams++;
	}
	if (videoStream && media_stream_get_state(&videoStream->ms) == MSStreamStarted) {
		nbActiveStreams++;
		if (media_stream_avpf_enabled(&videoStream->ms))
			nbAvpfEnabledStreams++;
	}
	return (nbActiveStreams > 0) && (nbActiveStreams == nbAvpfEnabledStreams);
}

bool MediaSessionPrivate::allStreamsEncrypted () const {
	int numberOfEncryptedStreams = 0;
	int numberOfActiveStreams = 0;
	if (audioStream && (media_stream_get_state(&audioStream->ms) == MSStreamStarted)) {
		numberOfActiveStreams++;
		if (media_stream_secured(&audioStream->ms))
			numberOfEncryptedStreams++;
	}
	if (videoStream && (media_stream_get_state(&videoStream->ms) == MSStreamStarted)) {
		numberOfActiveStreams++;
		if (media_stream_secured(&videoStream->ms))
			numberOfEncryptedStreams++;
	}
	if (textStream && (media_stream_get_state(&textStream->ms) == MSStreamStarted)) {
		numberOfActiveStreams++;
		if (media_stream_secured(&textStream->ms))
			numberOfEncryptedStreams++;
	}
	return (numberOfActiveStreams > 0) && (numberOfActiveStreams == numberOfEncryptedStreams);
}

bool MediaSessionPrivate::atLeastOneStreamStarted () const {
	return (audioStream && (media_stream_get_state(&audioStream->ms) == MSStreamStarted))
		|| (videoStream && (media_stream_get_state(&videoStream->ms) == MSStreamStarted))
		|| (textStream && (media_stream_get_state(&textStream->ms) == MSStreamStarted));
}

void MediaSessionPrivate::audioStreamAuthTokenReady (const string &authToken, bool verified) {
	this->authToken = authToken;
	authTokenVerified = verified;
	lInfo() << "Authentication token is " << authToken << "(" << (verified ? "verified" : "unverified") << ")";
}

void MediaSessionPrivate::audioStreamEncryptionChanged (bool encrypted) {
	propagateEncryptionChanged();

	#ifdef VIDEO_ENABLED
		L_Q();
		/* Enable video encryption */
		if ((params->getMediaEncryption() == LinphoneMediaEncryptionZRTP) && q->getCurrentParams()->videoEnabled()) {
			lInfo() << "Trying to start ZRTP encryption on video stream";
			video_stream_start_zrtp(videoStream);
		}
	#endif
}

uint16_t MediaSessionPrivate::getAvpfRrInterval () const {
	uint16_t rrInterval = 0;
	if (audioStream && (media_stream_get_state(&audioStream->ms) == MSStreamStarted)) {
		uint16_t streamRrInterval = media_stream_get_avpf_rr_interval(&audioStream->ms);
		if (streamRrInterval > rrInterval) rrInterval = streamRrInterval;
	}
	if (videoStream && (media_stream_get_state(&videoStream->ms) == MSStreamStarted)) {
		uint16_t streamRrInterval = media_stream_get_avpf_rr_interval(&videoStream->ms);
		if (streamRrInterval > rrInterval) rrInterval = streamRrInterval;
	}
	return rrInterval;
}

unsigned int MediaSessionPrivate::getNbActiveStreams () const {
	SalMediaDescription *md = nullptr;
	if (op)
		md = op->get_remote_media_description();
	if (!md)
		return 0;
	return sal_media_description_nb_active_streams_of_type(md, SalAudio) + sal_media_description_nb_active_streams_of_type(md, SalVideo) + sal_media_description_nb_active_streams_of_type(md, SalText);
}

bool MediaSessionPrivate::isEncryptionMandatory () const {
	L_Q();
	if (params->getMediaEncryption() == LinphoneMediaEncryptionDTLS) {
		lInfo() << "Forced encryption mandatory on CallSession [" << q << "] due to SRTP-DTLS";
		return true;
	}
	return params->mandatoryMediaEncryptionEnabled();
}

int MediaSessionPrivate::mediaParametersChanged (SalMediaDescription *oldMd, SalMediaDescription *newMd) {
	if (params->getPrivate()->getInConference() != currentParams->getPrivate()->getInConference())
		return SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION;
	if (upBandwidth != linphone_core_get_upload_bandwidth(core))
		return SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION;
	if (localDescChanged) {
		char *differences = sal_media_description_print_differences(localDescChanged);
		lInfo() << "Local description has changed: " << differences;
		ms_free(differences);
	}
	int otherDescChanged = sal_media_description_equals(oldMd, newMd);
	if (otherDescChanged) {
		char *differences = sal_media_description_print_differences(otherDescChanged);
		lInfo() << "Other description has changed: " << differences;
		ms_free(differences);
	}
	return localDescChanged | otherDescChanged;
}

void MediaSessionPrivate::propagateEncryptionChanged () {
	L_Q();
	if (!allStreamsEncrypted()) {
		lInfo() << "Some streams are not encrypted";
		q->getCurrentParams()->setMediaEncryption(LinphoneMediaEncryptionNone);
		if (listener)
			listener->onEncryptionChanged(q->getSharedFromThis(), false, authToken);
	} else {
		if (!authToken.empty()) {
			/* ZRTP only is using auth_token */
			q->getCurrentParams()->setMediaEncryption(LinphoneMediaEncryptionZRTP);
		} else {
			/* Otherwise it must be DTLS as SDES doesn't go through this function */
			q->getCurrentParams()->setMediaEncryption(LinphoneMediaEncryptionDTLS);
		}
		lInfo() << "All streams are encrypted, key exchanged using "
			<< ((q->getCurrentParams()->getMediaEncryption() == LinphoneMediaEncryptionZRTP) ? "ZRTP"
				: (q->getCurrentParams()->getMediaEncryption() == LinphoneMediaEncryptionDTLS) ? "DTLS" : "Unknown mechanism");
		if (listener)
			listener->onEncryptionChanged(q->getSharedFromThis(), true, authToken);
#ifdef VIDEO_ENABLED
		if (isEncryptionMandatory() && videoStream && media_stream_started(&videoStream->ms)) {
			/* Nothing could have been sent yet so generating key frame */
			video_stream_send_vfu(videoStream);
		}
#endif
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::fillLogStats (MediaStream *st) {
	float quality = media_stream_get_average_quality_rating(st);
	if (quality >= 0) {
		if (static_cast<int>(log->quality) == -1)
			log->quality = quality;
		else
			log->quality *= quality / 5.0f;
	}
}

void MediaSessionPrivate::updateRtpStats (LinphoneCallStats *stats, int streamIndex) {
	if (sessions[streamIndex].rtp_session) {
		const rtp_stats_t *rtpStats = rtp_session_get_stats(sessions[streamIndex].rtp_session);
		if (rtpStats)
			_linphone_call_stats_set_rtp_stats(stats, rtpStats);
	}
}

// -----------------------------------------------------------------------------

bool MediaSessionPrivate::mediaReportEnabled (int statsType) {
	L_Q();
	if (!qualityReportingEnabled())
		return false;
	if ((statsType == LINPHONE_CALL_STATS_VIDEO) && !q->getCurrentParams()->videoEnabled())
		return false;
	if ((statsType == LINPHONE_CALL_STATS_TEXT) && !q->getCurrentParams()->realtimeTextEnabled())
		return false;
	return (log->reporting.reports[statsType] != nullptr);
}

bool MediaSessionPrivate::qualityReportingEnabled () const {
	return (destProxy && linphone_proxy_config_quality_reporting_enabled(destProxy));
}

void MediaSessionPrivate::updateReportingCallState () {
	if ((state == LinphoneCallReleased) || !qualityReportingEnabled())
		return;
	switch (state) {
		case LinphoneCallStreamsRunning:
#if 0
			for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
				int streamIndex = (i == mainAudioStreamIndex) ? LINPHONE_CALL_STATS_AUDIO : mainVideoStreamIndex ? LINPHONE_CALL_STATS_VIDEO : LINPHONE_CALL_STATS_TEXT;
				bool enabled = mediaReportEnabled(streamIndex);
				MediaStream *ms = getMediaStream(i);
				if (enabled && set_on_action_suggested_cb(ms, qos_analyzer_on_action_suggested, log->reporting.reports[streamIndex])) {
					log->reporting.reports[streamIndex]->call = call;
					STR_REASSIGN(log->reporting.reports[streamIndex]->qos_analyzer.name, ms_strdup(ms_qos_analyzer_get_name(ms_bitrate_controller_get_qos_analyzer(ms->rc))));
				}
			}
			linphone_reporting_update_ip(call);
			if (!mediaReportEnabled(LINPHONE_CALL_STATS_VIDEO) && log->reporting.was_video_running)
				send_report(log->reporting.reports[LINPHONE_CALL_STATS_VIDEO], "VQSessionReport");
#endif
			log->reporting.was_video_running = mediaReportEnabled(LINPHONE_CALL_STATS_VIDEO);
			break;
		case LinphoneCallEnd:
#if 0
			set_on_action_suggested_cb(&audioStream->ms, nullptr, nullptr);
			set_on_action_suggested_cb(&videoStream->ms, nullptr, nullptr);
			if ((log->status == LinphoneCallSuccess) || (log->status == LinphoneCallAborted))
				linphone_reporting_publish_session_report(call, true);
#endif
			break;
		default:
			break;
	}
}

void MediaSessionPrivate::updateReportingMediaInfo (int statsType) {
	L_Q();
	/* op might be already released if hanging up in state LinphoneCallOutgoingInit */
	if (!op || !mediaReportEnabled(statsType))
		return;

	char *dialogId = op->get_dialog_id();
	reporting_session_report_t * report = log->reporting.reports[statsType];
	STR_REASSIGN(report->info.call_id, ms_strdup(log->call_id));

	STR_REASSIGN(report->local_metrics.user_agent, ms_strdup(linphone_core_get_user_agent(core)));
	STR_REASSIGN(report->remote_metrics.user_agent, ms_strdup(q->getRemoteUserAgent().c_str()));

	/* RFC states: "LocalGroupID provides the identification for the purposes of aggregation for the local endpoint" */
	STR_REASSIGN(report->info.local_addr.group, ms_strdup_printf("%s-%s-%s",
		dialogId ? dialogId : "", "local",
		report->local_metrics.user_agent ? report->local_metrics.user_agent : ""));
	STR_REASSIGN(report->info.remote_addr.group, ms_strdup_printf("%s-%s-%s",
		dialogId ? dialogId : "", "remote",
		report->remote_metrics.user_agent ? report->remote_metrics.user_agent : ""));

	if (direction == LinphoneCallIncoming) {
		STR_REASSIGN(report->info.remote_addr.id, linphone_address_as_string(log->from));
		STR_REASSIGN(report->info.local_addr.id, linphone_address_as_string(log->to));
		STR_REASSIGN(report->info.orig_id, ms_strdup(report->info.remote_addr.id));
	} else {
		STR_REASSIGN(report->info.remote_addr.id, linphone_address_as_string(log->to));
		STR_REASSIGN(report->info.local_addr.id, linphone_address_as_string(log->from));
		STR_REASSIGN(report->info.orig_id, ms_strdup(report->info.local_addr.id));
	}

	report->local_metrics.timestamps.start = log->start_date_time;
	report->local_metrics.timestamps.stop = log->start_date_time + q->getDuration();

	/* We use same timestamps for remote too */
	report->remote_metrics.timestamps.start = log->start_date_time;
	report->remote_metrics.timestamps.stop = log->start_date_time + q->getDuration();

	/* Yet we use the same payload config for local and remote, since this is the largest use case */
	MediaStream *stream = nullptr;
	const OrtpPayloadType *localPayload = nullptr;
	const OrtpPayloadType *remotePayload = nullptr;
	if (audioStream && (statsType == LINPHONE_CALL_STATS_AUDIO)) {
		stream = &audioStream->ms;
		localPayload = q->getCurrentParams()->getUsedAudioCodec();
		remotePayload = localPayload;
	} else if (videoStream && (statsType == LINPHONE_CALL_STATS_VIDEO)) {
		stream = &videoStream->ms;
		localPayload = q->getCurrentParams()->getUsedVideoCodec();
		remotePayload = localPayload;
	} else if (textStream && (statsType == LINPHONE_CALL_STATS_TEXT)) {
		stream = &textStream->ms;
		localPayload = q->getCurrentParams()->getUsedRealtimeTextCodec();
		remotePayload = localPayload;
	}

	if (stream) {
		RtpSession * session = stream->sessions.rtp_session;
		report->info.local_addr.ssrc = rtp_session_get_send_ssrc(session);
		report->info.remote_addr.ssrc = rtp_session_get_recv_ssrc(session);
		if (stream->qi){
			report->local_metrics.quality_estimates.moslq = ms_quality_indicator_get_average_lq_rating(stream->qi) >= 0 ?
				MAX(1, ms_quality_indicator_get_average_lq_rating(stream->qi)) : -1;
			report->local_metrics.quality_estimates.moscq = ms_quality_indicator_get_average_rating(stream->qi) >= 0 ?
				MAX(1, ms_quality_indicator_get_average_rating(stream->qi)) : -1;
		}
	}

	STR_REASSIGN(report->dialog_id, ms_strdup_printf("%s;%u", dialogId ? dialogId : "", report->info.local_addr.ssrc));
	ms_free(dialogId);

	if (localPayload) {
		report->local_metrics.session_description.payload_type = localPayload->type;
		if (localPayload->mime_type)
			STR_REASSIGN(report->local_metrics.session_description.payload_desc, ms_strdup(localPayload->mime_type));
		report->local_metrics.session_description.sample_rate = localPayload->clock_rate;
		if (localPayload->recv_fmtp)
			STR_REASSIGN(report->local_metrics.session_description.fmtp, ms_strdup(localPayload->recv_fmtp));
	}

	if (remotePayload) {
		report->remote_metrics.session_description.payload_type = remotePayload->type;
		STR_REASSIGN(report->remote_metrics.session_description.payload_desc, ms_strdup(remotePayload->mime_type));
		report->remote_metrics.session_description.sample_rate = remotePayload->clock_rate;
		STR_REASSIGN(report->remote_metrics.session_description.fmtp, ms_strdup(remotePayload->recv_fmtp));
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::executeBackgroundTasks (bool oneSecondElapsed) {
	switch (state) {
	case LinphoneCallStreamsRunning:
	case LinphoneCallOutgoingEarlyMedia:
	case LinphoneCallIncomingEarlyMedia:
	case LinphoneCallPausedByRemote:
	case LinphoneCallPaused:
		if (oneSecondElapsed) {
			float audioLoad = 0.f;
			float videoLoad = 0.f;
			float textLoad = 0.f;
			if (audioStream && audioStream->ms.sessions.ticker)
				audioLoad = ms_ticker_get_average_load(audioStream->ms.sessions.ticker);
			if (videoStream && videoStream->ms.sessions.ticker)
				videoLoad = ms_ticker_get_average_load(videoStream->ms.sessions.ticker);
			if (textStream && textStream->ms.sessions.ticker)
				textLoad = ms_ticker_get_average_load(textStream->ms.sessions.ticker);
			reportBandwidth();
			lInfo() << "Thread processing load: audio=" << audioLoad << "\tvideo=" << videoLoad << "\ttext=" << textLoad;
		}
		break;
	default:
		/* No stats for other states */
		break;
	}

	handleStreamEvents(mainAudioStreamIndex);
	handleStreamEvents(mainVideoStreamIndex);
	handleStreamEvents(mainTextStreamIndex);

#if 0
	int disconnectTimeout = linphone_core_get_nortp_timeout(core);
	bool disconnected = false;
	if (((state == LinphoneCallStreamsRunning) || (state == LinphoneCallPausedByRemote)) && oneSecondElapsed && audioStream
		&& (audioStream->ms.state == MSStreamStarted) && (disconnectTimeout > 0)) {
		disconnected = !audio_stream_alive(audioStream, disconnectTimeout);
	}
	if (disconnected)
		linphone_call_lost(call);
#endif
}

void MediaSessionPrivate::reportBandwidth () {
	L_Q();
	reportBandwidthForStream(&audioStream->ms, LinphoneStreamTypeAudio);
	reportBandwidthForStream(&videoStream->ms, LinphoneStreamTypeVideo);
	reportBandwidthForStream(&textStream->ms, LinphoneStreamTypeText);

	lInfo() << "Bandwidth usage for CallSession [" << q << "]:\n" << fixed << setprecision(2) <<
		"\tRTP  audio=[d=" << linphone_call_stats_get_download_bandwidth(audioStats) << ",u=" << linphone_call_stats_get_upload_bandwidth(audioStats) <<
		"], video=[d=" << linphone_call_stats_get_download_bandwidth(videoStats) << ",u=" << linphone_call_stats_get_upload_bandwidth(videoStats) <<
		"], text=[d=" << linphone_call_stats_get_download_bandwidth(textStats) << ",u=" << linphone_call_stats_get_upload_bandwidth(textStats) << "] kbits/sec\n" <<
		"\tRTCP audio=[d=" << linphone_call_stats_get_rtcp_download_bandwidth(audioStats) << ",u=" << linphone_call_stats_get_rtcp_upload_bandwidth(audioStats) <<
		"], video=[d=" << linphone_call_stats_get_rtcp_download_bandwidth(videoStats) << ",u=" << linphone_call_stats_get_rtcp_upload_bandwidth(videoStats) <<
		"], text=[d=" << linphone_call_stats_get_rtcp_download_bandwidth(textStats) << ",u=" << linphone_call_stats_get_rtcp_upload_bandwidth(textStats) << "] kbits/sec";
}

void MediaSessionPrivate::reportBandwidthForStream (MediaStream *ms, LinphoneStreamType type) {
	LinphoneCallStats *stats = nullptr;
	if (type == LinphoneStreamTypeAudio) {
		stats = audioStats;
	} else if (type == LinphoneStreamTypeVideo) {
		stats = videoStats;
	} else if (type == LinphoneStreamTypeText) {
		stats = textStats;
	} else
		return;

	bool active = ms ? (media_stream_get_state(ms) == MSStreamStarted) : false;
	_linphone_call_stats_set_download_bandwidth(stats, active ? (float)(media_stream_get_down_bw(ms) * 1e-3) : 0.f);
	_linphone_call_stats_set_upload_bandwidth(stats, active ? (float)(media_stream_get_up_bw(ms) * 1e-3) : 0.f);
	_linphone_call_stats_set_rtcp_download_bandwidth(stats, active ? (float)(media_stream_get_rtcp_down_bw(ms) * 1e-3) : 0.f);
	_linphone_call_stats_set_rtcp_upload_bandwidth(stats, active ? (float)(media_stream_get_rtcp_up_bw(ms) * 1e-3) : 0.f);
	_linphone_call_stats_set_ip_family_of_remote(stats,
		active ? (ortp_stream_is_ipv6(&ms->sessions.rtp_session->rtp.gs) ? LinphoneAddressFamilyInet6 : LinphoneAddressFamilyInet) : LinphoneAddressFamilyUnspec);

	if (core->send_call_stats_periodical_updates) {
		if (active)
			linphone_call_stats_update(stats, ms);
		_linphone_call_stats_set_updated(stats, _linphone_call_stats_get_updated(stats) | LINPHONE_CALL_STATS_PERIODICAL_UPDATE);
		if (listener)
			listener->onStatsUpdated(stats);
		_linphone_call_stats_set_updated(stats, 0);
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::abort (const string &errorMsg) {
#if 0
	linphone_core_stop_ringing(lc);
#endif
	stopStreams();
	CallSessionPrivate::abort(errorMsg);
}

void MediaSessionPrivate::handleIncomingReceivedStateInIncomingNotification () {
	L_Q();
	/* Try to be best-effort in giving real local or routable contact address for 100Rel case */
	setContactOp();
	bool proposeEarlyMedia = !!lp_config_get_int(linphone_core_get_config(core), "sip", "incoming_calls_early_media", false);
	if (proposeEarlyMedia)
		q->acceptEarlyMedia();
	else
		op->notify_ringing(false);
	if (op->get_replaces() && !!lp_config_get_int(linphone_core_get_config(core), "sip", "auto_answer_replacing_calls", 1))
		q->accept();
}

bool MediaSessionPrivate::isReadyForInvite () const {
	bool callSessionReady = CallSessionPrivate::isReadyForInvite();
	bool iceReady = false;
	if (iceAgent->hasSession()) {
		if (iceAgent->candidatesGathered())
			iceReady = true;
	} else
		iceReady = true;
	return callSessionReady && iceReady;
}

LinphoneStatus MediaSessionPrivate::pause () {
	L_Q();
	if ((state != LinphoneCallStreamsRunning) && (state != LinphoneCallPausedByRemote)) {
		lWarning() << "Cannot pause this MediaSession, it is not active";
		return -1;
	}
	string subject;
	if (sal_media_description_has_dir(resultDesc, SalStreamSendRecv))
		subject = "Call on hold";
	else if (sal_media_description_has_dir(resultDesc, SalStreamRecvOnly))
		subject = "Call on hold for me too";
	else {
		lError() << "No reason to pause this call, it is already paused or inactive";
		return -1;
	}
#if 0
	call->broken = FALSE;
#endif
	setState(LinphoneCallPausing, "Pausing call");
	makeLocalMediaDescription();
	op->set_local_media_description(localDesc);
	op->update(subject.c_str(), false);
	if (listener)
		listener->onResetCurrentSession(q->getSharedFromThis());
	if (audioStream || videoStream || textStream)
		stopStreams();
	pausedByApp = false;
	return 0;
}

int MediaSessionPrivate::restartInvite () {
	stopStreams();
	initializeStreams();
	return CallSessionPrivate::restartInvite();
}

void MediaSessionPrivate::setTerminated () {
	freeResources();
	CallSessionPrivate::setTerminated();
}

LinphoneStatus MediaSessionPrivate::startAcceptUpdate (LinphoneCallState nextState, const string &stateInfo) {
	if (iceAgent->hasSession() && (iceAgent->getNbLosingPairs() > 0)) {
		/* Defer the sending of the answer until there are no losing pairs left */
		return 0;
	}
	makeLocalMediaDescription();
	updateRemoteSessionIdAndVer();
	op->set_local_media_description(localDesc);
	op->accept();
	SalMediaDescription *md = op->get_final_media_description();
	iceAgent->stopIceForInactiveStreams(md);
	if (md && !sal_media_description_empty(md))
		updateStreams(md, nextState);
	setState(nextState, stateInfo);
	return 0;
}

LinphoneStatus MediaSessionPrivate::startUpdate (const string &subject) {
	fillMulticastMediaAddresses();
	if (!params->getPrivate()->getNoUserConsent())
		makeLocalMediaDescription();
	if (!core->sip_conf.sdp_200_ack)
		op->set_local_media_description(localDesc);
	else
		op->set_local_media_description(nullptr);
	LinphoneStatus result = CallSessionPrivate::startUpdate(subject);
	if (core->sip_conf.sdp_200_ack) {
		/* We are NOT offering, set local media description after sending the call so that we are ready to
		 * process the remote offer when it will arrive. */
		op->set_local_media_description(localDesc);
	}
	return result;
}

void MediaSessionPrivate::terminate () {
#if 0 // TODO: handle in Call class
	/* Stop ringing */
	bool_t stop_ringing = TRUE;
	bool_t ring_during_early_media = linphone_core_get_ring_during_incoming_early_media(lc);
	const bctbx_list_t *calls = linphone_core_get_calls(lc);
	while(calls) {
		if (((LinphoneCall *)calls->data)->state == LinphoneCallIncomingReceived || (ring_during_early_media && ((LinphoneCall *)calls->data)->state == LinphoneCallIncomingEarlyMedia)) {
			stop_ringing = FALSE;
			break;
		}
		calls = calls->next;
	}
	if(stop_ringing) {
		linphone_core_stop_ringing(lc);
	}
#endif

	stopStreams();
	CallSessionPrivate::terminate();
}

void MediaSessionPrivate::updateCurrentParams () const {
	CallSessionPrivate::updateCurrentParams();

	LinphoneVideoDefinition *vdef = linphone_video_definition_new(MS_VIDEO_SIZE_UNKNOWN_W, MS_VIDEO_SIZE_UNKNOWN_H, nullptr);
	currentParams->getPrivate()->setSentVideoDefinition(vdef);
	currentParams->getPrivate()->setReceivedVideoDefinition(vdef);
	linphone_video_definition_unref(vdef);
#ifdef VIDEO_ENABLED
	if (videoStream) {
		MSVideoSize vsize = video_stream_get_sent_video_size(videoStream);
		vdef = linphone_video_definition_new(static_cast<unsigned int>(vsize.width), static_cast<unsigned int>(vsize.height), nullptr);
		currentParams->getPrivate()->setSentVideoDefinition(vdef);
		linphone_video_definition_unref(vdef);
		vsize = video_stream_get_received_video_size(videoStream);
		vdef = linphone_video_definition_new(static_cast<unsigned int>(vsize.width), static_cast<unsigned int>(vsize.height), nullptr);
		currentParams->getPrivate()->setReceivedVideoDefinition(vdef);
		linphone_video_definition_unref(vdef);
		currentParams->getPrivate()->setSentFps(video_stream_get_sent_framerate(videoStream));
		currentParams->getPrivate()->setReceivedFps(video_stream_get_received_framerate(videoStream));
	}
#endif

	/* REVISITED
	 * Previous code was buggy.
	 * Relying on the mediastream's state (added by jehan: only) to know the current encryption is unreliable.
	 * For (added by jehan: both DTLS and) ZRTP it is though necessary.
	 * But for all others the current_params->media_encryption state should reflect (added by jehan: both) what is agreed by the offer/answer
	 * mechanism  (added by jehan: and encryption status from media which is much stronger than only result of offer/answer )
	 * Typically there can be inactive streams for which the media layer has no idea of whether they are encrypted or not.
	 */

	switch (params->getMediaEncryption()) {
		case LinphoneMediaEncryptionZRTP:
			if (atLeastOneStreamStarted()) {
				if (allStreamsEncrypted() && !authToken.empty())
					currentParams->setMediaEncryption(LinphoneMediaEncryptionZRTP);
				else {
					/* To avoid too many traces */
					lDebug() << "Encryption was requested to be " << linphone_media_encryption_to_string(params->getMediaEncryption())
						<< ", but isn't effective (allStreamsEncrypted=" << allStreamsEncrypted() << ", auth_token=" << authToken << ")";
					currentParams->setMediaEncryption(LinphoneMediaEncryptionNone);
				}
			} /* else don't update the state if all streams are shutdown */
			break;
		case LinphoneMediaEncryptionDTLS:
		case LinphoneMediaEncryptionSRTP:
			if (atLeastOneStreamStarted()) {
				if ((getNbActiveStreams() == 0) || allStreamsEncrypted())
					currentParams->setMediaEncryption(params->getMediaEncryption());
				else {
					/* To avoid to many traces */
					lDebug() << "Encryption was requested to be " << linphone_media_encryption_to_string(params->getMediaEncryption())
						<< ", but isn't effective (allStreamsEncrypted=" << allStreamsEncrypted() << ")";
					currentParams->setMediaEncryption(LinphoneMediaEncryptionNone);
				}
			} /* else don't update the state if all streams are shutdown */
			break;
		case LinphoneMediaEncryptionNone:
			/* Check if we actually switched to ZRTP */
			if (atLeastOneStreamStarted() && allStreamsEncrypted() && !authToken.empty())
				currentParams->setMediaEncryption(LinphoneMediaEncryptionZRTP);
			else
				currentParams->setMediaEncryption(LinphoneMediaEncryptionNone);
			break;
	}
	SalMediaDescription *md = resultDesc;
	currentParams->enableAvpf(allStreamsAvpfEnabled() && sal_media_description_has_avpf(md));
	if (currentParams->avpfEnabled())
		currentParams->setAvpfRrInterval(getAvpfRrInterval());
	else
		currentParams->setAvpfRrInterval(0);
	if (md) {
		SalStreamDescription *sd = sal_media_description_find_best_stream(md, SalAudio);
		currentParams->setAudioDirection(sd ? MediaSessionParamsPrivate::salStreamDirToMediaDirection(sd->dir) : LinphoneMediaDirectionInactive);
		if (currentParams->getAudioDirection() != LinphoneMediaDirectionInactive) {
			const char *rtpAddr = (sd->rtp_addr[0] != '\0') ? sd->rtp_addr : md->addr;
			currentParams->enableAudioMulticast(!!ms_is_multicast(rtpAddr));
		} else
			currentParams->enableAudioMulticast(false);
		sd = sal_media_description_find_best_stream(md, SalVideo);
		currentParams->getPrivate()->enableImplicitRtcpFb(sd && sal_stream_description_has_implicit_avpf(sd));
		currentParams->setVideoDirection(sd ? MediaSessionParamsPrivate::salStreamDirToMediaDirection(sd->dir) : LinphoneMediaDirectionInactive);
		if (currentParams->getVideoDirection() != LinphoneMediaDirectionInactive) {
			const char *rtpAddr = (sd->rtp_addr[0] != '\0') ? sd->rtp_addr : md->addr;
			currentParams->enableVideoMulticast(!!ms_is_multicast(rtpAddr));
		} else
			currentParams->enableVideoMulticast(false);
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::accept (const MediaSessionParams *csp) {
	if (csp) {
		params = new MediaSessionParams(*csp);
		iceAgent->prepare(localDesc, true);
		makeLocalMediaDescription();
		op->set_local_media_description(localDesc);
	}

	updateRemoteSessionIdAndVer();

	/* Give a chance a set card prefered sampling frequency */
	if (localDesc->streams[0].max_rate > 0) {
		lInfo() << "Configuring prefered card sampling rate to [" << localDesc->streams[0].max_rate << "]";
		if (core->sound_conf.play_sndcard)
			ms_snd_card_set_preferred_sample_rate(core->sound_conf.play_sndcard, localDesc->streams[0].max_rate);
		if (core->sound_conf.capt_sndcard)
			ms_snd_card_set_preferred_sample_rate(core->sound_conf.capt_sndcard, localDesc->streams[0].max_rate);
	}

#if 0
	if (!was_ringing && (audioStream->ms.state == MSStreamInitialized) && !core->use_files) {
		audio_stream_prepare_sound(audioStream, core->sound_conf.play_sndcard, core->sound_conf.capt_sndcard);
	}
#endif

	CallSessionPrivate::accept(params);

	SalMediaDescription *newMd = op->get_final_media_description();
	iceAgent->stopIceForInactiveStreams(newMd);
	if (newMd) {
		updateStreams(newMd, LinphoneCallStreamsRunning);
		setState(LinphoneCallStreamsRunning, "Connected (streams running)");
	} else
		expectMediaInAck = true;
}

LinphoneStatus MediaSessionPrivate::acceptUpdate (const CallSessionParams *csp, LinphoneCallState nextState, const string &stateInfo) {
	L_Q();
	SalMediaDescription *desc = op->get_remote_media_description();
	bool keepSdpVersion = !!lp_config_get_int(linphone_core_get_config(core), "sip", "keep_sdp_version", 0);
	if (keepSdpVersion && (desc->session_id == remoteSessionId) && (desc->session_ver == remoteSessionVer)) {
		/* Remote has sent an INVITE with the same SDP as before, so send a 200 OK with the same SDP as before. */
		lWarning() << "SDP version has not changed, send same SDP as before";
		op->accept();
		setState(nextState, stateInfo);
		return 0;
	}
	if (csp)
		params = new MediaSessionParams(*static_cast<const MediaSessionParams *>(csp));
	else {
		if (!op->is_offerer()) {
			/* Reset call params for multicast because this param is only relevant when offering */
			params->enableAudioMulticast(false);
			params->enableVideoMulticast(false);
		}
	}
	if (params->videoEnabled() && !linphone_core_video_enabled(core)) {
		lWarning() << "Requested video but video support is globally disabled. Refusing video";
		params->enableVideo(false);
	}
	if (q->getCurrentParams()->getPrivate()->getInConference()) {
		lWarning() << "Video isn't supported in conference";
		params->enableVideo(false);
	}
	/* Update multicast params according to call params */
	fillMulticastMediaAddresses();
	iceAgent->checkSession(IR_Controlled, true);
	initializeStreams(); /* So that video stream is initialized if necessary */
	if (iceAgent->prepare(localDesc, true))
		return 0; /* Deferred until completion of ICE gathering */
	startAcceptUpdate(nextState, stateInfo);
	return 0;
}

// -----------------------------------------------------------------------------

#ifdef VIDEO_ENABLED
void MediaSessionPrivate::videoStreamEventCb (const MSFilter *f, const unsigned int eventId, const void *args) {
	L_Q();
	switch (eventId) {
		case MS_VIDEO_DECODER_DECODING_ERRORS:
			lWarning() << "MS_VIDEO_DECODER_DECODING_ERRORS";
			if (videoStream && video_stream_is_decoding_error_to_be_reported(videoStream, 5000)) {
				video_stream_decoding_error_reported(videoStream);
				q->sendVfuRequest();
			}
			break;
		case MS_VIDEO_DECODER_RECOVERED_FROM_ERRORS:
			lInfo() << "MS_VIDEO_DECODER_RECOVERED_FROM_ERRORS";
			if (videoStream)
				video_stream_decoding_error_recovered(videoStream);
			break;
		case MS_VIDEO_DECODER_FIRST_IMAGE_DECODED:
			lInfo() << "First video frame decoded successfully";
			if (listener)
				listener->onFirstVideoFrameDecoded(q->getSharedFromThis());
			break;
		case MS_VIDEO_DECODER_SEND_PLI:
		case MS_VIDEO_DECODER_SEND_SLI:
		case MS_VIDEO_DECODER_SEND_RPSI:
			/* Handled internally by mediastreamer2 */
			break;
		default:
			lWarning() << "Unhandled event " << eventId;
			break;
	}
}
#endif

void MediaSessionPrivate::realTimeTextCharacterReceived (MSFilter *f, unsigned int id, void *arg) {
	if (id == MS_RTT_4103_RECEIVED_CHAR) {
#if 0
		RealtimeTextReceivedCharacter *data = reinterpret_cast<RealtimeTextReceivedCharacter *>(arg);
		LinphoneChatRoom * chat_room = linphone_call_get_chat_room(call);
		linphone_core_real_time_text_received(call->core, chat_room, data->character, call);
#endif
	}
}

// -----------------------------------------------------------------------------

void MediaSessionPrivate::stunAuthRequestedCb (const char *realm, const char *nonce, const char **username, const char **password, const char **ha1) {
	/* Get the username from the nat policy or the proxy config */
	LinphoneProxyConfig *proxy = nullptr;
	if (destProxy)
		proxy = destProxy;
	else
		proxy = linphone_core_get_default_proxy_config(core);
	if (!proxy)
		return;
	const char * user = nullptr;
	LinphoneNatPolicy *proxyNatPolicy = linphone_proxy_config_get_nat_policy(proxy);
	if (proxyNatPolicy)
		user = linphone_nat_policy_get_stun_server_username(proxyNatPolicy);
	else if (natPolicy)
		user = linphone_nat_policy_get_stun_server_username(natPolicy);
	if (!user) {
		/* If the username has not been found in the nat_policy, take the username from the currently used proxy config */
		const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(proxy);
		if (!addr)
			return;
		user = linphone_address_get_username(addr);
	}
	if (!user)
		return;

	const LinphoneAuthInfo *authInfo = linphone_core_find_auth_info(core, realm, user, nullptr);
	if (!authInfo) {
		lWarning() << "No auth info found for STUN auth request";
		return;
	}
	const char *hash = linphone_auth_info_get_ha1(authInfo);
	if (hash)
			*ha1 = hash;
	else
		*password = linphone_auth_info_get_passwd(authInfo);
	*username = user;
}

// =============================================================================

MediaSession::MediaSession (const Conference &conference, const CallSessionParams *params, CallSessionListener *listener)
	: CallSession(*new MediaSessionPrivate(conference, params, listener)) {
	L_D();
	d->iceAgent = new IceAgent(*this);
}

// -----------------------------------------------------------------------------

LinphoneStatus MediaSession::accept (const MediaSessionParams *msp) {
	L_D();
	LinphoneStatus result = d->checkForAcceptation();
	if (result < 0) return result;

#if 0
	bool_t was_ringing = FALSE;

	if (lc->current_call != call) {
		linphone_core_preempt_sound_resources(lc);
	}

	/* Stop ringing */
	if (linphone_ringtoneplayer_is_started(lc->ringtoneplayer)) {
		ms_message("Stop ringing");
		linphone_core_stop_ringing(lc);
		was_ringing = TRUE;
	}
	if (call->ringing_beep) {
		linphone_core_stop_dtmf(lc);
		call->ringing_beep = FALSE;
	}
#endif

	d->accept(msp);
	lInfo() << "CallSession accepted";
	return 0;
}

LinphoneStatus MediaSession::acceptEarlyMedia (const MediaSessionParams *msp) {
	L_D();
	if (d->state != LinphoneCallIncomingReceived) {
		lError() << "Bad state " << linphone_call_state_to_string(d->state) << " for MediaSession::acceptEarlyMedia()";
		return -1;
	}
	/* Try to be best-effort in giving real local or routable contact address for 100Rel case */
	d->setContactOp();
	/* If parameters are passed, update the media description */
	if (msp) {
		d->params = new MediaSessionParams(*msp);
		d->makeLocalMediaDescription();
		d->op->set_local_media_description(d->localDesc);
		d->op->set_sent_custom_header(d->params->getPrivate()->getCustomHeaders());
	}
	d->op->notify_ringing(true);
	d->setState(LinphoneCallIncomingEarlyMedia, "Incoming call early media");
	SalMediaDescription *md = d->op->get_final_media_description();
	if (md)
		d->updateStreams(md, d->state);
	return 0;
}

LinphoneStatus MediaSession::acceptUpdate (const MediaSessionParams *msp) {
	L_D();
	if (d->expectMediaInAck) {
		lError() << "MediaSession::acceptUpdate() is not possible during a late offer incoming reINVITE (INVITE without SDP)";
		return -1;
	}
	return CallSession::acceptUpdate(msp);
}

// -----------------------------------------------------------------------------

void MediaSession::configure (LinphoneCallDir direction, LinphoneProxyConfig *cfg, SalCallOp *op, const Address &from, const Address &to) {
	L_D();
	CallSession::configure (direction, cfg, op, from, to);

	if (d->destProxy)
		d->natPolicy = linphone_proxy_config_get_nat_policy(d->destProxy);
	if (!d->natPolicy)
		d->natPolicy = linphone_core_get_nat_policy(d->core);
	linphone_nat_policy_ref(d->natPolicy);

	if (direction == LinphoneCallOutgoing) {
		d->selectOutgoingIpVersion();
		d->getLocalIp(to);
		getCurrentParams()->getPrivate()->setUpdateCallWhenIceCompleted(d->params->getPrivate()->getUpdateCallWhenIceCompleted());
		d->fillMulticastMediaAddresses();
		if (d->natPolicy && linphone_nat_policy_ice_enabled(d->natPolicy))
			d->iceAgent->checkSession(IR_Controlling, false);
		d->runStunTestsIfNeeded();
		d->discoverMtu(to);
#if 0
		if (linphone_call_params_get_referer(params)){
			call->referer=linphone_call_ref(linphone_call_params_get_referer(params));
		}
#endif
	} else if (direction == LinphoneCallIncoming) {
		d->selectIncomingIpVersion();
		/* Note that the choice of IP version for streams is later refined by setCompatibleIncomingCallParams() when examining the
		 * remote offer, if any. If the remote offer contains IPv4 addresses, we should propose IPv4 as well. */
		Address cleanedFrom = from;
		cleanedFrom.clean();
		d->getLocalIp(cleanedFrom);
		d->params = new MediaSessionParams();
		d->params->initDefault(d->core);
		d->initializeParamsAccordingToIncomingCallParams();
		SalMediaDescription *md = d->op->get_remote_media_description();
		if (d->natPolicy && linphone_nat_policy_ice_enabled(d->natPolicy)) {
			if (md) {
				/* Create the ice session now if ICE is required */
				d->iceAgent->checkSession(IR_Controlled, false);
			} else {
				d->natPolicy = nullptr;
				lWarning() << "ICE not supported for incoming INVITE without SDP";
			}
		}
		if (d->natPolicy)
			d->runStunTestsIfNeeded();
		d->discoverMtu(cleanedFrom);
	}
}

void MediaSession::initiateIncoming () {
	L_D();
	CallSession::initiateIncoming();
	d->initializeStreams();
	if (d->natPolicy) {
		if (linphone_nat_policy_ice_enabled(d->natPolicy))
			d->deferIncomingNotification = d->iceAgent->prepare(d->localDesc, true);
	}
}

bool MediaSession::initiateOutgoing () {
	L_D();
	bool defer = CallSession::initiateOutgoing();
	d->initializeStreams();
	if (linphone_nat_policy_ice_enabled(d->natPolicy)) {
		if (d->core->sip_conf.sdp_200_ack)
			lWarning() << "ICE is not supported when sending INVITE without SDP";
		else {
			/* Defer the start of the call after the ICE gathering process */
			defer |= d->iceAgent->prepare(d->localDesc, false);
		}
	}
	return defer;
}

void MediaSession::iterate (time_t currentRealTime, bool oneSecondElapsed) {
	L_D();
	int elapsed = (int)(currentRealTime - d->log->start_date_time);
	d->executeBackgroundTasks(oneSecondElapsed);
	if ((d->state == LinphoneCallOutgoingInit) && (elapsed >= d->core->sip_conf.delayed_timeout)) {
		if (d->iceAgent->hasSession()) {
			lWarning() << "ICE candidates gathering from [" << linphone_nat_policy_get_stun_server(d->natPolicy) << "] has not finished yet, proceed with the call without ICE anyway";
			d->iceAgent->deleteSession();
		}
	}
	CallSession::iterate(currentRealTime, oneSecondElapsed);
}

LinphoneStatus MediaSession::pause () {
	L_D();
	LinphoneStatus result = d->pause();
	if (result == 0)
		d->pausedByApp = true;
	return result;
}

LinphoneStatus MediaSession::resume () {
	L_D();
	if (d->state != LinphoneCallPaused) {
		lWarning() << "we cannot resume a call that has not been established and paused before";
		return -1;
	}
	if (!d->params->getPrivate()->getInConference()) {
		if (linphone_core_sound_resources_locked(d->core)) {
			lWarning() << "Cannot resume MediaSession " << this << " because another call is locking the sound resources";
			return -1;
		}
		linphone_core_preempt_sound_resources(d->core);
		lInfo() << "Resuming MediaSession " << this;
	}
	d->automaticallyPaused = false;
#if 0
	call->broken = FALSE;
#endif
	/* Stop playing music immediately. If remote side is a conference it
	 * prevents the participants to hear it while the 200OK comes back. */
	if (d->audioStream)
		audio_stream_play(d->audioStream, nullptr);
	d->makeLocalMediaDescription();
	sal_media_description_set_dir(d->localDesc, SalStreamSendRecv);
	if (!d->core->sip_conf.sdp_200_ack)
		d->op->set_local_media_description(d->localDesc);
	else
		d->op->set_local_media_description(nullptr);
	string subject = "Call resuming";
	if (d->params->getPrivate()->getInConference() && !getCurrentParams()->getPrivate()->getInConference())
		subject = "Conference";
	if (d->op->update(subject.c_str(), false) != 0)
		return -1;
	d->setState(LinphoneCallResuming,"Resuming");
	if (!d->params->getPrivate()->getInConference() && d->listener)
		d->listener->onSetCurrentSession(getSharedFromThis());
	if (d->core->sip_conf.sdp_200_ack) {
		/* We are NOT offering, set local media description after sending the call so that we are ready to
		 * process the remote offer when it will arrive. */
		d->op->set_local_media_description(d->localDesc);
	}
	return 0;
}

void MediaSession::sendVfuRequest () {
#ifdef VIDEO_ENABLED
	L_D();
	MediaSessionParams *currentParams = getCurrentParams();
	if ((currentParams->avpfEnabled() || currentParams->getPrivate()->implicitRtcpFbEnabled())
		&& d->videoStream && media_stream_get_state(&d->videoStream->ms) == MSStreamStarted) { // || sal_media_description_has_implicit_avpf((const SalMediaDescription *)call->resultdesc)
		lInfo() << "Request Full Intra Request on CallSession [" << this << "]";
		video_stream_send_fir(d->videoStream);
	} else if (d->core->sip_conf.vfu_with_info) {
		lInfo() << "Request SIP INFO FIR on CallSession [" << this << "]";
		if (d->state == LinphoneCallStreamsRunning)
			d->op->send_vfu_request();
	} else
		lInfo() << "vfu request using sip disabled from config [sip,vfu_with_info]";
#endif
}

void MediaSession::startIncomingNotification () {
	L_D();
	d->makeLocalMediaDescription();
	d->op->set_local_media_description(d->localDesc);
	SalMediaDescription *md = d->op->get_final_media_description();
	if (md) {
		if (sal_media_description_empty(md) || linphone_core_incompatible_security(d->core, md)) {
			LinphoneErrorInfo *ei = linphone_error_info_new();
			linphone_error_info_set(ei, nullptr, LinphoneReasonNotAcceptable, 488, "Not acceptable here", nullptr);
#if 0
			linphone_core_report_early_failed_call(d->core, LinphoneCallIncoming, linphone_address_ref(from_addr), linphone_address_ref(to_addr), ei);
#endif
			d->op->decline(SalReasonNotAcceptable, nullptr);
#if 0
			linphone_call_unref(call);
#endif
			return;
		}
	}

	CallSession::startIncomingNotification();
}

int MediaSession::startInvite (const Address *destination, const string &subject, const Content *content) {
	L_D();
	linphone_core_stop_dtmf_stream(d->core);
	d->makeLocalMediaDescription();
	if (d->core->ringstream && d->core->sound_conf.play_sndcard && d->core->sound_conf.capt_sndcard) {
		/* Give a chance to set card prefered sampling frequency */
		if (d->localDesc->streams[0].max_rate > 0)
			ms_snd_card_set_preferred_sample_rate(d->core->sound_conf.play_sndcard, d->localDesc->streams[0].max_rate);
		if (!d->core->use_files)
			audio_stream_prepare_sound(d->audioStream, d->core->sound_conf.play_sndcard, d->core->sound_conf.capt_sndcard);
	}
	if (!d->core->sip_conf.sdp_200_ack) {
		/* We are offering, set local media description before sending the call */
		d->op->set_local_media_description(d->localDesc);
	}

	int result = CallSession::startInvite(destination, subject, content);
	if (result < 0) {
		if (d->state == LinphoneCallError)
			d->stopStreams();
		return result;
	}
	if (d->core->sip_conf.sdp_200_ack) {
		/* We are NOT offering, set local media description after sending the call so that we are ready to
		   process the remote offer when it will arrive. */
		d->op->set_local_media_description(d->localDesc);
	}
	return result;
}

void MediaSession::startRecording () {
	L_D();
	if (d->params->getRecordFilePath().empty()) {
		lError() << "MediaSession::startRecording(): no output file specified. Use linphone_call_params_set_record_file()";
		return;
	}
	if (d->audioStream && !d->params->getPrivate()->getInConference())
		audio_stream_mixed_record_start(d->audioStream);
	d->recordActive = true;
}

void MediaSession::stopRecording () {
	L_D();
	if (d->audioStream && !d->params->getPrivate()->getInConference())
		audio_stream_mixed_record_stop(d->audioStream);
	d->recordActive = false;
}

LinphoneStatus MediaSession::update (const MediaSessionParams *msp, const string &subject) {
	L_D();
	LinphoneCallState nextState;
	LinphoneCallState initialState = d->state;
	if (!d->isUpdateAllowed(nextState))
		return -1;
	if (d->currentParams == msp)
		lWarning() << "CallSession::update() is given the current params, this is probably not what you intend to do!";
	d->iceAgent->checkSession(IR_Controlling, true);
	if (msp) {
#if 0
		call->broken = FALSE;
#endif
		d->setState(nextState, "Updating call");
		d->params = new MediaSessionParams(*msp);
		if (d->iceAgent->prepare(d->localDesc, false)) {
			lInfo() << "Defer CallSession update to gather ICE candidates";
			return 0;
		}
		LinphoneStatus result = d->startUpdate(subject);
		if (result && (d->state != initialState)) {
			/* Restore initial state */
			d->setState(initialState, "Restore initial state");
		}
	} else {
#ifdef VIDEO_ENABLED
		if (d->videoStream && (d->state == LinphoneCallStreamsRunning)) {
			const LinphoneVideoDefinition *vdef = linphone_core_get_preferred_video_definition(d->core);
			MSVideoSize vsize;
			vsize.width = static_cast<int>(linphone_video_definition_get_width(vdef));
			vsize.height = static_cast<int>(linphone_video_definition_get_height(vdef));
			video_stream_set_sent_video_size(d->videoStream, vsize);
			video_stream_set_fps(d->videoStream, linphone_core_get_preferred_framerate(d->core));
			if (d->cameraEnabled && (d->videoStream->cam != d->core->video_conf.device))
				video_stream_change_camera(d->videoStream, d->core->video_conf.device);
			else
				video_stream_update_video_params(d->videoStream);
		}
#endif
	}
	return 0;
}

// -----------------------------------------------------------------------------

void MediaSession::resetFirstVideoFrameDecoded () {
	L_D();
	if (d->videoStream && d->videoStream->ms.decoder)
		ms_filter_call_method_noarg(d->videoStream->ms.decoder, MS_VIDEO_DECODER_RESET_FIRST_IMAGE_NOTIFICATION);
}

LinphoneStatus MediaSession::takePreviewSnapshot (const string& file) {
#ifdef VIDEO_ENABLED
	L_D();
	if (d->videoStream && d->videoStream->local_jpegwriter) {
		const char *filepath = file.empty() ? nullptr : file.c_str();
		return ms_filter_call_method(d->videoStream->local_jpegwriter, MS_JPEG_WRITER_TAKE_SNAPSHOT, (void *)filepath);
	}
	lWarning() << "Cannot take local snapshot: no currently running video stream on this call";
#endif
	return -1;
}

LinphoneStatus MediaSession::takeVideoSnapshot (const string& file) {
#ifdef VIDEO_ENABLED
	L_D();
	if (d->videoStream && d->videoStream->jpegwriter) {
		const char *filepath = file.empty() ? nullptr : file.c_str();
		return ms_filter_call_method(d->videoStream->jpegwriter, MS_JPEG_WRITER_TAKE_SNAPSHOT, (void *)filepath);
	}
	lWarning() << "Cannot take snapshot: no currently running video stream on this call";
#endif
	return -1;
}

void MediaSession::zoomVideo (float zoomFactor, float *cx, float *cy) {
	zoomVideo(zoomFactor, *cx, *cy);
}

void MediaSession::zoomVideo (float zoomFactor, float cx, float cy) {
	L_D();
	if (d->videoStream && d->videoStream->output) {
		if (zoomFactor < 1)
			zoomFactor = 1;
		float halfsize = 0.5f * 1.0f / zoomFactor;
		if ((cx - halfsize) < 0)
			cx = 0 + halfsize;
		if ((cx + halfsize) > 1)
			cx = 1 - halfsize;
		if ((cy - halfsize) < 0)
			cy = 0 + halfsize;
		if ((cy + halfsize) > 1)
			cy = 1 - halfsize;
		float zoom[3] = { zoomFactor, cx, cy };
		ms_filter_call_method(d->videoStream->output, MS_VIDEO_DISPLAY_ZOOM, &zoom);
	} else
		lWarning() << "Could not apply zoom: video output wasn't activated";
}

// -----------------------------------------------------------------------------

bool MediaSession::cameraEnabled () const {
	L_D();
	return d->cameraEnabled;
}

bool MediaSession::echoCancellationEnabled () const {
	L_D();
	if (!d->audioStream || !d->audioStream->ec)
		return !!linphone_core_echo_cancellation_enabled(d->core);

	bool val;
	ms_filter_call_method(d->audioStream->ec, MS_ECHO_CANCELLER_GET_BYPASS_MODE, &val);
	return !val;
}

bool MediaSession::echoLimiterEnabled () const {
	L_D();
	if (d->audioStream)
		return d->audioStream->el_type !=ELInactive;
	return !!linphone_core_echo_limiter_enabled(d->core);
}

void MediaSession::enableCamera (bool value) {
#ifdef VIDEO_ENABLED
	L_D();
	d->cameraEnabled = value;
	switch (d->state) {
		case LinphoneCallStreamsRunning:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallConnected:
			if (d->videoStream && video_stream_started(d->videoStream) && (video_stream_get_camera(d->videoStream) != d->getVideoDevice())) {
				string currentCam = video_stream_get_camera(d->videoStream) ? ms_web_cam_get_name(video_stream_get_camera(d->videoStream)) : "NULL";
				string newCam = d->getVideoDevice() ? ms_web_cam_get_name(d->getVideoDevice()) : "NULL";
				lInfo() << "Switching video cam from [" << currentCam << "] to [" << newCam << "] on CallSession [" << this << "]";
				video_stream_change_camera(d->videoStream, d->getVideoDevice());
			}
			break;
		default:
			break;
	}
#endif
}

void MediaSession::enableEchoCancellation (bool value) {
	L_D();
	if (d->audioStream && d->audioStream->ec) {
		bool bypassMode = !value;
		ms_filter_call_method(d->audioStream->ec, MS_ECHO_CANCELLER_SET_BYPASS_MODE, &bypassMode);
	}
}

void MediaSession::enableEchoLimiter (bool value) {
	L_D();
	if (d->audioStream) {
		if (value) {
			string type = lp_config_get_string(linphone_core_get_config(d->core), "sound", "el_type", "mic");
			if (type == "mic")
				audio_stream_enable_echo_limiter(d->audioStream, ELControlMic);
			else if (type == "full")
				audio_stream_enable_echo_limiter(d->audioStream, ELControlFull);
		} else
			audio_stream_enable_echo_limiter(d->audioStream, ELInactive);
	}
}

bool MediaSession::getAllMuted () const {
	L_D();
	return d->allMuted;
}

LinphoneCallStats * MediaSession::getAudioStats () const {
	return getStats(LinphoneStreamTypeAudio);
}

string MediaSession::getAuthenticationToken () const {
	L_D();
	return d->authToken;
}

bool MediaSession::getAuthenticationTokenVerified () const {
	L_D();
	return d->authTokenVerified;
}

float MediaSession::getAverageQuality () const {
	L_D();
	float audioRating = -1.f;
	float videoRating = -1.f;
	if (d->audioStream)
		audioRating = media_stream_get_average_quality_rating(&d->audioStream->ms) / 5.0f;
	if (d->videoStream)
		videoRating = media_stream_get_average_quality_rating(&d->videoStream->ms) / 5.0f;
	return MediaSessionPrivate::aggregateQualityRatings(audioRating, videoRating);
}

MediaSessionParams * MediaSession::getCurrentParams () const {
	L_D();
	d->updateCurrentParams();
	return d->currentParams;
}

float MediaSession::getCurrentQuality () const {
	L_D();
	float audioRating = -1.f;
	float videoRating = -1.f;
	if (d->audioStream)
		audioRating = media_stream_get_quality_rating(&d->audioStream->ms) / 5.0f;
	if (d->videoStream)
		videoRating = media_stream_get_quality_rating(&d->videoStream->ms) / 5.0f;
	return MediaSessionPrivate::aggregateQualityRatings(audioRating, videoRating);
}

const MediaSessionParams * MediaSession::getMediaParams () const {
	L_D();
	return d->params;
}

RtpTransport * MediaSession::getMetaRtcpTransport (int streamIndex) const {
	L_D();
	if ((streamIndex < 0) || (streamIndex >= getStreamCount()))
		return nullptr;
	RtpTransport *metaRtp;
	RtpTransport *metaRtcp;
	rtp_session_get_transports(d->sessions[streamIndex].rtp_session, &metaRtp, &metaRtcp);
	return metaRtcp;
}

RtpTransport * MediaSession::getMetaRtpTransport (int streamIndex) const {
	L_D();
	if ((streamIndex < 0) || (streamIndex >= getStreamCount()))
		return nullptr;
	RtpTransport *metaRtp;
	RtpTransport *metaRtcp;
	rtp_session_get_transports(d->sessions[streamIndex].rtp_session, &metaRtp, &metaRtcp);
	return metaRtp;
}

float MediaSession::getMicrophoneVolumeGain () const {
	L_D();
	if (d->audioStream)
		return audio_stream_get_sound_card_input_gain(d->audioStream);
	else {
		lError() << "Could not get record volume: no audio stream";
		return -1.0f;
	}
}

void * MediaSession::getNativeVideoWindowId () const {
	L_D();
	if (d->videoWindowId) {
		/* The video id was previously set by the app */
		return d->videoWindowId;
	}
#ifdef VIDEO_ENABLED
	else if (d->videoStream) {
		/* It was not set but we want to get the one automatically created by mediastreamer2 (desktop versions only) */
		return video_stream_get_native_window_id(d->videoStream);
	}
#endif
	return nullptr;
}

const CallSessionParams * MediaSession::getParams () const {
	L_D();
	return d->params;
}

float MediaSession::getPlayVolume () const {
	L_D();
	if (d->audioStream && d->audioStream->volrecv) {
		float vol = 0;
		ms_filter_call_method(d->audioStream->volrecv, MS_VOLUME_GET, &vol);
		return vol;
	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

float MediaSession::getRecordVolume () const {
	L_D();
	if (d->audioStream && d->audioStream->volsend && !d->audioMuted && (d->state == LinphoneCallStreamsRunning)) {
		float vol = 0;
		ms_filter_call_method(d->audioStream->volsend, MS_VOLUME_GET, &vol);
		return vol;
	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

const MediaSessionParams * MediaSession::getRemoteParams () {
	L_D();
	if (d->op){
		SalMediaDescription *md = d->op->get_remote_media_description();
		if (md) {
			if (d->remoteParams)
				delete d->remoteParams;
			d->remoteParams = new MediaSessionParams();
			unsigned int nbAudioStreams = sal_media_description_nb_active_streams_of_type(md, SalAudio);
			for (unsigned int i = 0; i < nbAudioStreams; i++) {
				SalStreamDescription *sd = sal_media_description_get_active_stream_of_type(md, SalAudio, i);
				if (sal_stream_description_has_srtp(sd))
					d->remoteParams->setMediaEncryption(LinphoneMediaEncryptionSRTP);
			}
			unsigned int nbVideoStreams = sal_media_description_nb_active_streams_of_type(md, SalVideo);
			for (unsigned int i = 0; i < nbVideoStreams; i++) {
				SalStreamDescription *sd = sal_media_description_get_active_stream_of_type(md, SalVideo, i);
				if (sal_stream_description_active(sd))
					d->remoteParams->enableVideo(true);
				if (sal_stream_description_has_srtp(sd))
					d->remoteParams->setMediaEncryption(LinphoneMediaEncryptionSRTP);
			}
			unsigned int nbTextStreams = sal_media_description_nb_active_streams_of_type(md, SalText);
			for (unsigned int i = 0; i < nbTextStreams; i++) {
				SalStreamDescription *sd = sal_media_description_get_active_stream_of_type(md, SalText, i);
				if (sal_stream_description_has_srtp(sd))
					d->remoteParams->setMediaEncryption(LinphoneMediaEncryptionSRTP);
				d->remoteParams->enableRealtimeText(true);
			}
			if (!d->remoteParams->videoEnabled()) {
				if ((md->bandwidth > 0) && (md->bandwidth <= linphone_core_get_edge_bw(d->core)))
					d->remoteParams->enableLowBandwidth(true);
			}
			if (md->name[0] != '\0')
				d->remoteParams->setSessionName(md->name);

			d->remoteParams->getPrivate()->setCustomSdpAttributes(md->custom_sdp_attributes);
			d->remoteParams->getPrivate()->setCustomSdpMediaAttributes(LinphoneStreamTypeAudio, md->streams[d->mainAudioStreamIndex].custom_sdp_attributes);
			d->remoteParams->getPrivate()->setCustomSdpMediaAttributes(LinphoneStreamTypeVideo, md->streams[d->mainVideoStreamIndex].custom_sdp_attributes);
			d->remoteParams->getPrivate()->setCustomSdpMediaAttributes(LinphoneStreamTypeText, md->streams[d->mainTextStreamIndex].custom_sdp_attributes);
		}
		const SalCustomHeader *ch = d->op->get_recv_custom_header();
		if (ch) {
			/* Instanciate a remote_params only if a SIP message was received before (custom headers indicates this) */
			if (!d->remoteParams)
				d->remoteParams = new MediaSessionParams();
			d->remoteParams->getPrivate()->setCustomHeaders(ch);
		}
		return d->remoteParams;
	}
	return nullptr;
}

float MediaSession::getSpeakerVolumeGain () const {
	L_D();
	if (d->audioStream)
		return audio_stream_get_sound_card_output_gain(d->audioStream);
	else {
		lError() << "Could not get playback volume: no audio stream";
		return -1.0f;
	}
}

LinphoneCallStats * MediaSession::getStats (LinphoneStreamType type) const {
	L_D();
	if (type == LinphoneStreamTypeUnknown)
		return nullptr;
	LinphoneCallStats *stats = nullptr;
	LinphoneCallStats *statsCopy = _linphone_call_stats_new();
	if (type == LinphoneStreamTypeAudio)
		stats = d->audioStats;
	else if (type == LinphoneStreamTypeVideo)
		stats = d->videoStats;
	else if (type == LinphoneStreamTypeText)
		stats = d->textStats;
	MediaStream *ms = d->getMediaStream(type);
	if (ms && stats)
		linphone_call_stats_update(stats, ms);
	_linphone_call_stats_clone(statsCopy, stats);
	return statsCopy;
}

int MediaSession::getStreamCount () const {
	/* TODO: Revisit when multiple media streams will be implemented */
#ifdef VIDEO_ENABLED
	if (getCurrentParams()->realtimeTextEnabled())
		return 3;
	return 2;
#else
	if (getCurrentParams()->realtimeTextEnabled())
		return 2;
	return 1;
#endif
}

MSFormatType MediaSession::getStreamType (int streamIndex) const {
	L_D();
	/* TODO: Revisit when multiple media streams will be implemented */
	if (streamIndex == d->mainVideoStreamIndex)
		return MSVideo;
	else if (streamIndex == d->mainTextStreamIndex)
		return MSText;
	else if (streamIndex == d->mainAudioStreamIndex)
		return MSAudio;
	return MSUnknownMedia;
}

LinphoneCallStats * MediaSession::getTextStats () const {
	return getStats(LinphoneStreamTypeText);
}

LinphoneCallStats * MediaSession::getVideoStats () const {
	return getStats(LinphoneStreamTypeVideo);
}

bool MediaSession::mediaInProgress () const {
	L_D();
	if ((linphone_call_stats_get_ice_state(d->audioStats) == LinphoneIceStateInProgress)
		|| (linphone_call_stats_get_ice_state(d->videoStats) == LinphoneIceStateInProgress)
		|| (linphone_call_stats_get_ice_state(d->textStats) == LinphoneIceStateInProgress))
		return true;
	/* TODO: could check zrtp state */
	return false;
}

void MediaSession::setAuthenticationTokenVerified (bool value) {
	L_D();
	if (!d->audioStream || !media_stream_started(&d->audioStream->ms)) {
		lError() << "MediaSession::setAuthenticationTokenVerified(): No audio stream or not started";
		return;
	}
	if (!d->audioStream->ms.sessions.zrtp_context) {
		lError() << "MediaSession::setAuthenticationTokenVerified(): No zrtp context";
		return;
	}
	if (!d->authTokenVerified && value)
		ms_zrtp_sas_verified(d->audioStream->ms.sessions.zrtp_context);
	else if (d->authTokenVerified && !value)
		ms_zrtp_sas_reset_verified(d->audioStream->ms.sessions.zrtp_context);
	d->authTokenVerified = value;
	d->propagateEncryptionChanged();
}

void MediaSession::setMicrophoneVolumeGain (float value) {
	L_D();
	if(d->audioStream)
		audio_stream_set_sound_card_input_gain(d->audioStream, value);
	else
		lError() << "Could not set record volume: no audio stream";
}

void MediaSession::setNativeVideoWindowId (void *id) {
	L_D();
	d->videoWindowId = id;
#ifdef VIDEO_ENABLED
	if (d->videoStream)
		video_stream_set_native_window_id(d->videoStream, id);
#endif
}

void MediaSession::setSpeakerVolumeGain (float value) {
	L_D();
	if (d->audioStream)
		audio_stream_set_sound_card_output_gain(d->audioStream, value);
	else
		lError() << "Could not set playback volume: no audio stream";
}

LINPHONE_END_NAMESPACE
