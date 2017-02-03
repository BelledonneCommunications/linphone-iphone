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
 * @}
**/


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_PAYLOAD_TYPE_H_ */
