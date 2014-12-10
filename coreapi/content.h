/*
content.h
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef LINPHONE_CONTENT_H_
#define LINPHONE_CONTENT_H_


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup misc
 * @{
 */

/**
 * The LinphoneContent object holds data that can be embedded in a signaling message.
**/
struct _LinphoneContent;
/**
 * The LinphoneContent object holds data that can be embedded in a signaling message.
**/
typedef struct _LinphoneContent LinphoneContent;

/**
 * @deprecated Use LinphoneContent objects instead of this structure.
 */
struct _LinphoneContentPrivate{
	char *type; /**<mime type for the data, for example "application"*/
	char *subtype; /**<mime subtype for the data, for example "html"*/
	void *data; /**<the actual data buffer, usually a string. Null when provided by callbacks #LinphoneCoreFileTransferSendCb or #LinphoneCoreFileTransferRecvCb*/
	size_t size; /**<the size of the data buffer, excluding null character despite null character is always set for convenience.
				When provided by callback #LinphoneCoreFileTransferSendCb or #LinphoneCoreFileTransferRecvCb, it states the total number of bytes of the transfered file*/
	char *encoding; /**<The encoding of the data buffer, for example "gzip"*/
	char *name; /**< used by RCS File transfer messages to store the original filename of the file to be downloaded from server */
};

/**
 * Alias to the LinphoneContentPrivate struct.
 * @deprecated
**/
typedef struct _LinphoneContentPrivate LinphoneContentPrivate;

/**
 * Convert a LinphoneContentPrivate structure to a LinphoneContent object.
 * @deprecated Utility macro to ease porting existing code from LinphoneContentPrivate structure (old LinphoneContent structure) to new LinphoneContent object.
 */
#define LINPHONE_CONTENT(lcp) linphone_content_private_to_linphone_content(lcp)

/**
 * Convert a LinphoneContentPrivate structure to a LinphoneContent object.
 * @deprecated Utility function to ease porting existing code from LinphoneContentPrivate structure (old LinphoneContent structure) to new LinphoneContent object.
 */
LINPHONE_PUBLIC LinphoneContent * linphone_content_private_to_linphone_content(const LinphoneContentPrivate *lcp);

/**
 * Convert a LinphoneContent object to a LinphoneContentPrivate structure.
 * @deprecated Utility macro to ease porting existing code from LinphoneContentPrivate structure (old LinphoneContent structure) to new LinphoneContent object.
 */
#define LINPHONE_CONTENT_PRIVATE(lc) linphone_content_to_linphone_content_private(lc)

/**
 * Convert a LinphoneContent object to a LinphoneContentPrivate structure.
 * @deprecated Utility function to ease porting existing code from LinphoneContentPrivate structure (old LinphoneContent structure) to new LinphoneContent object.
 */
LINPHONE_PUBLIC LinphoneContentPrivate * linphone_content_to_linphone_content_private(const LinphoneContent *content);

/**
 * Create a content with default values from Linphone core.
 * @param[in] lc LinphoneCore object
 * @return LinphoneContent object with default values set
 */
LINPHONE_PUBLIC LinphoneContent * linphone_core_create_content(LinphoneCore *lc);

/**
 * Acquire a reference to the content.
 * @param[in] content LinphoneContent object.
 * @return The same LinphoneContent object.
**/
LINPHONE_PUBLIC LinphoneContent * linphone_content_ref(LinphoneContent *content);

/**
 * Release reference to the content.
 * @param[in] content LinphoneContent object.
**/
LINPHONE_PUBLIC void linphone_content_unref(LinphoneContent *content);

/**
 * Retrieve the user pointer associated with the content.
 * @param[in] content LinphoneContent object.
 * @return The user pointer associated with the content.
**/
LINPHONE_PUBLIC void *linphone_content_get_user_data(const LinphoneContent *content);

