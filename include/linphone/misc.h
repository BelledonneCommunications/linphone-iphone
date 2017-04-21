/*
misc.h
Copyright (C) 2010-2017 Belledonne Communications SARL

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

#ifndef LINPHONE_MISC_H_
#define LINPHONE_MISC_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Lowest volume measurement that can be returned by linphone_call_get_play_volume() or linphone_call_get_record_volume(), corresponding to pure silence.
 * @ingroup call_misc
**/
#define LINPHONE_VOLUME_DB_LOWEST (-120) /* WARNING: keep this in sync with mediastreamer2/msvolume.h */


/**
 * Disable a sip transport
 * Use with #LinphoneSipTransports
 * @ingroup initializing
 */
#define LC_SIP_TRANSPORT_DISABLED 0
/**
 * Randomly chose a sip port for this transport
 * Use with #LinphoneSipTransports
 * @ingroup initializing
 */
#define LC_SIP_TRANSPORT_RANDOM (-1)

/**
 * Don't create any server socket for this transport, ie don't bind on any port.
 * Use with #LinphoneSipTransports
 * @ingroup initializing
**/
#define LC_SIP_TRANSPORT_DONTBIND (-2)


/**
 * Function returning a human readable value for LinphoneStreamType.
 * @ingroup initializing
 **/
LINPHONE_PUBLIC const char *linphone_stream_type_to_string(const LinphoneStreamType);

/**
 * Human readable version of the #LinphoneRegistrationState
 * @param cs sate
 * @ingroup proxies
 */
LINPHONE_PUBLIC const char *linphone_registration_state_to_string(LinphoneRegistrationState cs);

/**
 * Convert enum member to string.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char *linphone_media_encryption_to_string(LinphoneMediaEncryption menc);

LINPHONE_PUBLIC const char* linphone_privacy_to_string(LinphonePrivacy privacy);

LINPHONE_PUBLIC const char *linphone_subscription_state_to_string(LinphoneSubscriptionState state);

LINPHONE_PUBLIC const char *linphone_publish_state_to_string(LinphonePublishState state);

LINPHONE_PUBLIC const char *linphone_ice_state_to_string(LinphoneIceState state);

LINPHONE_PUBLIC const char *linphone_global_state_to_string(LinphoneGlobalState gs);

LINPHONE_PUBLIC const char *linphone_core_log_collection_upload_state_to_string(const LinphoneCoreLogCollectionUploadState lcus);

LINPHONE_PUBLIC const char *linphone_call_state_to_string(LinphoneCallState cs);

/**
 * Converts a _LinphoneConfiguringState enum to a string.
 * @ingroup misc
**/
LINPHONE_PUBLIC const char *linphone_configuring_state_to_string(LinphoneConfiguringState cs);

/**
 * Returns a #LinphoneChatMessageState as a string.
 */
LINPHONE_PUBLIC	const char* linphone_chat_message_state_to_string(const LinphoneChatMessageState state);

/**
 * Converts a LinphoneReason enum to a string.
 * @param[in] err A #LinphoneReason
 * @return The string representation of the specified LinphoneReason
 * @ingroup misc
**/
LINPHONE_PUBLIC const char *linphone_reason_to_string(LinphoneReason err);

/**
 * Return humain readable presence status
 * @param ss
 * @deprecated Use #LinphonePresenceModel, #LinphonePresenceActivity and linphone_presence_activity_to_string() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED const char *linphone_online_status_to_string(LinphoneOnlineStatus ss);

/**
 * Convert a string into LinphoneTunnelMode enum
 * @param string String to convert
 * @return An LinphoneTunnelMode enum. If the passed string is NULL or
 * does not match with any mode, the LinphoneTunnelModeDisable is returned.
 */
LINPHONE_PUBLIC LinphoneTunnelMode linphone_tunnel_mode_from_string(const char *string);

/**
 * Convert a tunnel mode enum into string
 * @param mode Enum to convert
 * @return "disable", "enable" or "auto"
 */
LINPHONE_PUBLIC const char *linphone_tunnel_mode_to_string(LinphoneTunnelMode mode);

/**
 * Check whether Matroksa format is supported by the player
 * @return TRUE if it is supported
 */
LINPHONE_PUBLIC bool_t linphone_local_player_matroska_supported(void);

/**
 * Converts a LinphoneTransportType enum to a lowercase string.
 * @ingroup misc
**/
LINPHONE_PUBLIC const char* linphone_transport_to_string(LinphoneTransportType transport);

/**
 * Converts a lowercase string to a LinphoneTransportType enum.
 * @ingroup misc
 * @return Transport matching input, or LinphoneTransportUdp if nothing is found
**/
LINPHONE_PUBLIC LinphoneTransportType linphone_transport_parse(const char* transport);

/**
 * Converts an error code to a LinphoneReason.
 * @param[in] err An error code
 * @return The LinphoneReason corresponding to the specified error code
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneReason linphone_error_code_to_reason(int err);

/**
 * Converts a LinphoneReason to an error code.
 * @param[in] reason A LinphoneReason
 * @return The error code corresponding to the specified LinphoneReason
 * @ingroup misc
 */
LINPHONE_PUBLIC int linphone_reason_to_error_code(LinphoneReason reason);

/**
 * Increment refcount.
 * @param[in] range LinphoneRange object
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneRange *linphone_range_ref(LinphoneRange *range);

/**
 * Decrement refcount and possibly free the object.
 * @param[in] range LinphoneRange object
 * @ingroup misc
**/
LINPHONE_PUBLIC void linphone_range_unref(LinphoneRange *range);

/**
 * Gets the user data in the LinphoneRange object
 * @param[in] range the LinphoneRange
 * @return the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void *linphone_range_get_user_data(const LinphoneRange *range);

/**
 * Sets the user data in the LinphoneRange object
 * @param[in] range the LinphoneRange object
 * @param[in] data the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_range_set_user_data(LinphoneRange *range, void *data);

/**
 * Gets the lower value of the range
 * @param[in] range a LinphoneRange
 * @return The lower value
 * @ingroup misc
 */
LINPHONE_PUBLIC int linphone_range_get_min(const LinphoneRange *range);

/**
 * Gets the higher value of the range
 * @param[in] range a LinphoneRange
 * @return The higher value
 * @ingroup misc
 */
LINPHONE_PUBLIC int linphone_range_get_max(const LinphoneRange *range);

/**
 * Sets the lower value of the range
 * @param[in] range a LinphoneRange
 * @param[in] min the value to set
 * @ingroup misc
 */
LINPHONE_PUBLIC void linphone_range_set_min(LinphoneRange *range, int min);

/**
 * Sets the higher value of the range
 * @param[in] range a LinphoneRange
 * @param[in] max the value to set
 * @ingroup misc
 */
LINPHONE_PUBLIC void linphone_range_set_max(LinphoneRange *range, int max);


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_MISC_H_ */
