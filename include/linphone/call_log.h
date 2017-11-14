/*
linphone
Copyright (C) 2010-2014  Belledonne Communications SARL

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef __LINPHONE_CALL_LOG_H__
#define __LINPHONE_CALL_LOG_H__

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup call_logs
 * @{
**/

/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

/**
 * Get the call ID used by the call.
 * @param[in] cl LinphoneCallLog object
 * @return The call ID used by the call as a string.
**/
LINPHONE_PUBLIC const char * linphone_call_log_get_call_id(const LinphoneCallLog *cl);

/**
 * Get the direction of the call.
 * @param[in] cl LinphoneCallLog object
 * @return The direction of the call.
**/
LINPHONE_PUBLIC LinphoneCallDir linphone_call_log_get_dir(LinphoneCallLog *cl);

/**
 * Get the duration of the call since connected.
 * @param[in] cl LinphoneCallLog object
 * @return The duration of the call in seconds.
**/
LINPHONE_PUBLIC int linphone_call_log_get_duration(LinphoneCallLog *cl);

/**
 * Get the origin address (ie from) of the call.
 * @param[in] cl LinphoneCallLog object
 * @return The origin address (ie from) of the call.
**/
LINPHONE_PUBLIC LinphoneAddress * linphone_call_log_get_from_address(LinphoneCallLog *cl);

/**
 * Get the RTP statistics computed locally regarding the call.
 * @param[in] cl LinphoneCallLog object
 * @return The RTP statistics that have been computed locally for the call.
**/
LINPHONE_PUBLIC const rtp_stats_t * linphone_call_log_get_local_stats(const LinphoneCallLog *cl);

/**
 * Get the overall quality indication of the call.
 * @param[in] cl LinphoneCallLog object
 * @return The overall quality indication of the call.
**/
LINPHONE_PUBLIC float linphone_call_log_get_quality(LinphoneCallLog *cl);

/**
 * Get the persistent reference key associated to the call log.
 *
 * The reference key can be for example an id to an external database.
 * It is stored in the config file, thus can survive to process exits/restarts.
 *
 * @param[in] cl LinphoneCallLog object
 * @return The reference key string that has been associated to the call log, or NULL if none has been associated.
**/
LINPHONE_PUBLIC const char * linphone_call_log_get_ref_key(const LinphoneCallLog *cl);

/**
 * Get the remote address (that is from or to depending on call direction).
 * @param[in] cl LinphoneCallLog object
 * @return The remote address of the call.
**/
LINPHONE_PUBLIC LinphoneAddress * linphone_call_log_get_remote_address(LinphoneCallLog *cl);

/**
 * Get the RTP statistics computed by the remote end and sent back via RTCP.
 * @note Not implemented yet.
 * @param[in] cl LinphoneCallLog object
 * @return The RTP statistics that have been computed by the remote end for the call.
**/
LINPHONE_PUBLIC const rtp_stats_t * linphone_call_log_get_remote_stats(const LinphoneCallLog *cl);

/**
 * Get the start date of the call.
 * @param[in] cl LinphoneCallLog object
 * @return The date of the beginning of the call.
**/
LINPHONE_PUBLIC time_t linphone_call_log_get_start_date(LinphoneCallLog *cl);

/**
 * Get the status of the call.
 * @param[in] cl LinphoneCallLog object
 * @return The status of the call.
**/
LINPHONE_PUBLIC LinphoneCallStatus linphone_call_log_get_status(LinphoneCallLog *cl);

/**
 * Get the destination address (ie to) of the call.
 * @param[in] cl LinphoneCallLog object
 * @return The destination address (ie to) of the call.
**/
LINPHONE_PUBLIC LinphoneAddress * linphone_call_log_get_to_address(LinphoneCallLog *cl);

/**
 * Associate a persistent reference key to the call log.
 *
 * The reference key can be for example an id to an external database.
 * It is stored in the config file, thus can survive to process exits/restarts.
 *
 * @param[in] cl LinphoneCallLog object
 * @param[in] refkey The reference key string to associate to the call log.
**/
LINPHONE_PUBLIC void linphone_call_log_set_ref_key(LinphoneCallLog *cl, const char *refkey);

