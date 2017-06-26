/*
im_encryption_engine.h
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

#ifndef LINPHONE_IM_ENCRYPTION_ENGINE_H
#define LINPHONE_IM_ENCRYPTION_ENGINE_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup misc
 * @{
 */

/**
 * Acquire a reference to the LinphoneImEncryptionEngineCbs.
 * @param[in] cbs LinphoneImEncryptionEngineCbs object.
 * @return The same LinphoneImEncryptionEngineCbs object.
**/
LinphoneImEncryptionEngineCbs * linphone_im_encryption_engine_cbs_ref(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Release reference to the LinphoneImEncryptionEngineCbs.
 * @param[in] cbs LinphoneImEncryptionEngineCbs object.
**/
void linphone_im_encryption_engine_cbs_unref(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Gets the user data in the LinphoneImEncryptionEngineCbs object
 * @param[in] cbs the LinphoneImEncryptionEngineCbs
 * @return the user data
*/
LINPHONE_PUBLIC void *linphone_im_encryption_engine_cbs_get_user_data(const LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the user data in the LinphoneImEncryptionEngineCbs object
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] data the user data
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_user_data(LinphoneImEncryptionEngineCbs *cbs, void *data);

/**
 * Acquire a reference to the LinphoneImEncryptionEngine.
 * @param[in] imee LinphoneImEncryptionEngine object.
 * @return The same LinphoneImEncryptionEngine object.
**/
LINPHONE_PUBLIC LinphoneImEncryptionEngine * linphone_im_encryption_engine_ref(LinphoneImEncryptionEngine *imee);

/**
 * Release reference to the LinphoneImEncryptionEngine.
 * @param[in] imee LinphoneImEncryptionEngine object.
**/
LINPHONE_PUBLIC void linphone_im_encryption_engine_unref(LinphoneImEncryptionEngine *imee);

/**
 * Gets the user data in the LinphoneImEncryptionEngine object
 * @param[in] imee the LinphoneImEncryptionEngine
 * @return the user data
*/
LINPHONE_PUBLIC void *linphone_im_encryption_engine_get_user_data(const LinphoneImEncryptionEngine *imee);

/**
 * Sets the user data in the LinphoneImEncryptionEngine object
 * @param[in] imee the LinphoneImEncryptionEngine object
 * @param[in] data the user data
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_set_user_data(LinphoneImEncryptionEngine *imee, void *data);

/**
 * Gets the LinphoneCore object that created the IM encryption engine
 * @param[in] imee LinphoneImEncryptionEngine object
 * @return The LinphoneCore object that created the IM encryption engine
 */
LINPHONE_PUBLIC LinphoneCore * linphone_im_encryption_engine_get_core(LinphoneImEncryptionEngine *imee);

/**
 * Gets the LinphoneImEncryptionEngineCbs object that holds the callbacks
 * @param[in] imee the LinphoneImEncryptionEngine object
 * @return the LinphoneImEncryptionEngineCbs object
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbs* linphone_im_encryption_engine_get_callbacks(const LinphoneImEncryptionEngine *imee);

/**
 * Gets the callback that will decrypt the chat messages upon reception
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbsIncomingMessageCb linphone_im_encryption_engine_cbs_get_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will decrypt the chat messages upon reception
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] cb the callback to call
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsIncomingMessageCb cb);

/**
 * Gets the callback that will encrypt the chat messages before sending them
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbsOutgoingMessageCb linphone_im_encryption_engine_cbs_get_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will encrypt the chat messages before sending them
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] cb the callback to call
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsOutgoingMessageCb cb);

/**
 * Gets the callback that will decrypt the files while downloading them
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbsDownloadingFileCb linphone_im_encryption_engine_cbs_get_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will decrypt the files while downloading them
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] cb the callback to call
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsDownloadingFileCb cb);

/**
 * Gets the callback that will will encrypt the files while uploading them
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbsUploadingFileCb linphone_im_encryption_engine_cbs_get_process_uploading_file(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will encrypt the files while uploading them
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] cb the callback to call
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_uploading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsUploadingFileCb cb);

/**
 * Gets the callback telling wheter or not to encrypt the files
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback telling wheter or not to encrypt the files
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] cb the callback to call
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_is_encryption_enabled_for_file_transfer(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb cb);

/**
 * Gets the callback that will generate the key to encrypt the file before uploading it
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will generate the key to encrypt the file before uploading it
 * @param[in] cbs the LinphoneImEncryptionEngineCbs object
 * @param[in] cb the callback to call
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_generate_file_transfer_key(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb cb);

/** Set a chat message text to be sent by #linphone_chat_room_send_message
 * @param[in] msg LinphoneChatMessage
 * @param[in] text Const char *
 * @returns 0 if succeed.
*/
LINPHONE_PUBLIC int linphone_chat_message_set_text(LinphoneChatMessage *msg, const char* text);

/**
 * Create the IM encryption engine
 * @return The created the IM encryption engine
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngine *linphone_im_encryption_engine_new(void);
	
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_IM_ENCRYPTION_ENGINE_H */
