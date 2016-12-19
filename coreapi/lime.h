/*
linphone
Copyright (C) 2015  Belledonne Communications SARL

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

#ifndef LIME_H
#define LIME_H

#define LIME_INVALID_CACHE	0x1001
#define LIME_UNABLE_TO_DERIVE_KEY 0x1002
#define LIME_UNABLE_TO_ENCRYPT_MESSAGE 0x1004
#define LIME_UNABLE_TO_DECRYPT_MESSAGE 0x1008
#define LIME_NO_VALID_KEY_FOUND_FOR_PEER	0x1010
#define LIME_INVALID_ENCRYPTED_MESSAGE 0x1020
#define LIME_NOT_ENABLED 0x1100

/* this define the maximum key derivation number allowed to get the caches back in sync in case of missed messages */
#define MAX_DERIVATION_NUMBER 100

#define LIME_SENDER	0x01
#define LIME_RECEIVER 0x02
#include <stdint.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>

#include "linphone/core.h"
#include <mediastreamer2/mscommon.h>

#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

/**
 * @brief Structure holding all needed material to encrypt/decrypt Messages */
typedef struct limeKey_struct  {
	uint8_t key[32]; /**< a 256 bit key used to encrypt/decrypt message */
	uint8_t sessionId[32]; /**< a session id used to derive key */
	uint32_t sessionIndex; /**< an index to count number of derivation */
	uint8_t peerZID[12]; /**< the ZID associated to this key */
} limeKey_t;

/**
 * @brief Store the differents keys associated to a sipURI */
typedef struct limeURIKeys_struct {
	limeKey_t 	**peerKeys; /**< an array of all the key material associated to each ZID matching the specified URI */
	uint16_t	associatedZIDNumber; /**< previous array length */
	uint8_t 	*peerURI; /**< the sip URI associated to all the keys, must be a null terminated string */
} limeURIKeys_t;

/**
 * @brief Get from cache all the senders keys associated to the given URI
 * peerKeys field from associatedKeys param must be NULL when calling this function.
 * Structure content must then be freed using lime_freeKeys function
 *
 * @param[in]		cacheBuffer		The xmlDoc containing current cache
 * @param[in,out]	associatedKeys	Structure containing the peerURI. After this call contains all key material associated to the given URI. Must be then freed through lime_freeKeys function
 *
 * @return 0 on success, error code otherwise
 */
LINPHONE_PUBLIC int lime_getCachedSndKeysByURI(xmlDocPtr cacheBuffer, limeURIKeys_t *associatedKeys);

/**
 * @brief Get the receiver key associated to the ZID given in the associatedKey parameter
 *
 * @param[in]		cacheBuffer		The xmlDoc containing current cache
 * @param[in,out]	associatedKey	Structure containing the peerZID and will store the retrieved key
 *
 * @return 0 on success, error code otherwise
 */
LINPHONE_PUBLIC int lime_getCachedRcvKeyByZid(xmlDocPtr cacheBuffer, limeKey_t *associatedKey);

/**
 * @brief Set in cache the given key material, association is made by ZID contained in the associatedKey parameter
 *
 * @param[out]		cacheBuffer		The xmlDoc containing current cache to be updated
 * @param[in,out]	associatedKey	Structure containing the key and ZID to identify the peer node to be updated
 * @param[in]		role			Can be LIME_SENDER or LIME_RECEIVER, specify which key we want to update
 *
 * @return 0 on success, error code otherwise
 */

LINPHONE_PUBLIC int lime_setCachedKey(xmlDocPtr cacheBuffer, limeKey_t *associatedKey, uint8_t role);

/**
 * @brief Free all allocated data in the associated keys structure
 * Note, this will also free the peerURI string which then must have been allocated
 * This does not free the memory area pointed by associatedKeys.
 *
 * @param[in,out]	associatedKeys	The structure to be cleaned
 *
 */
LINPHONE_PUBLIC void lime_freeKeys(limeURIKeys_t *associatedKeys);

/**
 * @brief encrypt a message with the given key
 *
 * @param[in]	key					Key to use: first 192 bits are used as key, last 64 bits as init vector
 * @param[in]	plainMessage		The string to be encrypted
 * @param[in]	messageLength		The length in bytes of the message to be encrypted
 * @param[in]	selfZID				The self ZID is use in authentication tag computation
 * @param[out]	encryptedMessage	A buffer to hold the output, ouput length is input's one + 16 for the authentication tag
 * 									Authentication tag is set at the begining of the encrypted Message
 *
 * @return 0 on success, error code otherwise
 *
 */
LINPHONE_PUBLIC int lime_encryptMessage(limeKey_t *key, uint8_t *plainMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *encryptedMessage);