/**
 * Tell whether video was enabled at the end of the call or not.
 * @param[in] cl LinphoneCallLog object
 * @return A boolean value telling whether video was enabled at the end of the call.
**/
LINPHONE_PUBLIC bool_t linphone_call_log_video_enabled(LinphoneCallLog *cl);

/**
 * Get a human readable string describing the call.
 * @note: the returned string must be freed by the application (use ms_free()).
 * @param[in] cl LinphoneCallLog object
 * @return A human readable string describing the call.
**/
LINPHONE_PUBLIC char * linphone_call_log_to_str(LinphoneCallLog *cl);

/**
 * Tells whether that call was a call to a conference server
 * @param[in] cl #LinphoneCallLog object
 * @return TRUE if the call was a call to a conference server
 */
LINPHONE_PUBLIC bool_t linphone_call_log_was_conference(LinphoneCallLog *cl);

/**
 * When the call was failed, return an object describing the failure.
 * @param[in] cl #LinphoneCallLog object
 * @return information about the error encountered by the call associated with this call log.
**/
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_call_log_get_error_info(LinphoneCallLog *cl);


/*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

/**
 * Get the user data associated with the call log.
 * @param[in] cl LinphoneCallLog object
 * @return The user data associated with the call log.
**/
LINPHONE_PUBLIC void *linphone_call_log_get_user_data(const LinphoneCallLog *cl);

/**
 * Assign a user data to the call log.
 * @param[in] cl LinphoneCallLog object
 * @param[in] ud The user data to associate with the call log.
**/
LINPHONE_PUBLIC void linphone_call_log_set_user_data(LinphoneCallLog *cl, void *ud);

/**
 * Acquire a reference to the call log.
 * @param[in] cl LinphoneCallLog object
 * @return The same LinphoneCallLog object
**/
LINPHONE_PUBLIC LinphoneCallLog * linphone_call_log_ref(LinphoneCallLog *cl);

/**
 * Release a reference to the call log.
 * @param[in] cl LinphoneCallLog object
**/
LINPHONE_PUBLIC void linphone_call_log_unref(LinphoneCallLog *cl);

/**
 * Creates a fake LinphoneCallLog.
 * @param[in] lc LinphoneCore object
 * @param[in] from LinphoneAddress of caller
 * @param[in] to LinphoneAddress of callee
 * @param[in] dir LinphoneCallDir of call
 * @param[in] duration call length in seconds
 * @param[in] start_time timestamp of call start time
 * @param[in] connected_time timestamp of call connection
 * @param[in] status LinphoneCallStatus of call
 * @param[in] video_enabled whether video was enabled or not for this call
 * @param[in] quality call quality
 * @return LinphoneCallLog object
**/
LINPHONE_PUBLIC LinphoneCallLog *linphone_core_create_call_log(LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, LinphoneCallDir dir, 
		int duration, time_t start_time, time_t connected_time, LinphoneCallStatus status, bool_t video_enabled, float quality);


/*******************************************************************************
 * DEPRECATED                                                                  *
 ******************************************************************************/

/** @deprecated Use linphone_call_log_get_from_address() instead. */
#define linphone_call_log_get_from(cl) linphone_call_log_get_from_address(cl)

/** @deprecated Use linphone_call_log_get_to_address() instead. */
#define linphone_call_log_get_to(cl) linphone_call_log_get_to_address(cl)

/** @deprecated Use linphone_call_log_set_user_data() instead. */
#define linphone_call_log_set_user_pointer(cl, ud) linphone_call_log_set_user_data(cl, ud)

/** @deprecated Use linphone_call_log_get_user_data() instead. */
#define linphone_call_log_get_user_pointer(cl) linphone_call_log_get_user_data(cl)

/**
 * Destroy a LinphoneCallLog.
 * @param cl LinphoneCallLog object
 * @deprecated Use linphone_call_log_unref() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_call_log_destroy(LinphoneCallLog *cl);

/**
 * @}
**/

#ifdef __cplusplus
}
#endif

#endif /* __LINPHONE_CALL_LOG_H__ */
