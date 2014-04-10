#ifndef LIME_H
#define LIME_H

#define LIME_INVALID_CACHE	0x1001
#define LIME_UNABLE_TO_DERIVE_KEY 0x1002
#define LIME_UNABLE_TO_ENCRYPT_MESSAGE 0x1004
#define LIME_UNABLE_TO_DECRYPT_MESSAGE 0x1008
#define LIME_NO_KEY_FOUND_FOR_PEER	0x1010

#define LIME_SENDER	0x01
#define LIME_RECEIVER 0x02
#include <stdint.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>

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
 * @brief Retrieve selfZID from cache
 *
 * @param[in]	cacheBuffer		The xmlDoc containing current cache
 * @param[out]	selfZid			The ZID found as a 24 hexa char string null terminated
 *
 * @return 0 on success, error code otherwise
 */
__attribute__ ((visibility ("default"))) int lime_getSelfZid(xmlDocPtr cacheBuffer, uint8_t selfZid[25]);

/**
 * @brief Get from cache all the senders keys associated to the given URI
 * peerKeys field from associatedKeys param must be NULL when calling this function.
 * Structure content must then be freed using lime_freeKeys function
 *
 * @param[in]		cacheBuffer		The xmlDoc containing current cache
 * @param[in/out]	associatedKeys	Structure containing the peerURI. After this call contains all key material associated to the given URI. Must be then freed through lime_freeKeys function
 *
 * @return 0 on success, error code otherwise
 */
__attribute__ ((visibility ("default"))) int lime_getCachedSndKeysByURI(xmlDocPtr cacheBuffer, limeURIKeys_t *associatedKeys);

/**
 * @brief Get the receiver key associated to the ZID given in the associatedKey parameter
 *
 * @param[in]		cacheBuffer		The xmlDoc containing current cache
 * @param[in/out]	associatedKey	Structure containing the peerZID and will store the retrieved key
 *
 * @return 0 on success, error code otherwise
 */
__attribute__ ((visibility ("default"))) int lime_getCachedRcvKeyByZid(xmlDocPtr cacheBuffer, limeKey_t *associatedKey);

/**
 * @brief Set in cache the given key material, association is made by ZID contained in the associatedKey parameter
 *
 * @param[out]		cacheBuffer		The xmlDoc containing current cache to be updated
 * @param[in/out]	associatedKey	Structure containing the key and ZID to identify the peer node to be updated
 * @param[in]		role			Can be LIME_SENDER or LIME_RECEIVER, specify which key we want to update 
 *
 * @return 0 on success, error code otherwise
 */

__attribute__ ((visibility ("default"))) int lime_setCachedKey(xmlDocPtr cacheBuffer, limeKey_t *associatedKey, uint8_t role);

/**
 * @brief Free all allocated data in the associated keys structure
 * Note, this will also free the peerURI string which then must have been allocated
 *
 * @param[in/out]	associatedKeys	The structure to be cleaned
 *
 */
__attribute__ ((visibility ("default"))) void lime_freeKeys(limeURIKeys_t associatedKeys);

/**
 * @brief Derive in place the key given in parameter and increment session index
 * Derivation is made derived Key = HMAC_SHA256(Key, 0x0000001||"MessageKey"||0x00||SessionId||SessionIndex||256)
 *
 * @param[in/out]	key		The structure containing the original key which will be overwritten, the sessionId and SessionIndex
 *
 * @return 0 on success, error code otherwise
 */
__attribute__ ((visibility ("default"))) int lime_deriveKey(limeKey_t *key);

/**
 * @brief encrypt a message with the given key
 * 
 * @param[in]	key					Key to use: first 192 bits are used as key, last 64 bits as init vector
 * @param[in]	message				The string to be encrypted
 * @param[in]	messageLength		The length in bytes of the message to be encrypted
 * @param[in]	selfZID				The self ZID is use in authentication tag computation
 * @param[out]	encryptedMessage	A buffer to hold the output, ouput length is input's one + 16 for the authentication tag
 * 									Authentication tag is set at the begining of the encrypted Message
 *
 * @return 0 on success, error code otherwise
 * 
 */
__attribute__ ((visibility ("default"))) int lime_encryptMessage(limeKey_t *key, uint8_t *plainMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *encryptedMessage);

/**
 * @brief decrypt and authentify a message with the given key
 * 
 * @param[in]	key					Key to use: first 192 bits are used as key, last 64 bits as init vector
 * @param[in]	message				The string to be decrypted
 * @param[in]	messageLength		The length in bytes of the message to be decrypted (this include the 16 bytes tag at the begining of the message)
 * @param[in]	selfZID				The self ZID is use in authentication tag computation
 * @param[out]	plainMessage		A buffer to hold the output, ouput length is input's one - 16 for the authentication tag + 1 for null termination char
 * 									Authentication tag is retrieved at the begining of the encrypted Message
 *
 * @return 0 on success, error code otherwise
 * 
 */

__attribute__ ((visibility ("default"))) int lime_decryptMessage(limeKey_t *key, uint8_t *encryptedMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *plainMessage);

/**
 * @brief create the encrypted multipart xml message from plain text and destination URI
 * Retrieve in cache the needed keys which are then updated. Output buffer is allocated and must be freed by caller
 *
 * @param[in/out]	cacheBuffer		The xmlDoc containing current cache, get the keys and selfZID from it, updated by this function with derivated keys
 * @param[in]		message			The plain text message to be encrypted
 * @param[in]		peerURI			The destination URI, associated keys will be found in cache
 * @param[out]		output			The output buffer, allocated and set with the encrypted message xml body(null terminated string). Must be freed by caller
 *
 * @return 	0 on success, error code otherwise
 */
__attribute__ ((visibility ("default"))) int lime_createMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t *peerURI, uint8_t **output);
__attribute__ ((visibility ("default"))) int lime_decryptMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t **output);
#endif /* LIME_H */
