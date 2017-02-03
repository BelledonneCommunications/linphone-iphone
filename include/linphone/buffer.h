/*
buffer.h
Copyright (C) 2010-2014 Belledonne Communications SARL

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

#ifndef LINPHONE_BUFFER_H_
#define LINPHONE_BUFFER_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup misc
 * @{
 */

/**
 * Create a new empty LinphoneBuffer object.
 * @return A new LinphoneBuffer object.
 */
LINPHONE_PUBLIC LinphoneBuffer * linphone_buffer_new(void);

/**
 * Create a new LinphoneBuffer object from existing data.
 * @param[in] data The initial data to store in the LinphoneBuffer.
 * @param[in] size The size of the initial data to stroe in the LinphoneBuffer.
 * @return A new LinphoneBuffer object.
 */
LINPHONE_PUBLIC LinphoneBuffer * linphone_buffer_new_from_data(const uint8_t *data, size_t size);

/**
 * Create a new LinphoneBuffer object from a string.
 * @param[in] data The initial string content of the LinphoneBuffer.
 * @return A new LinphoneBuffer object.
 */
LINPHONE_PUBLIC LinphoneBuffer * linphone_buffer_new_from_string(const char *data);

/**
 * Acquire a reference to the buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @return The same LinphoneBuffer object.
**/
LINPHONE_PUBLIC LinphoneBuffer * linphone_buffer_ref(LinphoneBuffer *buffer);

/**
 * Release reference to the buffer.
 * @param[in] buffer LinphoneBuffer object.
**/
LINPHONE_PUBLIC void linphone_buffer_unref(LinphoneBuffer *buffer);

/**
 * Retrieve the user pointer associated with the buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @return The user pointer associated with the buffer.
**/
LINPHONE_PUBLIC void *linphone_buffer_get_user_data(const LinphoneBuffer *buffer);

/**
 * Assign a user pointer to the buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @param[in] ud The user pointer to associate with the buffer.
**/
LINPHONE_PUBLIC void linphone_buffer_set_user_data(LinphoneBuffer *buffer, void *ud);

/**
 * Get the content of the data buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @return The content of the data buffer.
 */
LINPHONE_PUBLIC const uint8_t * linphone_buffer_get_content(const LinphoneBuffer *buffer);

/**
 * Set the content of the data buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @param[in] content The content of the data buffer.
 * @param[in] size The size of the content of the data buffer.
 */
LINPHONE_PUBLIC void linphone_buffer_set_content(LinphoneBuffer *buffer, const uint8_t *content, size_t size);

/**
 * Get the string content of the data buffer.
 * @param[in] buffer LinphoneBuffer object
 * @return The string content of the data buffer.
 */
LINPHONE_PUBLIC const char * linphone_buffer_get_string_content(const LinphoneBuffer *buffer);

/**
 * Set the string content of the data buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @param[in] content The string content of the data buffer.
 */
LINPHONE_PUBLIC void linphone_buffer_set_string_content(LinphoneBuffer *buffer, const char *content);

/**
 * Get the size of the content of the data buffer.
 * @param[in] buffer LinphoneBuffer object.
 * @return The size of the content of the data buffer.
 */
LINPHONE_PUBLIC size_t linphone_buffer_get_size(const LinphoneBuffer *buffer);

/**
 * Set the size of the content of the data buffer.
 * @param[in] buffer LinphoneBuffer object
 * @param[in] size The size of the content of the data buffer.
 */
LINPHONE_PUBLIC void linphone_buffer_set_size(LinphoneBuffer *buffer, size_t size);

/**
 * Tell whether the LinphoneBuffer is empty.
 * @param[in] buffer LinphoneBuffer object
 * @return A boolean value telling whether the LinphoneBuffer is empty or not.
 */
LINPHONE_PUBLIC bool_t linphone_buffer_is_empty(const LinphoneBuffer *buffer);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_BUFFER_H_ */