/**
 * Assign a user pointer to the content.
 * @param[in] content LinphoneContent object.
 * @param[in] ud The user pointer to associate with the content.
**/
LINPHONE_PUBLIC void linphone_content_set_user_data(LinphoneContent *content, void *ud);

/**
 * Get the mime type of the content data.
 * @param[in] content LinphoneContent object.
 * @return The mime type of the content data, for example "application".
 */
LINPHONE_PUBLIC const char * linphone_content_get_type(const LinphoneContent *content);

/**
 * Set the mime type of the content data.
 * @param[in] content LinphoneContent object.
 * @param[in] type The mime type of the content data, for example "application".
 */
LINPHONE_PUBLIC void linphone_content_set_type(LinphoneContent *content, const char *type);

/**
 * Get the mime subtype of the content data.
 * @param[in] content LinphoneContent object.
 * @return The mime subtype of the content data, for example "html".
 */
LINPHONE_PUBLIC const char * linphone_content_get_subtype(const LinphoneContent *content);

/**
 * Set the mime subtype of the content data.
 * @param[in] content LinphoneContent object.
 * @param[in] subtype The mime subtype of the content data, for example "html".
 */
LINPHONE_PUBLIC void linphone_content_set_subtype(LinphoneContent *content, const char *subtype);

/**
 * Get the content data buffer, usually a string.
 * @param[in] content LinphoneContent object.
 * @return The content data buffer.
 */
LINPHONE_PUBLIC void * linphone_content_get_buffer(const LinphoneContent *content);

/**
 * Set the content data buffer, usually a string.
 * @param[in] content LinphoneContent object.
 * @param[in] buffer The content data buffer.
 * @param[in] size The size of the content data buffer.
 */
LINPHONE_PUBLIC void linphone_content_set_buffer(LinphoneContent *content, const void *buffer, size_t size);

/**
 * Get the string content data buffer.
 * @param[in] content LinphoneContent object
 * @return The string content data buffer.
 */
LINPHONE_PUBLIC char * linphone_content_get_string_buffer(const LinphoneContent *content);

/**
 * Set the string content data buffer.
 * @param[in] content LinphoneContent object.
 * @param[in] buffer The string content data buffer.
 */
LINPHONE_PUBLIC void linphone_content_set_string_buffer(LinphoneContent *content, const char *buffer);

/**
 * Get the content data buffer size, excluding null character despite null character is always set for convenience.
 * @param[in] content LinphoneContent object.
 * @return The content data buffer size.
 */
LINPHONE_PUBLIC size_t linphone_content_get_size(const LinphoneContent *content);

/**
 * Set the content data size, excluding null character despite null character is always set for convenience.
 * @param[in] content LinphoneContent object
 * @param[in] size The content data buffer size.
 */
LINPHONE_PUBLIC void linphone_content_set_size(LinphoneContent *content, size_t size);

/**
 * Get the encoding of the data buffer, for example "gzip".
 * @param[in] content LinphoneContent object.
 * @return The encoding of the data buffer.
 */
LINPHONE_PUBLIC const char * linphone_content_get_encoding(const LinphoneContent *content);

/**
 * Set the encoding of the data buffer, for example "gzip".
 * @param[in] content LinphoneContent object.
 * @param[in] encoding The encoding of the data buffer.
 */
LINPHONE_PUBLIC void linphone_content_set_encoding(LinphoneContent *content, const char *encoding);

/**
 * Get the name associated with a RCS file transfer message. It is used to store the original filename of the file to be downloaded from server.
 * @param[in] content LinphoneContent object.
 * @return The name of the content.
 */
LINPHONE_PUBLIC const char * linphone_content_get_name(const LinphoneContent *content);

/**
 * Set the name associated with a RCS file transfer message. It is used to store the original filename of the file to be downloaded from server.
 * @param[in] content LinphoneContent object.
 * @param[in] name The name of the content.
 */
LINPHONE_PUBLIC void linphone_content_set_name(LinphoneContent *content, const char *name);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_CONTENT_H_ */
