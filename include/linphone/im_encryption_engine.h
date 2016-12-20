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

#include <mediastreamer2/mscommon.h>

#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

/**
 * @addtogroup misc
 * @{
 */

/**
 * IM encryption engine.
 */
typedef struct _LinphoneImEncryptionEngine LinphoneImEncryptionEngine;

/**
 * Callback to decrypt incoming LinphoneChatMessage
 * @param engine ImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @param msg LinphoneChatMessage object
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsIncomingMessageCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to encrypt outgoing LinphoneChatMessage
 * @param engine LinphoneImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @param msg LinphoneChatMessage object
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsOutgoingMessageCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to know whether or not the engine will encrypt files before uploading them
 * @param engine LinphoneImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @return TRUE if files will be encrypted, FALSE otherwise
*/
typedef bool_t (*LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room);

/**
 * Callback to generate the key used to encrypt the files before uploading them
 * Key can be stored in the LinphoneContent object inside the LinphoneChatMessage using linphone_content_set_key
 * @param engine LinphoneImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @param msg LinphoneChatMessage object
*/
typedef void (*LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to decrypt downloading file
 * @param engine LinphoneImEncryptionEngine object
 * @param msg LinphoneChatMessage object
 * @param buffer Encrypted data buffer
 * @param size Size of the encrypted data buffer
 * @param decrypted_buffer Buffer in which to write the decrypted data
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsDownloadingFileCb)(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, const char *buffer, size_t size, char *decrypted_buffer);

/**
 * Callback to encrypt uploading file
 * @param engine LinphoneImEncryptionEngine object
 * @param msg LinphoneChatMessage object
 * @param offset The current offset of the upload
 * @param buffer Encrypted data buffer
 * @param size Size of the plain data buffer and the size of the encrypted data buffer once encryption is done
 * @param encrypted_buffer Buffer in which to write the encrypted data
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsUploadingFileCb)(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const char *buffer, size_t *size, char *encrypted_buffer);

/**
 * An object to handle the callbacks for the handling a LinphoneImEncryptionEngine object.
 */
typedef struct _LinphoneImEncryptionEngineCbs LinphoneImEncryptionEngineCbs;

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

/**
 * @}
 */

#endif /* LINPHONE_IM_ENCRYPTION_ENGINE_H */
