/*
ImEncryptionEgine.h
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

#ifndef IM_ENCRYPTION_ENGINE_H
#define IM_ENCRYPTION_ENGINE_H

#include <mediastreamer2/mscommon.h>

#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

/**
 * Callback to decrypt incoming LinphoneChatMessage
 * @param lc the LinphoneCore object
 * @param room the LinphoneChatRoom object
 * @param msg the LinphoneChatMessage object
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineIncomingMessageCb)(LinphoneCore* lc, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to encrypt outging LinphoneChatMessage
 * @param lc the LinphoneCore object
 * @param room the LinphoneChatRoom object
 * @param msg the LinphoneChatMessage object
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineOutgoingMessageCb)(LinphoneCore* lc, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to know whether or not the engine will encrypt files before uploading them
 * @param lc the LinphoneCore object
 * @param room the LinphoneChatRoom object
 * @return TRUE if files will be encrypted, FALSE otherwise
*/
typedef bool_t (*LinphoneImEncryptionEngineIsEncryptionEnabledForFileTransferCb)(LinphoneCore *lc, LinphoneChatRoom *room);

/**
 * Callback to generate the key used to encrypt the files before uploading them
 * Key can be stored in the LinphoneContent object inside the LinphoneChatMessage using linphone_content_set_key
 * @param lc the LinphoneCore object
 * @param room the LinphoneChatRoom object
 * @param msg the LinphoneChatMessage object
*/
typedef void (*LinphoneImEncryptionEngineGenerateFileTransferKeyCb)(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to decrypt downloading file
 * @param lc the LinphoneCore object
 * @param msg the LinphoneChatMessage object
 * @param buffer the encrypted data buffer
 * @param size the size of the encrypted data buffer
 * @param decrypted_buffer the buffer in which to write the decrypted data
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineDownloadingFileCb)(LinphoneCore *lc, LinphoneChatMessage *msg, const char *buffer, size_t size, char *decrypted_buffer);

/**
 * Callback to encrypt uploading file
 * @param lc the LinphoneCore object
 * @param msg the LinphoneChatMessage object
 * @param buffer the encrypted data buffer
 * @param size the size of the plain data buffer and the size of the encrypted data buffer once encryption is done
 * @param encrypted_buffer the buffer in which to write the encrypted data
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineUploadingFileCb)(LinphoneCore *lc, LinphoneChatMessage *msg, size_t offset, const char *buffer, size_t *size, char *encrypted_buffer);

typedef struct _LinphoneImEncryptionEngineCbs LinphoneImEncryptionEngineCbs;

typedef struct _LinphoneImEncryptionEngine LinphoneImEncryptionEngine;

LinphoneImEncryptionEngineCbs *linphone_im_encryption_engine_cbs_new(void);

void linphone_im_encryption_engine_cbs_destory(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Gets the user data in the LinphoneImEncryptionEngineCbs object
 * @param cbs the LinphoneImEncryptionEngineCbs
 * @return the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void *linphone_im_encryption_engine_cbs_get_user_data(const LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the user data in the LinphoneImEncryptionEngineCbs object
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param data the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_user_data(LinphoneImEncryptionEngineCbs *cbs, void *data);

/**
 * Creates a LinphoneImEncryptionEngine object
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngine *linphone_im_encryption_engine_new(void);

/**
 * Destroys the LinphoneImEncryptionEngine
 * @param imee the LinphoneImEncryptionEngine object
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_destory(LinphoneImEncryptionEngine *imee);

/**
 * Gets the user data in the LinphoneImEncryptionEngine object
 * @param imee the LinphoneImEncryptionEngine
 * @return the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void *linphone_im_encryption_engine_get_user_data(const LinphoneImEncryptionEngine *imee);

/**
 * Sets the user data in the LinphoneImEncryptionEngine object
 * @param imee the LinphoneImEncryptionEngine object
 * @param data the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_set_user_data(LinphoneImEncryptionEngine *imee, void *data);

/**
 * Gets the LinphoneImEncryptionEngineCbs object that holds the callbacks
 * @param imee the LinphoneImEncryptionEngine object
 * @return the LinphoneImEncryptionEngineCbs object
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineCbs* linphone_im_encryption_engine_get_callbacks(const LinphoneImEncryptionEngine *imee);

/**
 * Gets the callback that will decrypt the chat messages upon reception
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineIncomingMessageCb linphone_im_encryption_engine_cbs_get_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will decrypt the chat messages upon reception
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param cb the callback to call
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineIncomingMessageCb cb);

/**
 * Gets the callback that will encrypt the chat messages before sending them
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineOutgoingMessageCb linphone_im_encryption_engine_cbs_get_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will encrypt the chat messages before sending them
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param cb the callback to call
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineOutgoingMessageCb cb);

/**
 * Gets the callback that will decrypt the files while downloading them
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineDownloadingFileCb linphone_im_encryption_engine_cbs_get_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will decrypt the files while downloading them
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param cb the callback to call
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineDownloadingFileCb cb);

/**
 * Gets the callback that will will encrypt the files while uploading them
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineUploadingFileCb linphone_im_encryption_engine_cbs_get_process_uploading_file(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will encrypt the files while uploading them
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param cb the callback to call
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_uploading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineUploadingFileCb cb);

/**
 * Gets the callback telling wheter or not to encrypt the files
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineIsEncryptionEnabledForFileTransferCb linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback telling wheter or not to encrypt the files
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param cb the callback to call
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_is_encryption_enabled_for_file_transfer(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineIsEncryptionEnabledForFileTransferCb cb);

/**
 * Gets the callback that will generate the key to encrypt the file before uploading it
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @return the callback
 * @ingroup misc
*/
LINPHONE_PUBLIC LinphoneImEncryptionEngineGenerateFileTransferKeyCb linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(LinphoneImEncryptionEngineCbs *cbs);

/**
 * Sets the callback that will generate the key to encrypt the file before uploading it
 * @param cbs the LinphoneImEncryptionEngineCbs object
 * @param cb the callback to call
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_generate_file_transfer_key(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineGenerateFileTransferKeyCb cb);

#endif /* IM_ENCRYPTION_ENGINE_H */