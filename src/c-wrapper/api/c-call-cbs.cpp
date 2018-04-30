/*
 * c-call-cbs.cpp
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

#include "linphone/api/c-call-cbs.h"

#include "c-wrapper/c-wrapper.h"

// =============================================================================

struct _LinphoneCallCbs {
	belle_sip_object_t base;
	void *userData;
	LinphoneCallCbsDtmfReceivedCb dtmfReceivedCb;
	LinphoneCallCbsEncryptionChangedCb encryptionChangedCb;
	LinphoneCallCbsInfoMessageReceivedCb infoMessageReceivedCb;
	LinphoneCallCbsStateChangedCb stateChangedCb;
	LinphoneCallCbsStatsUpdatedCb statsUpdatedCb;
	LinphoneCallCbsTransferStateChangedCb transferStateChangedCb;
	LinphoneCallCbsAckProcessingCb ackProcessing;
	LinphoneCallCbsTmmbrReceivedCb tmmbrReceivedCb;
	LinphoneCallCbsSnapshotTakenCb snapshotTakenCb;
	LinphoneCallCbsNextVideoFrameDecodedCb nextVideoFrameDecodedCb;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneCallCbs);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

// =============================================================================

LinphoneCallCbs *_linphone_call_cbs_new (void) {
	return belle_sip_object_new(LinphoneCallCbs);
}

LinphoneCallCbs *linphone_call_cbs_ref (LinphoneCallCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_call_cbs_unref (LinphoneCallCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_call_cbs_get_user_data (const LinphoneCallCbs *cbs) {
	return cbs->userData;
}

void linphone_call_cbs_set_user_data (LinphoneCallCbs *cbs, void *ud) {
	cbs->userData = ud;
}

LinphoneCallCbsDtmfReceivedCb linphone_call_cbs_get_dtmf_received (LinphoneCallCbs *cbs) {
	return cbs->dtmfReceivedCb;
}

void linphone_call_cbs_set_dtmf_received (LinphoneCallCbs *cbs, LinphoneCallCbsDtmfReceivedCb cb) {
	cbs->dtmfReceivedCb = cb;
}

LinphoneCallCbsEncryptionChangedCb linphone_call_cbs_get_encryption_changed (LinphoneCallCbs *cbs) {
	return cbs->encryptionChangedCb;
}

void linphone_call_cbs_set_encryption_changed (LinphoneCallCbs *cbs, LinphoneCallCbsEncryptionChangedCb cb) {
	cbs->encryptionChangedCb = cb;
}

LinphoneCallCbsInfoMessageReceivedCb linphone_call_cbs_get_info_message_received (LinphoneCallCbs *cbs) {
	return cbs->infoMessageReceivedCb;
}

void linphone_call_cbs_set_info_message_received (LinphoneCallCbs *cbs, LinphoneCallCbsInfoMessageReceivedCb cb) {
	cbs->infoMessageReceivedCb = cb;
}

LinphoneCallCbsStateChangedCb linphone_call_cbs_get_state_changed (LinphoneCallCbs *cbs) {
	return cbs->stateChangedCb;
}

void linphone_call_cbs_set_state_changed (LinphoneCallCbs *cbs, LinphoneCallCbsStateChangedCb cb) {
	cbs->stateChangedCb = cb;
}

LinphoneCallCbsStatsUpdatedCb linphone_call_cbs_get_stats_updated (LinphoneCallCbs *cbs) {
	return cbs->statsUpdatedCb;
}

void linphone_call_cbs_set_stats_updated (LinphoneCallCbs *cbs, LinphoneCallCbsStatsUpdatedCb cb) {
	cbs->statsUpdatedCb = cb;
}

LinphoneCallCbsTransferStateChangedCb linphone_call_cbs_get_transfer_state_changed (LinphoneCallCbs *cbs) {
	return cbs->transferStateChangedCb;
}

void linphone_call_cbs_set_transfer_state_changed (LinphoneCallCbs *cbs, LinphoneCallCbsTransferStateChangedCb cb) {
	cbs->transferStateChangedCb = cb;
}

LinphoneCallCbsAckProcessingCb linphone_call_cbs_get_ack_processing (LinphoneCallCbs *cbs){
	return cbs->ackProcessing;
}

void linphone_call_cbs_set_ack_processing (LinphoneCallCbs *cbs, LinphoneCallCbsAckProcessingCb cb){
	cbs->ackProcessing = cb;
}

LinphoneCallCbsTmmbrReceivedCb linphone_call_cbs_get_tmmbr_received (LinphoneCallCbs *cbs) {
	return cbs->tmmbrReceivedCb;
}

void linphone_call_cbs_set_tmmbr_received (LinphoneCallCbs *cbs, LinphoneCallCbsTmmbrReceivedCb cb) {
	cbs->tmmbrReceivedCb = cb;
}

LinphoneCallCbsSnapshotTakenCb linphone_call_cbs_get_snapshot_taken(LinphoneCallCbs *cbs) {
	return cbs->snapshotTakenCb;
}

void linphone_call_cbs_set_snapshot_taken(LinphoneCallCbs *cbs, LinphoneCallCbsSnapshotTakenCb cb) {
	cbs->snapshotTakenCb = cb;
}

LinphoneCallCbsNextVideoFrameDecodedCb linphone_call_cbs_get_next_video_frame_decoded(LinphoneCallCbs *cbs) {
	return cbs->nextVideoFrameDecodedCb;
}

void linphone_call_cbs_set_next_video_frame_decoded(LinphoneCallCbs *cbs, LinphoneCallCbsNextVideoFrameDecodedCb cb) {
	cbs->nextVideoFrameDecodedCb = cb;
}
