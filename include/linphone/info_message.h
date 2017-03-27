/*
info_message.h
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

#ifndef LINPHONE_INFO_MESSAGE_H_
#define LINPHONE_INFO_MESSAGE_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup misc
 * @{
 */


/**
 * Add a header to an info message to be sent.
 * @param im the info message
 * @param name the header'name
 * @param value the header's value
**/
LINPHONE_PUBLIC void linphone_info_message_add_header(LinphoneInfoMessage *im, const char *name, const char *value);

/**
 * Obtain a header value from a received info message.
 * @param im the info message
 * @param name the header'name
 * @return the corresponding header's value, or NULL if not exists.
**/
LINPHONE_PUBLIC const char *linphone_info_message_get_header(const LinphoneInfoMessage *im, const char *name);

/**
 * Assign a content to the info message.
 * @param im the linphone info message
 * @param content the content described as a #LinphoneContent structure.
 * All fields of the LinphoneContent are copied, thus the application can destroy/modify/recycloe the content object freely ater the function returns.
**/
LINPHONE_PUBLIC void linphone_info_message_set_content(LinphoneInfoMessage *im,  const LinphoneContent *content);

/**
 * Returns the info message's content as a #LinphoneContent structure.
**/
LINPHONE_PUBLIC const LinphoneContent * linphone_info_message_get_content(const LinphoneInfoMessage *im);

/**
 * Take a reference on a #LinphoneInfoMessage.
 */
LINPHONE_PUBLIC LinphoneInfoMessage *linphone_info_message_ref(LinphoneInfoMessage *im);

/**
 * Release a reference on a #LinphoneInfoMessage.
 */
LINPHONE_PUBLIC void linphone_info_message_unref(LinphoneInfoMessage *im);

/**
 * Destroy a LinphoneInfoMessage.
 * @deprecated Use linphone_info_message_unref() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_info_message_destroy(LinphoneInfoMessage *im);

LINPHONE_PUBLIC LinphoneInfoMessage *linphone_info_message_copy(const LinphoneInfoMessage *orig);


/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_INFO_MESSAGE_H_ */