/**
 * @brief Encrypt a file before transfering it to the server, encryption is done in several call, first one will be done with cryptoContext null, last one with length = 0
 *
 * @param[in,out]	cryptoContext		The context used to encrypt the file using AES-GCM. Is created at first call(if null)
 * @param[in]		key					256 bits : 192 bits of key || 64 bits of Initial Vector
 * @param[in]		length				Length of data to be encrypted, if 0 it will conclude the encryption
 * @param[in]		plain				Plain data to be encrypted (length bytes)
 * @param[out]		cipher				Output to a buffer allocated by caller, at least length bytes available
 *
 * @return 0 on success, error code otherwise
 *
 */
LINPHONE_PUBLIC int lime_encryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher);

/**
 * @brief Decrypt a file retrieved from server, decryption is done in several call, first one will be done with cryptoContext null, last one with length = 0
 *
 * @param[in,out]	cryptoContext		The context used to decrypt the file using AES-GCM. Is created at first call(if null)
 * @param[in]		key					256 bits : 192 bits of key || 64 bits of Initial Vector
 * @param[in]		length				Length of data to be decrypted, if 0 it will conclude the decryption
 * @param[out]		plain				Output to a buffer allocated by caller, at least length bytes available
 * @param[in]		cipher				Cipher text to be decrypted(length bytes)
 *
 * @return 0 on success, error code otherwise
 *
 */
LINPHONE_PUBLIC int lime_decryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher);

/**
 * @brief decrypt and authentify a message with the given key
 *
 * @param[in]	key					Key to use: first 192 bits are used as key, last 64 bits as init vector
 * @param[in]	encryptedMessage	The string to be decrypted
 * @param[in]	messageLength		The length in bytes of the message to be decrypted (this include the 16 bytes tag at the begining of the message)
 * @param[in]	selfZID				The self ZID is use in authentication tag computation
 * @param[out]	plainMessage		A buffer to hold the output, ouput length is input's one - 16 for the authentication tag + 1 for null termination char
 * 									Authentication tag is retrieved at the begining of the encrypted Message
 *
 * @return 0 on success, error code otherwise
 *
 */

LINPHONE_PUBLIC int lime_decryptMessage(limeKey_t *key, uint8_t *encryptedMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *plainMessage);

/**
 * @brief create the encrypted multipart xml message from plain text and destination URI
 * Retrieve in cache the needed keys which are then updated. Output buffer is allocated and must be freed by caller
 *
 * @param[in,out]	cacheBuffer		The xmlDoc containing current cache, get the keys and selfZID from it, updated by this function with derivated keys
 * @param[in]		message			The plain text message to be encrypted
 * @param[in]		peerURI			The destination URI, associated keys will be found in cache
 * @param[out]		output			The output buffer, allocated and set with the encrypted message xml body(null terminated string). Must be freed by caller
 *
 * @return 	0 on success, error code otherwise
 */
LINPHONE_PUBLIC int lime_createMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t *peerURI, uint8_t **output);

/**
 * @brief decrypt a multipart xml message
 * Retrieve in cache the needed key which is then updated. Output buffer is allocated and must be freed by caller
 *
 * @param[in,out]	cacheBuffer		The xmlDoc containing current cache, get the key and selfZID from it, updated by this function with derivated keys
 * @param[in]		message			The multipart message, contain one or several part identified by destination ZID, one shall match the self ZID retrieved from cache
 * @param[out]		output			The output buffer, allocated and set with the decrypted message(null terminated string). Must be freed by caller
 *
 * @return 	0 on success, error code otherwise
 */

LINPHONE_PUBLIC int lime_decryptMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t **output);

/**
 * @brief given a readable version of error code generated by Lime functions
 * @param[in]	errorCode	The error code
 * @return a string containing the error description
 */
LINPHONE_PUBLIC char *lime_error_code_to_string(int errorCode);

/**
 * @brief Check if Lime was enabled at build time
 *
 * @return TRUE if Lime is available, FALSE if not
 */
LINPHONE_PUBLIC bool_t lime_is_available(void);

int lime_im_encryption_engine_process_incoming_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

int lime_im_encryption_engine_process_outgoing_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

int lime_im_encryption_engine_process_downloading_file_cb(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, const char *buffer, size_t size, char *decrypted_buffer);

int lime_im_encryption_engine_process_uploading_file_cb(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const char *buffer, size_t *size, char *encrypted_buffer);

bool_t lime_im_encryption_engine_is_file_encryption_enabled_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room);

void lime_im_encryption_engine_generate_file_transfer_key_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

#endif /* LIME_H */
