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


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup media_parameters
 * @{
**/

/**
 * Get the type of payload.
 * @param[in] pt LinphonePayloadType object
 * @return The type of payload.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_type(const LinphonePayloadType *pt);

/**
 * Get the normal bitrate in bits/s.
 * @param[in] pt LinphonePayloadType object
 * @return The normal bitrate in bits/s.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_normal_bitrate(const LinphonePayloadType *pt);

/**
 * Get the mime type.
 * @param[in] pt LinphonePayloadType object
 * @return The mime type.
 */
LINPHONE_PUBLIC const char * linphone_payload_type_get_mime_type(const LinphonePayloadType *pt);

/**
 * Get the number of channels.
 * @param[in] pt LinphonePayloadType object
 * @return The number of channels.
 */
LINPHONE_PUBLIC int linphone_payload_type_get_channels(const LinphonePayloadType *pt);

/**
 * Returns the payload type number assigned for this codec.
 * @param[in] pt LinphonePayloadType object
 * @return The number of the payload type
**/
LINPHONE_PUBLIC int linphone_payload_type_get_number(const PayloadType *pt);

/**
 * Force a number for a payload type. The LinphoneCore does payload type number assignment automatically. THis function is to be used mainly for tests, in order
 * to override the automatic assignment mechanism.
 * @param[in] pt LinphonePayloadType object
 * @param[in] number The number to assign to the payload type
**/
LINPHONE_PUBLIC void linphone_payload_type_set_number(PayloadType *pt, int number);

/**
 * Tells whether the specified payload type represents a variable bitrate codec.
 * @param[in] pt LinphonePayloadType object
 * @return TRUE if the payload type represents a VBR codec, FALSE if disabled.
 */
LINPHONE_PUBLIC bool_t linphone_payload_type_is_vbr(const LinphonePayloadType *pt);

/**
 * @}
**/


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_PAYLOAD_TYPE_H_ */
