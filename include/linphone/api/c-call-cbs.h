/*
 * c-call-cbs.h
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

#ifndef _L_C_CALL_CBS_H_
#define _L_C_CALL_CBS_H_

#include "linphone/api/c-callbacks.h"
#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup call_control
 * @{
 */

/**
 * Acquire a reference to the #LinphoneCallCbs object.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The same #LinphoneCallCbs object.
 */
LINPHONE_PUBLIC LinphoneCallCbs *linphone_call_cbs_ref (LinphoneCallCbs *cbs);

/**
 * Release reference to the #LinphoneCallCbs object.
 * @param[in] cbs #LinphoneCallCbs object.
 */
LINPHONE_PUBLIC void linphone_call_cbs_unref (LinphoneCallCbs *cbs);

/**
 * Retrieve the user pointer associated with the #LinphoneCallCbs object.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The user pointer associated with the #LinphoneCallCbs object.
 */
LINPHONE_PUBLIC void *linphone_call_cbs_get_user_data (const LinphoneCallCbs *cbs);

/**
 * Assign a user pointer to the #LinphoneCallCbs object.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] ud The user pointer to associate with the #LinphoneCallCbs object.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_user_data (LinphoneCallCbs *cbs, void *ud);

/**
 * Get the dtmf received callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current dtmf received callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsDtmfReceivedCb linphone_call_cbs_get_dtmf_received (LinphoneCallCbs *cbs);

/**
 * Set the dtmf received callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The dtmf received callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_dtmf_received (LinphoneCallCbs *cbs, LinphoneCallCbsDtmfReceivedCb cb);

/**
 * Get the encryption changed callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current encryption changed callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsEncryptionChangedCb linphone_call_cbs_get_encryption_changed (LinphoneCallCbs *cbs);

/**
 * Set the encryption changed callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The encryption changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_encryption_changed (LinphoneCallCbs *cbs, LinphoneCallCbsEncryptionChangedCb cb);

/**
 * Get the info message received callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current info message received callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsInfoMessageReceivedCb linphone_call_cbs_get_info_message_received (LinphoneCallCbs *cbs);

/**
 * Set the info message received callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The info message received callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_info_message_received (LinphoneCallCbs *cbs, LinphoneCallCbsInfoMessageReceivedCb cb);

/**
 * Get the state changed callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current state changed callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsStateChangedCb linphone_call_cbs_get_state_changed (LinphoneCallCbs *cbs);

/**
 * Set the state changed callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_state_changed (LinphoneCallCbs *cbs, LinphoneCallCbsStateChangedCb cb);

/**
 * Get the stats updated callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current stats updated callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsStatsUpdatedCb linphone_call_cbs_get_stats_updated (LinphoneCallCbs *cbs);

/**
 * Set the stats updated callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The stats updated callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_stats_updated (LinphoneCallCbs *cbs, LinphoneCallCbsStatsUpdatedCb cb);

/**
 * Get the transfer state changed callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current transfer state changed callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsTransferStateChangedCb linphone_call_cbs_get_transfer_state_changed (LinphoneCallCbs *cbs);

/**
 * Set the transfer state changed callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The transfer state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_transfer_state_changed (LinphoneCallCbs *cbs, LinphoneCallCbsTransferStateChangedCb cb);

/**
 * Get the ACK processing callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @return The current ack processing callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsAckProcessingCb linphone_call_cbs_get_ack_processing (LinphoneCallCbs *cbs);

/**
 * Set ACK processing callback.
 * @param[in] cbs #LinphoneCallCbs object.
 * @param[in] cb The ack processing callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_ack_processing (LinphoneCallCbs *cbs, LinphoneCallCbsAckProcessingCb cb);

/**
 * Get the TMMBR received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current TMMBR received callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsTmmbrReceivedCb linphone_call_cbs_get_tmmbr_received(LinphoneCallCbs *cbs);

/**
 * Set the TMMBR received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The TMMBR received callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_tmmbr_received(LinphoneCallCbs *cbs, LinphoneCallCbsTmmbrReceivedCb cb);

/**
 * Get the snapshot taken callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current snapshot taken callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsSnapshotTakenCb linphone_call_cbs_get_snapshot_taken(LinphoneCallCbs *cbs);

/**
 * Set the snapshot taken callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The snapshot taken callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_snapshot_taken(LinphoneCallCbs *cbs, LinphoneCallCbsSnapshotTakenCb cb);

 /**
 * Get the next video frame decoded callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current next video frame decoded callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsNextVideoFrameDecodedCb linphone_call_cbs_get_next_video_frame_decoded(LinphoneCallCbs *cbs);

/**
 * Set the next video frame decoded callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The next video frame decoded callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_next_video_frame_decoded(LinphoneCallCbs *cbs, LinphoneCallCbsNextVideoFrameDecodedCb cb);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_CALL_CBS_H_
