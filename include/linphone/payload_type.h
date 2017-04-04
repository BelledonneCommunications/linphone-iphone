/*
payload_type.h
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

#ifndef LINPHONE_PAYLOAD_TYPE_H_
#define LINPHONE_PAYLOAD_TYPE_H_

#include "linphone/types.h"

/**
 * @addtogroup media_parameters
 * @{
**/


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Take a reference on a #LinphonePayloadType.
 */
LINPHONE_PUBLIC LinphonePayloadType *linphone_payload_type_ref(LinphonePayloadType *pt);

/**
 * Release a reference on a #LinphonePayloadType.
 */
LINPHONE_PUBLIC void linphone_payload_type_unref(LinphonePayloadType *pt);

/**
 * Get the type of a payload type.
 * @param[in] pt The payload type.
 * @return The type of the payload e.g. PAYLOAD_AUDIO_CONTINUOUS or PAYLOAD_VIDEO.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_type(const LinphonePayloadType *pt);

/**
 * Enable/disable a payload type.
 * @param[in] pt The payload type to enable/disable.
 * @param[in] enabled Set TRUE for enabling and FALSE for disabling.
 * @return 0 for success, -1 for failure.
 */
LINPHONE_PUBLIC int linphone_payload_type_enable(LinphonePayloadType *pt, bool_t enabled);

/**
 * Check whether a palyoad type is enabled.
 * @return TRUE if enabled, FALSE if disabled.
 */
LINPHONE_PUBLIC bool_t linphone_payload_type_enabled(const LinphonePayloadType *pt);

/**
 * Return a string describing a payload type. The format of the string is
 * &lt;mime_type&gt;/&lt;clock_rate&gt;/&lt;channels&gt;.
 * @param[in] pt The payload type.
 * @return The description of the payload type. Must be release after use.
 */
LINPHONE_PUBLIC char *linphone_payload_type_get_description(const LinphonePayloadType *pt);

/**
 * Get a description of the encoder used to provide a payload type.
 * @param[in] pt The payload type.
 * @return The description of the encoder. Can be NULL if the payload type is not supported by Mediastreamer2.
 */
LINPHONE_PUBLIC const char *linphone_payload_type_get_encoder_description(const LinphonePayloadType *pt);

/**
 * Get the normal bitrate in bits/s.
 * @param[in] pt The payload type.
 * @return The normal bitrate in bits/s or -1 if an error has occured.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_normal_bitrate(const LinphonePayloadType *pt);

/**
 * Change the normal bitrate of a payload type..
 * @param[in] pt The payload type to change.
 * @param[in] bitrate The new bitrate in bits/s.
 */
LINPHONE_PUBLIC void linphone_payload_type_set_normal_bitrate(LinphonePayloadType *pt, int bitrate);

/**
 * Get the mime type.
 * @param[in] pt The payload type.
 * @return The mime type.
 */
LINPHONE_PUBLIC const char * linphone_payload_type_get_mime_type(const LinphonePayloadType *pt);

/**
 * Get the number of channels.
 * @param[in] pt The payload type.
 * @return The number of channels.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_channels(const LinphonePayloadType *pt);

/**
 * Returns the payload type number assigned for this codec.
 * @param[in] pt The payload type.
 * @return The number of the payload type.
**/
LINPHONE_PUBLIC int linphone_payload_type_get_number(const LinphonePayloadType *pt);

/**
 * Force a number for a payload type. The LinphoneCore does payload type number assignment automatically.
 * This function is mainly to be used for tests, in order to override the automatic assignment mechanism.
 * @param[in] pt The payload type.
 * @param[in] number The number to assign to the payload type.
**/
LINPHONE_PUBLIC void linphone_payload_type_set_number(LinphonePayloadType *pt, int number);

/**
 * Get the format parameters for incoming streams.
 * @param[in] pt The payload type.
 * @return The format parameters as string.
 */
LINPHONE_PUBLIC const char *linphone_payload_type_get_recv_fmtp(const LinphonePayloadType *pt);

/**
 * Set the format parameters for incoming streams.
 * @param[in] pt The payload type.
 * @param[in] recv_fmtp The new format parameters as string. The string will be copied.
 */
LINPHONE_PUBLIC void linphone_payload_type_set_recv_fmtp(LinphonePayloadType *pt, const char *recv_fmtp);

/**
 * Get the format parameters for outgoing streams.
 * @param[in] pt The payload type.
 * @return The format parameters as string.
 */
LINPHONE_PUBLIC const char *linphone_payload_type_get_send_fmtp(const LinphonePayloadType *pt);

/**
 * Set the format parameters for outgoing streams.
 * @param[in] pt The payload type.
 * @param[in] send_fmtp The new format parameters as string. The string will be copied.
 */
LINPHONE_PUBLIC void linphone_payload_type_set_send_fmtp(LinphonePayloadType *pt, const char *send_fmtp);

/**
 * Get the clock rate of a payload type.
 * @param[in] pt The payload type.
 * @return[in] The clock rate in Hz.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_clock_rate(const LinphonePayloadType *pt);

/**
 * Tells whether the specified payload type represents a variable bitrate codec.
 * @param[in] pt The payload type.
 * @return TRUE if the payload type represents a VBR codec, FALSE instead.
 */
LINPHONE_PUBLIC bool_t linphone_payload_type_is_vbr(const LinphonePayloadType *pt);

/**
 * Check whether the payload is usable according the bandwidth targets set in the core.
 * @param[in] pt The payload type to test.
 * @return TRUE if the payload type is usable.
 */
LINPHONE_PUBLIC bool_t linphone_payload_type_is_usable(const LinphonePayloadType *pt);


#ifdef __cplusplus
}
#endif

/**
 * @}
**/

#endif /* LINPHONE_PAYLOAD_TYPE_H_ */
