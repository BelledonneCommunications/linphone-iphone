/*
error_info.h
Copyright (C) 2016  Belledonne Communications SARL

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

#ifndef LINPHONE_ERROR_INFO_H
#define LINPHONE_ERROR_INFO_H

#include <mediastreamer2/mscommon.h>
#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

/**
 * @addtogroup misc
 * @{
 */

/**
 * Enum describing various failure reasons or contextual information for some events.
 * @see linphone_call_get_reason()
 * @see linphone_proxy_config_get_error()
 * @see linphone_error_info_get_reason()
**/
enum _LinphoneReason{
	LinphoneReasonNone, /**< No reason has been set by the core */
	LinphoneReasonNoResponse, /**< No response received from remote */
	LinphoneReasonForbidden, /**< Authentication failed due to bad credentials or resource forbidden */
	LinphoneReasonDeclined, /**< The call has been declined */
	LinphoneReasonNotFound, /**< Destination of the call was not found */
	LinphoneReasonNotAnswered, /**< The call was not answered in time (request timeout) */
	LinphoneReasonBusy, /**< Phone line was busy */
	LinphoneReasonUnsupportedContent, /**< Unsupported content */
	LinphoneReasonIOError, /**< Transport error: connection failures, disconnections etc... */
	LinphoneReasonDoNotDisturb, /**< Do not disturb reason */
	LinphoneReasonUnauthorized, /**< Operation is unauthorized because missing credential */
	LinphoneReasonNotAcceptable, /**< Operation is rejected due to incompatible or unsupported media parameters */
	LinphoneReasonNoMatch, /**< Operation could not be executed by server or remote client because it didn't have any context for it */
	LinphoneReasonMovedPermanently, /**< Resource moved permanently */
	LinphoneReasonGone, /**< Resource no longer exists */
	LinphoneReasonTemporarilyUnavailable, /**< Temporarily unavailable */
	LinphoneReasonAddressIncomplete, /**< Address incomplete */
	LinphoneReasonNotImplemented, /**< Not implemented */
	LinphoneReasonBadGateway, /**< Bad gateway */
	LinphoneReasonServerTimeout, /**< Server timeout */
	LinphoneReasonUnknown /**< Unknown reason */
};

#define LinphoneReasonBadCredentials LinphoneReasonForbidden

/*for compatibility*/
#define LinphoneReasonMedia LinphoneReasonUnsupportedContent

/**
 * Enum describing failure reasons.
**/
typedef enum _LinphoneReason LinphoneReason;

/**
 * Object representing full details about a signaling error or status.
 * All LinphoneErrorInfo object returned by the liblinphone API are readonly and transcients. For safety they must be used immediately
 * after obtaining them. Any other function call to the liblinphone may change their content or invalidate the pointer.
**/
typedef struct _LinphoneErrorInfo LinphoneErrorInfo;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Converts a LinphoneReason enum to a string.
 * @param[in] err A #LinphoneReason
 * @return The string representation of the specified LinphoneReason
**/
LINPHONE_PUBLIC const char *linphone_reason_to_string(LinphoneReason err);

/**
 * Converts an error code to a LinphoneReason.
 * @param[in] err An error code
 * @return The LinphoneReason corresponding to the specified error code
**/
LINPHONE_PUBLIC LinphoneReason linphone_error_code_to_reason(int err);

/**
 * Converts a LinphoneReason to an error code.
 * @param[in] reason A LinphoneReason
 * @return The error code corresponding to the specified LinphoneReason
 */
LINPHONE_PUBLIC int linphone_reason_to_error_code(LinphoneReason reason);

/**
 * Get reason code from the error info.
 * @param[in] ei ErrorInfo object
 * @return A #LinphoneReason
**/
LINPHONE_PUBLIC LinphoneReason linphone_error_info_get_reason(const LinphoneErrorInfo *ei);

/**
 * Get textual phrase from the error info.
 * This is the text that is provided by the peer in the protocol (SIP).
 * @param[in] ei ErrorInfo object
 * @return The error phrase
**/
LINPHONE_PUBLIC const char * linphone_error_info_get_phrase(const LinphoneErrorInfo *ei);

/**
 * Provides additional information regarding the failure.
 * With SIP protocol, the "Reason" and "Warning" headers are returned.
 * @param[in] ei ErrorInfo object
 * @return More details about the failure
**/
LINPHONE_PUBLIC const char * linphone_error_info_get_details(const LinphoneErrorInfo *ei);

/**
 * Get the status code from the low level protocol (ex a SIP status code).
 * @param[in] ei ErrorInfo object
 * @return The status code
**/
LINPHONE_PUBLIC int linphone_error_info_get_protocol_code(const LinphoneErrorInfo *ei);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ERROR_INFO_H */
