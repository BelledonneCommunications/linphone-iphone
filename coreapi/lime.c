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

#include "lime.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIME
#include "private.h"
#include "bctoolbox/crypto.h"
#include "bctoolbox/port.h"

#define FILE_TRANSFER_KEY_SIZE 32

/**
 * @brief check at runtime if LIME is available
 *
 * @return TRUE when Lime was fully compiled, FALSE when it wasn't
 */
bool_t lime_is_available(void) { return TRUE; }

/**
 * @brief Retrieve selfZID from cache
 *
 * @param[in]	cacheBuffer		The xmlDoc containing current cache
 * @param[out]	selfZid			The ZID found as a 24 hexa char string null terminated
 *
 * @return 0 on success, error code otherwise
 */
static int lime_getSelfZid(xmlDocPtr cacheBuffer, uint8_t selfZid[25]) {
	xmlNodePtr cur;
	xmlChar *selfZidHex;

	if (cacheBuffer == NULL ) {
		return LIME_INVALID_CACHE;
	}

	cur = xmlDocGetRootElement(cacheBuffer);
	/* if we found a root element, parse its children node */
	if (cur!=NULL)
	{
		cur = cur->xmlChildrenNode;
	}
	selfZidHex = NULL;
	while (cur!=NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"selfZID"))){ /* self ZID found, extract it */
			selfZidHex = xmlNodeListGetString(cacheBuffer, cur->xmlChildrenNode, 1);
			/* copy it to the output buffer and add the null termination */
			memcpy(selfZid, selfZidHex, 24);
			selfZid[24]='\0';
			break;
		}
		cur = cur->next;
	}

	/* did we found a ZID? */
	if (selfZidHex == NULL) {
		return LIME_INVALID_CACHE;
	}

	xmlFree(selfZidHex);
	return 0;
}

int lime_getCachedSndKeysByURI(xmlDocPtr cacheBuffer, limeURIKeys_t *associatedKeys) {
	xmlNodePtr cur;
	size_t keysFound = 0; /* used to detect the no key found error because of validity expired */

	/* parse the file to get all peer matching the sipURI given in associatedKeys*/
	if (cacheBuffer == NULL ) { /* there is no cache return error */
		return LIME_INVALID_CACHE;
	}

	/* reset number of associated keys and their buffer */
	associatedKeys->associatedZIDNumber = 0;
	associatedKeys->peerKeys = NULL;

	cur = xmlDocGetRootElement(cacheBuffer);
	/* if we found a root element, parse its children node */
	if (cur!=NULL)
	{
		cur = cur->xmlChildrenNode;
	}
	while (cur!=NULL) { /* loop on all peer nodes */
		uint8_t matchingURIFlag = 0; /* this flag is set to one if we found the requested sipURI in the current peer node */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"peer"))) { /* found a peer node, check if there is a matching sipURI node in it */
			xmlNodePtr peerNodeChildren = cur->xmlChildrenNode;
			matchingURIFlag = 0;

			/* loop on children nodes until the end or we found the matching sipURI */
			while (peerNodeChildren!=NULL && matchingURIFlag==0) {
				if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"uri")) { /* found a peer an URI node, check the content */
					xmlChar *uriNodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
					if (!xmlStrcmp(uriNodeContent, (const xmlChar *)associatedKeys->peerURI)) { /* found a match with requested URI */
						matchingURIFlag=1;
					}
					xmlFree(uriNodeContent);
				}
				peerNodeChildren = peerNodeChildren->next;
			}

			if (matchingURIFlag == 1) { /* we found a match for the URI in this peer node, extract the keys, session Id, index values and key validity period */
				/* allocate a new limeKey_t structure to hold the retreived keys */
				limeKey_t *currentPeerKeys = (limeKey_t *)malloc(sizeof(limeKey_t));
				uint8_t itemFound = 0; /* count the item found, we must get all of the requested infos: 5 nodes*/
				uint8_t pvs = 0;
				uint8_t keyValidityFound = 0; /* flag raised when we found a key validity in the cache */
				bctoolboxTimeSpec currentTimeSpec;
				bctoolboxTimeSpec validityTimeSpec; /* optionnal(backward compatibility) tag for key validity */
				validityTimeSpec.tv_sec=0;
				validityTimeSpec.tv_nsec=0;

				peerNodeChildren = cur->xmlChildrenNode; /* reset peerNodeChildren to the first child of node */
				while (peerNodeChildren!=NULL && itemFound<6) {
					xmlChar *nodeContent = NULL;
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"ZID")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(currentPeerKeys->peerZID, nodeContent, 24);
						itemFound++;
					}

					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndKey")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(currentPeerKeys->key, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndSId")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(currentPeerKeys->sessionId, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndIndex")) {
						uint8_t sessionIndexBuffer[4]; /* session index is a uint32_t but we first retrieved it as an hexa string, convert it to a 4 uint8_t buffer */
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(sessionIndexBuffer, nodeContent, 8);
						/* convert it back to a uint32_t (MSByte first)*/
						currentPeerKeys->sessionIndex = sessionIndexBuffer[3] + (sessionIndexBuffer[2]<<8) + (sessionIndexBuffer[1]<<16) + (sessionIndexBuffer[0]<<24);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"pvs")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(&pvs, nodeContent, 2); /* pvs is retrieved as a 2 characters hexa string, convert it to an int8 */
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"valid")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						validityTimeSpec.tv_sec = bctbx_strToUint64(nodeContent); /* validity is retrieved as a 16 characters hexa string, convert it to an uint64 */
						itemFound++;
						keyValidityFound = 1;
					}

					xmlFree(nodeContent);
					peerNodeChildren = peerNodeChildren->next;
				}

				/* key validity may not be present in cache(transition from older versions), so check this, if not found-> set it to 0 wich means valid for ever */
				if (keyValidityFound == 0) {
					itemFound++;
					validityTimeSpec.tv_sec = 0;
				}

				/* check if we have all the requested information and the PVS flag is set to 1 and key is still valid*/
				bctbx_get_utc_cur_time(&currentTimeSpec);
				if (itemFound == 6 && pvs == 1) {
					keysFound++;
					if (validityTimeSpec.tv_sec == 0 || bctbx_timespec_compare(&currentTimeSpec, &validityTimeSpec)<0) {
						associatedKeys->associatedZIDNumber +=1;
						/* extend array of pointer to limeKey_t structures to add the one we found */
						associatedKeys->peerKeys = (limeKey_t **)realloc(associatedKeys->peerKeys, (associatedKeys->associatedZIDNumber)*sizeof(limeKey_t *));

						/* add the new entry at the end */
						associatedKeys->peerKeys[associatedKeys->associatedZIDNumber-1] = currentPeerKeys;
					} else {
						free(currentPeerKeys);
					}
				} else {
					free(currentPeerKeys);
				}
			}
		}
		cur = cur->next;
	}
	if (associatedKeys->associatedZIDNumber == 0) {
		if (keysFound == 0) {
			return LIME_NO_VALID_KEY_FOUND_FOR_PEER;
		} else {
			return LIME_PEER_KEY_HAS_EXPIRED;
		}
	}
	return 0;
}

int lime_getCachedRcvKeyByZid(xmlDocPtr cacheBuffer, limeKey_t *associatedKey) {
	uint8_t peerZidHex[25];
	/* to check we collect all the information needed from the cache and that pvs(boolean for previously verified Sas) is set in cache */
	uint8_t itemFound = 0;
	uint8_t pvs = 0;
	xmlNodePtr cur;

	if (cacheBuffer == NULL ) { /* there is no cache return error */
		return LIME_INVALID_CACHE;
	}

	/* get the given ZID into hex format */
	bctbx_int8ToStr(peerZidHex, associatedKey->peerZID, 12);
	peerZidHex[24]='\0'; /* must be a null terminated string */

	cur = xmlDocGetRootElement(cacheBuffer);
	/* if we found a root element, parse its children node */
	if (cur!=NULL)
	{
		cur = cur->xmlChildrenNode;

	}

	while (cur!=NULL) { /* loop on all peer nodes */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"peer"))){ /* found a peer, check his ZID element */
			xmlChar *currentZidHex = xmlNodeListGetString(cacheBuffer, cur->xmlChildrenNode->xmlChildrenNode, 1); /* ZID is the first element of peer */
			if (!xmlStrcmp(currentZidHex, (const xmlChar *)peerZidHex)) { /* we found the peer element we are looking for */
				xmlNodePtr peerNodeChildren = cur->xmlChildrenNode->next;
				while (peerNodeChildren != NULL && itemFound<4) { /* look for the tag we want to read : rcvKey, rcvSId, rcvIndex and pvs*/
					xmlChar *nodeContent = NULL;
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvKey")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(associatedKey->key, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvSId")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(associatedKey->sessionId, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvIndex")) {
						uint8_t sessionIndexBuffer[4]; /* session index is a uint32_t but we first retrieved it as an hexa string, convert it to a 4 uint8_t buffer */
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(sessionIndexBuffer, nodeContent, 8);
						/* convert it back to a uint32_t (MSByte first)*/
						associatedKey->sessionIndex = sessionIndexBuffer[3] + (sessionIndexBuffer[2]<<8) + (sessionIndexBuffer[1]<<16) + (sessionIndexBuffer[0]<<24);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"pvs")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						bctbx_strToUint8(&pvs, nodeContent, 2); /* pvs is retrieved as a 2 characters hexa string, convert it to an int8 */
						itemFound++;
					}
					xmlFree(nodeContent);
					peerNodeChildren = peerNodeChildren->next;
				}
				xmlFree(currentZidHex);
				break; /* we parsed the peer node we were looking for, get out of the main while */
			}
			xmlFree(currentZidHex);
		}
		cur = cur->next;
	}

	/* if we manage to find the correct key information and that pvs is set to 1, return 0 (success) */
	if ((pvs == 1) && (itemFound == 4)) {
		return 0;
	}

	/* otherwise, key wasn't found or is invalid */
	return LIME_NO_VALID_KEY_FOUND_FOR_PEER;
}

int lime_setCachedKey(xmlDocPtr cacheBuffer, limeKey_t *associatedKey, uint8_t role, uint64_t validityTimeSpan) {
	uint8_t done=0;
	xmlNodePtr cur;
	uint8_t peerZidHex[25];
	uint8_t keyHex[65]; /* key is 32 bytes long -> 64 bytes string + null termination */
	uint8_t sessionIdHex[65]; /* sessionId is 32 bytes long -> 64 bytes string + null termination */
	uint8_t sessionIndexHex[9]; /*  sessionInedx is an uint32_t : 4 bytes long -> 8 bytes string + null termination */
	uint8_t validHex[17]; /* validity is a unix period in seconds on 64bits -> 16 bytes string + null termination */
	bctoolboxTimeSpec currentTimeSpec;

	uint8_t itemFound = 0;

	if (cacheBuffer == NULL ) { /* there is no cache return error */
		return LIME_INVALID_CACHE;
	}

	/* get the given ZID into hex format */
	bctbx_int8ToStr(peerZidHex, associatedKey->peerZID, 12);
	peerZidHex[24]='\0'; /* must be a null terminated string */

	cur = xmlDocGetRootElement(cacheBuffer);
	/* if we found a root element, parse its children node */
	if (cur!=NULL)
	{
		cur = cur->xmlChildrenNode;

	}

	/* convert the given tag content to null terminated Hexadecimal strings */
	bctbx_int8ToStr(keyHex, associatedKey->key, 32);
	keyHex[64] = '\0';
	bctbx_int8ToStr(sessionIdHex, associatedKey->sessionId, 32);
	sessionIdHex[64] = '\0';
	bctbx_uint32ToStr(sessionIndexHex, associatedKey->sessionIndex);
	if (validityTimeSpan > 0 && role == LIME_RECEIVER) {
		bctbx_get_utc_cur_time(&currentTimeSpec);
		bctbx_timespec_add(&currentTimeSpec, validityTimeSpan);
		bctbx_uint64ToStr(validHex, currentTimeSpec.tv_sec);
	}

	while (cur!=NULL && done==0) { /* loop on all peer nodes */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"peer"))){ /* found a peer, check his ZID element */
			xmlChar *currentZidHex = xmlNodeListGetString(cacheBuffer, cur->xmlChildrenNode->xmlChildrenNode, 1); /* ZID is the first element of peer */
			if (!xmlStrcmp(currentZidHex, (const xmlChar *)peerZidHex)) { /* we found the peer element we are looking for */
				uint8_t validFound = 0;
				xmlNodePtr peerNodeChildren = cur->xmlChildrenNode->next;
				if (role==LIME_SENDER) {
					itemFound=1; /* we do not look for valid when setting sender key */
				}
				while (peerNodeChildren != NULL && itemFound<4) { /* look for the tag we want to write */
					if (role == LIME_RECEIVER) { /* writing receiver key */
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvKey")) {
							xmlNodeSetContent(peerNodeChildren, (const xmlChar *)keyHex);
							itemFound++;
						}
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvSId")) {
							xmlNodeSetContent(peerNodeChildren, (const xmlChar *)sessionIdHex);
							itemFound++;
						}
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvIndex")) {
							xmlNodeSetContent(peerNodeChildren, (const xmlChar *)sessionIndexHex);
							itemFound++;
						}
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"valid")) {
							if (validityTimeSpan > 0) {
								xmlNodeSetContent(peerNodeChildren, (const xmlChar *)validHex);
							}
							itemFound++;
							validFound=1;
						}
					} else { /* writing sender key */
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndKey")) {
							xmlNodeSetContent(peerNodeChildren, (const xmlChar *)keyHex);
							itemFound++;
						}
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndSId")) {
							xmlNodeSetContent(peerNodeChildren, (const xmlChar *)sessionIdHex);
							itemFound++;
						}
						if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndIndex")) {
							xmlNodeSetContent(peerNodeChildren, (const xmlChar *)sessionIndexHex);
							itemFound++;
						}
					}
					peerNodeChildren = peerNodeChildren->next;
				}

				/* we may want to add the valid node which if it is missing and we've been ask to update it */
				if (role == LIME_RECEIVER && validityTimeSpan>0 && validFound==0) {
					xmlNewTextChild(cur, NULL, (const xmlChar *)"valid", validHex);
				}
				done=1; /* step out of the <peer> loop  */
			}
			xmlFree(currentZidHex);
		}
		cur = cur->next;
	}
	return 0;
}

/**
 * @brief Derive in place the key given in parameter and increment session index
 * Derivation is made derived Key = HMAC_SHA256(Key, 0x0000001||"MessageKey"||0x00||SessionId||SessionIndex||256)
 *
 * @param[in/out]	key		The structure containing the original key which will be overwritten, the sessionId and SessionIndex
 *
 * @return 0 on success, error code otherwise
 */
static int lime_deriveKey(limeKey_t *key) {
	uint8_t inputData[55];
	uint8_t derivedKey[32];

	if (key == NULL) {
		return LIME_UNABLE_TO_DERIVE_KEY;
	}

 	/* Derivation is made derived Key = HMAC_SHA256(Key, 0x0000001||"MessageKey"||0x00||SessionId||SessionIndex||0x00000100)*/
	/* total data to be hashed is       55 bytes  :           4   +      10     +   1 +     32   +   4         +   4 */
	inputData[0] = 0x00;
	inputData[1] = 0x00;
	inputData[2] = 0x00;
	inputData[3] = 0x01;

	memcpy(inputData+4, "MessageKey", 10);

	inputData[14] = 0x00;

	memcpy(inputData+15, key->sessionId, 32);

	inputData[47] = (uint8_t)((key->sessionIndex>>24)&0x000000FF);
	inputData[48] = (uint8_t)((key->sessionIndex>>16)&0x000000FF);
	inputData[49] = (uint8_t)((key->sessionIndex>>8)&0x000000FF);
	inputData[50] = (uint8_t)(key->sessionIndex&0x000000FF);

	inputData[51] = 0x00;
	inputData[52] = 0x00;
	inputData[53] = 0x01;
	inputData[54] = 0x00;

	/* derive the key in a temp buffer */
	bctbx_hmacSha256(key->key, 32, inputData, 55, 32, derivedKey);

	/* overwrite the old key with the derived one */
	memcpy(key->key, derivedKey, 32);

	/* increment the session Index */
	key->sessionIndex += 1;
	return 0;
}

void lime_freeKeys(limeURIKeys_t *associatedKeys) {
	int i;

	/* free all associated keys */
	for (i=0; i< associatedKeys->associatedZIDNumber; i++) {
		if (associatedKeys->peerKeys[i] != NULL) {
			/*shouldn't we memset to zero the content of peerKeys[i] in order clear keys?*/
			free(associatedKeys->peerKeys[i]);
			associatedKeys->peerKeys[i] = NULL;
		}
	}

	free(associatedKeys->peerKeys);
	associatedKeys->peerKeys = NULL;

	/* free sipURI string */
	free(associatedKeys->peerURI);
	associatedKeys->peerURI = NULL;
}

int lime_encryptMessage(limeKey_t *key, const uint8_t *plainMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *encryptedMessage) {
	uint8_t authenticatedData[28];
	/* Authenticated data is senderZID(12 bytes)||receiverZID(12 bytes)||sessionIndex(4 bytes) */
	memcpy(authenticatedData, selfZID, 12);
	memcpy(authenticatedData+12, key->peerZID, 12);
	authenticatedData[24] = (uint8_t)((key->sessionIndex>>24)&0x000000FF);
	authenticatedData[25] = (uint8_t)((key->sessionIndex>>16)&0x000000FF);
	authenticatedData[26] = (uint8_t)((key->sessionIndex>>8)&0x000000FF);
	authenticatedData[27] = (uint8_t)(key->sessionIndex&0x000000FF);

	/* AES-GCM : key is 192 bits long, Init Vector 64 bits. 256 bits key given is AES key||IV */
	/* tag is 16 bytes long and is set in the 16 first bytes of the encrypted message */
	return bctbx_aes_gcm_encrypt_and_tag(key->key, 24,
			plainMessage, messageLength,
			authenticatedData, 28,
			key->key+24, 8, /* IV is at the end(last 64 bits) of the given key buffer */
			encryptedMessage, 16, /* the first 16 bytes of output are the authentication tag */
			encryptedMessage+16); /* actual encrypted message starts after 16 bytes of authentication tag */

	return 0;
}

int lime_encryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {
	bctbx_aes_gcm_context_t *gcmContext;

	if (*cryptoContext == NULL) { /* first call to the function, allocate a crypto context and initialise it */
		/* key contains 192bits of key || 64 bits of Initialisation Vector, no additional data */
		gcmContext = bctbx_aes_gcm_context_new(key, 24, NULL, 0, key+24, 8, BCTBX_GCM_ENCRYPT);
		*cryptoContext = gcmContext;
	} else { /* this is not the first call, get the context */
		gcmContext = (bctbx_aes_gcm_context_t *)*cryptoContext;
	}

	if (length != 0) {
		bctbx_aes_gcm_process_chunk(gcmContext, (const uint8_t *)plain, length, (uint8_t *)cipher);
	} else { /* lenght is 0, finish the stream, no tag to be generated */
		bctbx_aes_gcm_finish(gcmContext, NULL, 0);
		*cryptoContext = NULL;
	}

	return 0;
}

int lime_decryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {
	bctbx_aes_gcm_context_t *gcmContext;

	if (*cryptoContext == NULL) { /* first call to the function, allocate a crypto context and initialise it */
		/* key contains 192bits of key || 64 bits of Initialisation Vector, no additional data */
		gcmContext = bctbx_aes_gcm_context_new(key, 24, NULL, 0, key+24, 8, BCTBX_GCM_DECRYPT);
		*cryptoContext = gcmContext;
	} else { /* this is not the first call, get the context */
		gcmContext = (bctbx_aes_gcm_context_t *)*cryptoContext;
	}

	if (length != 0) {
		bctbx_aes_gcm_process_chunk(gcmContext, (const unsigned char *)cipher, length, (unsigned char *)plain);
	} else { /* lenght is 0, finish the stream */
		bctbx_aes_gcm_finish(gcmContext, NULL, 0);
		*cryptoContext = NULL;
	}

	return 0;
}


int lime_decryptMessage(limeKey_t *key, uint8_t *encryptedMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *plainMessage) {
	uint8_t authenticatedData[28];
	int retval;

	/* Authenticated data is senderZID(12 bytes)||receiverZID(12 bytes)||sessionIndex(4 bytes) */
	memcpy(authenticatedData, key->peerZID, 12);
	memcpy(authenticatedData+12, selfZID, 12);
	authenticatedData[24] = (uint8_t)((key->sessionIndex>>24)&0x000000FF);
	authenticatedData[25] = (uint8_t)((key->sessionIndex>>16)&0x000000FF);
	authenticatedData[26] = (uint8_t)((key->sessionIndex>>8)&0x000000FF);
	authenticatedData[27] = (uint8_t)(key->sessionIndex&0x000000FF);

	/* AES-GCM : key is 192 bits long, Init Vector 64 bits. 256 bits key given is AES key||IV */
	/* tag is 16 bytes long and is the 16 first bytes of the encrypted message */
	retval = bctbx_aes_gcm_decrypt_and_auth(key->key, 24, /* key is 192 bits long */
			encryptedMessage+16,  messageLength-16, /* encrypted message first 16 bytes store the authentication tag, then is the actual message */
			authenticatedData, 28, /* additionnal data needed for authentication */
			key->key+24, 8, /* last 8 bytes of key is the initialisation vector */
			encryptedMessage, 16, /* first 16 bytes of message is the authentication tag */
			plainMessage);

	/* add the null termination char */
	plainMessage[messageLength-16] = '\0';

	return retval;
}

int lime_createMultipartMessage(xmlDocPtr cacheBuffer, const char *contentType, uint8_t *message, uint8_t *peerURI, uint8_t **output) {
	uint8_t selfZidHex[25];
	uint8_t selfZid[12]; /* same data but in byte buffer */
	uint32_t encryptedMessageLength;
	uint32_t encryptedContentTypeLength;
	limeURIKeys_t associatedKeys;
	xmlDocPtr xmlOutputMessage;
	xmlNodePtr rootNode;
	int i,ret;
	int xmlStringLength;
	xmlChar *local_output = NULL;

	/* retrieve selfZIDHex from cache(return a 24 char hexa string + null termination) */
	if (lime_getSelfZid(cacheBuffer, selfZidHex) != 0) {
		return LIME_UNABLE_TO_ENCRYPT_MESSAGE;
	}
	bctbx_strToUint8(selfZid, selfZidHex, 24);

	/* encrypted message length is plaintext + 16 for tag */
	encryptedMessageLength = (uint32_t)strlen((char *)message) + 16;
	encryptedContentTypeLength = (uint32_t)strlen((char *)contentType) + 16;

	/* retrieve keys associated to the peer URI */
	associatedKeys.peerURI = (uint8_t *)malloc(strlen((char *)peerURI)+1);
	strcpy((char *)(associatedKeys.peerURI), (char *)peerURI);
	associatedKeys.associatedZIDNumber  = 0;
	associatedKeys.peerKeys = NULL;

	if ((ret = lime_getCachedSndKeysByURI(cacheBuffer, &associatedKeys)) != 0) {
		lime_freeKeys(&associatedKeys);
		return ret;
	}

	/* create an xml doc to hold the multipart message */
	xmlOutputMessage = xmlNewDoc((const xmlChar *)"1.0");
	/* root tag is "doc" */
	rootNode = xmlNewDocNode(xmlOutputMessage, NULL, (const xmlChar *)"doc", NULL);
	xmlDocSetRootElement(xmlOutputMessage, rootNode);
	/* add the self ZID child */
	xmlNewTextChild(rootNode, NULL, (const xmlChar *)"ZID", selfZidHex);

	/* loop on all keys found */
	for (i=0; i<associatedKeys.associatedZIDNumber; i++) {
		uint8_t peerZidHex[25];
		uint8_t sessionIndexHex[9];
		xmlNodePtr msgNode;
		size_t b64Size = 0;
		unsigned char *encryptedMessageb64;
		unsigned char *encryptedContentTypeb64;

		/* encrypt message with current key */
		limeKey_t *currentKey = associatedKeys.peerKeys[i];
		/* encrypted message include a 16 bytes tag */
		uint8_t *encryptedMessage = (uint8_t *)ms_malloc(encryptedMessageLength);
		uint8_t *encryptedContentType = (uint8_t *)ms_malloc(encryptedContentTypeLength);
		lime_encryptMessage(currentKey, message, (uint32_t)strlen((char *)message), selfZid, encryptedMessage);
		lime_encryptMessage(currentKey, (const uint8_t *)contentType, (uint32_t)strlen((char *)contentType), selfZid, encryptedContentType);
		/* add a "msg" node the the output message, doc node is :
		 * <msg>
		 * 		<pzid>peerZID</pzid>
		 * 		<index>session index</index>
		 * 		<text>ciphertext</text>
		 * 		<content-type>ciphertext</content-type>
		 * </msg> */
		msgNode = xmlNewDocNode(xmlOutputMessage, NULL, (const xmlChar *)"msg", NULL);
		bctbx_int8ToStr(peerZidHex, currentKey->peerZID, 12);
		peerZidHex[24] = '\0';
		bctbx_uint32ToStr(sessionIndexHex, currentKey->sessionIndex);

		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"pzid", peerZidHex);
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"index", sessionIndexHex);

		/* convert the cipherText to base 64 */
		bctbx_base64_encode(NULL, &b64Size, encryptedMessage, encryptedMessageLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedMessageb64 = ms_malloc(b64Size+1); /* allocate a buffer of requested size +1 for NULL termination */
		bctbx_base64_encode(encryptedMessageb64, &b64Size, encryptedMessage, encryptedMessageLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedMessageb64[b64Size] = '\0'; /* libxml need a null terminated string */
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"text", (const xmlChar *)encryptedMessageb64);
		ms_free(encryptedMessage);
		ms_free(encryptedMessageb64);

		/* convert the encrypted content-type to base 64 */
		b64Size = 0;
		bctbx_base64_encode(NULL, &b64Size, encryptedContentType, encryptedContentTypeLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedContentTypeb64 = ms_malloc(b64Size+1); /* allocate a buffer of requested size +1 for NULL termination */
		bctbx_base64_encode(encryptedContentTypeb64, &b64Size, encryptedContentType, encryptedContentTypeLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedContentTypeb64[b64Size] = '\0'; /* libxml need a null terminated string */
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"content-type", (const xmlChar *)encryptedContentTypeb64);
		ms_free(encryptedContentType);
		ms_free(encryptedContentTypeb64);

		/* add the message Node into the doc */
		xmlAddChild(rootNode, msgNode);

		/* update the key used */
		lime_deriveKey(currentKey);
		lime_setCachedKey(cacheBuffer, currentKey, LIME_SENDER, 0); /* never update validity when sending a message */
	}

	/* dump the whole message doc into the output */
	xmlDocDumpFormatMemoryEnc(xmlOutputMessage, &local_output, &xmlStringLength, "UTF-8", 0);

	*output = (uint8_t *)ms_malloc(xmlStringLength + 1);
	memcpy(*output, local_output, xmlStringLength);
	(*output)[xmlStringLength] = '\0';

	xmlFree(local_output);
	xmlFreeDoc(xmlOutputMessage);
	lime_freeKeys(&associatedKeys);

	return 0;
}

int lime_decryptMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t **output, char **content_type, uint64_t validityTimeSpan) {
	int retval = 0;
	uint8_t selfZidHex[25];
	uint8_t selfZid[12]; /* same data but in byte buffer */
	char xpath_str[MAX_XPATH_LENGTH];
	limeKey_t associatedKey;
	const char *peerZidHex = NULL;
	xmlparsing_context_t *xml_ctx;
	xmlXPathObjectPtr msg_object;
	uint8_t *encryptedMessage = NULL;
	size_t encryptedMessageLength = 0;
	uint8_t *encryptedContentType = NULL;
	size_t encryptedContentTypeLength = 0;
	uint32_t usedSessionIndex = 0;
	int i;

	if (cacheBuffer == NULL) {
		return LIME_INVALID_CACHE;
	}
	/* retrieve selfZIDHex from cache(return a 24 char hexa string + null termination) */
	if (lime_getSelfZid(cacheBuffer, selfZidHex) != 0) {
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}
	bctbx_strToUint8(selfZid, selfZidHex, 24);

	xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)message, 0, NULL, 0);
	if (xml_ctx->doc == NULL) {
		retval = LIME_INVALID_ENCRYPTED_MESSAGE;
		goto error;
	}

	if (linphone_create_xml_xpath_context(xml_ctx) < 0) {
		retval = LIME_INVALID_ENCRYPTED_MESSAGE;
		goto error;
	}

	/* Retrieve the sender ZID */
	peerZidHex = linphone_get_xml_text_content(xml_ctx, "/doc/ZID");
	if (peerZidHex != NULL) {
		/* Convert it from hexa string to bytes string and set the result in the associatedKey structure */
		bctbx_strToUint8(associatedKey.peerZID, (const uint8_t *)peerZidHex, (uint16_t)strlen(peerZidHex));
		linphone_free_xml_text_content(peerZidHex);

		/* Get the matching key from cache */
		retval = lime_getCachedRcvKeyByZid(cacheBuffer, &associatedKey);
		if (retval != 0) {
			goto error;
		}

		/* Retrieve the portion of message which is encrypted with our key */
		msg_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/doc/msg");
		if ((msg_object != NULL) && (msg_object->nodesetval != NULL)) {
			for (i = 1; i <= msg_object->nodesetval->nodeNr; i++) {
				const char *currentZidHex;
				const char *sessionIndexHex;
				const char *encryptedMessageb64;
				const char *encryptedContentTypeb64;
				snprintf(xpath_str, sizeof(xpath_str), "/doc/msg[%i]/pzid", i);
				currentZidHex = linphone_get_xml_text_content(xml_ctx, xpath_str);
				if ((currentZidHex != NULL) && (strcmp(currentZidHex, (char *)selfZidHex) == 0)) {
					/* We found the msg node we are looking for */
					snprintf(xpath_str, sizeof(xpath_str), "/doc/msg[%i]/index", i);
					sessionIndexHex = linphone_get_xml_text_content(xml_ctx, xpath_str);
					if (sessionIndexHex != NULL) {
						usedSessionIndex = bctbx_strToUint32((const unsigned char *)sessionIndexHex);
						linphone_free_xml_text_content(sessionIndexHex);
					}
					snprintf(xpath_str, sizeof(xpath_str), "/doc/msg[%i]/text", i);
					encryptedMessageb64 = linphone_get_xml_text_content(xml_ctx, xpath_str);
					if (encryptedMessageb64 != NULL) {
						bctbx_base64_decode(NULL, &encryptedMessageLength, (const unsigned char *)encryptedMessageb64, strlen(encryptedMessageb64)); /* encryptedMessageLength is 0, so it will be set to the requested buffer length */
						encryptedMessage = (uint8_t *)ms_malloc(encryptedMessageLength);
						bctbx_base64_decode(encryptedMessage, &encryptedMessageLength, (const unsigned char *)encryptedMessageb64, strlen(encryptedMessageb64));
						linphone_free_xml_text_content(encryptedMessageb64);
					}
					snprintf(xpath_str, sizeof(xpath_str), "/doc/msg[%i]/content-type", i);
					encryptedContentTypeb64 = linphone_get_xml_text_content(xml_ctx, xpath_str);
					if (encryptedContentTypeb64 != NULL) {
						bctbx_base64_decode(NULL, &encryptedContentTypeLength, (const unsigned char *)encryptedContentTypeb64, strlen(encryptedContentTypeb64)); /* encryptedContentTypeLength is 0, so it will be set to the requested buffer length */
						encryptedContentType = (uint8_t *)ms_malloc(encryptedContentTypeLength);
						bctbx_base64_decode(encryptedContentType, &encryptedContentTypeLength, (const unsigned char *)encryptedContentTypeb64, strlen(encryptedContentTypeb64));
						linphone_free_xml_text_content(encryptedContentTypeb64);
					}
					break;
				}
				if (currentZidHex != NULL) linphone_free_xml_text_content(currentZidHex);
			}
		}
	}

	/* do we have retrieved correctly all the needed data */
	if (encryptedMessage == NULL) {
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	/* shall we derive our key before going for decryption */
	if (usedSessionIndex < associatedKey.sessionIndex) {
		/* something wen't wrong with the cache, this shall never happend */
		ms_free(encryptedMessage);
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	if ((usedSessionIndex - associatedKey.sessionIndex > MAX_DERIVATION_NUMBER) ) {
		/* we missed to many messages, ask for a cache reset via a ZRTP call */
		ms_free(encryptedMessage);
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	while (usedSessionIndex>associatedKey.sessionIndex) {
		lime_deriveKey(&associatedKey);
	}

	/* Decrypt the message */
	*output = (uint8_t *)ms_malloc(encryptedMessageLength - 16 + 1); /* plain message is same length than encrypted one with 16 bytes less for the tag + 1 to add the null termination char */
	retval = lime_decryptMessage(&associatedKey, encryptedMessage, (uint32_t)encryptedMessageLength, selfZid, *output);
	ms_free(encryptedMessage);
	if (retval != 0) {
		ms_free(*output);
		*output = NULL;
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	/* Decrypt the content-type */
	if (encryptedContentType != NULL) {
		*content_type = (char *)ms_malloc(encryptedContentTypeLength - 16 + 1); /* content-type is same length than encrypted one with 16 bytes less for the tag + 1 to add the null termination char */
		retval = lime_decryptMessage(&associatedKey, encryptedContentType, (uint32_t)encryptedContentTypeLength, selfZid, *((uint8_t **)content_type));
		ms_free(encryptedContentType);
		if (retval != 0) {
			ms_free(*content_type);
			*content_type = NULL;
			return LIME_UNABLE_TO_DECRYPT_MESSAGE;
		}
	}

	/* update used key */
	lime_deriveKey(&associatedKey);
	lime_setCachedKey(cacheBuffer, &associatedKey, LIME_RECEIVER, validityTimeSpan);

error:
	linphone_xmlparsing_context_destroy(xml_ctx);
	return retval;
}

bool_t linphone_chat_room_lime_available(LinphoneChatRoom *cr) {
	if (cr) {
		switch (linphone_core_lime_enabled(cr->lc)) {
			case LinphoneLimeDisabled: return FALSE;
			case LinphoneLimeMandatory:
			case LinphoneLimePreferred: {
				FILE *CACHEFD = NULL;
				if (cr->lc->zrtp_secrets_cache != NULL) {
					CACHEFD = fopen(cr->lc->zrtp_secrets_cache, "rb+");
					if (CACHEFD) {
						size_t cacheSize;
						xmlDocPtr cacheXml;
						char *cacheString = ms_load_file_content(CACHEFD, &cacheSize);
						if (!cacheString) {
							ms_warning("Unable to load content of ZRTP ZID cache");
							return FALSE;
						}
						cacheString[cacheSize] = '\0';
						cacheSize += 1;
						fclose(CACHEFD);
						cacheXml = xmlParseDoc((xmlChar*)cacheString);
						ms_free(cacheString);
						if (cacheXml) {
							bool_t res;
							limeURIKeys_t associatedKeys;
							char *peer = linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
							
							/* retrieve keys associated to the peer URI */
							associatedKeys.peerURI = (uint8_t *)malloc(strlen(peer)+1);
							strcpy((char *)(associatedKeys.peerURI), peer);
							associatedKeys.associatedZIDNumber  = 0;
							associatedKeys.peerKeys = NULL;

							res = (lime_getCachedSndKeysByURI(cacheXml, &associatedKeys) == 0);
							lime_freeKeys(&associatedKeys);
							xmlFreeDoc(cacheXml);
							ms_free(peer);
							return res;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

int lime_im_encryption_engine_process_incoming_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	LinphoneCore *lc = linphone_im_encryption_engine_get_core(engine);
	int errcode = -1;
	/* check if we have a xml/cipher message to be decrypted */
	if (msg->content_type && (strcmp("xml/cipher", msg->content_type) == 0 || strcmp("application/cipher.vnd.gsma.rcs-ft-http+xml", msg->content_type) == 0)) {
		/* access the zrtp cache to get keys needed to decipher the message */
		FILE *CACHEFD = NULL;
		const char *zrtp_secrets_cache = linphone_core_get_zrtp_secrets_file(lc);
		errcode = 0;
		if (zrtp_secrets_cache != NULL) CACHEFD = fopen(zrtp_secrets_cache, "rb+");
		if (CACHEFD == NULL) {
			ms_warning("Unable to access ZRTP ZID cache to decrypt message");
			errcode = 500;
			return errcode;
		} else {
			size_t cacheSize;
			char *cacheString;
			int retval;
			xmlDocPtr cacheXml;
			uint8_t *decrypted_body = NULL;
			char *decrypted_content_type = NULL;
			
			cacheString=ms_load_file_content(CACHEFD, &cacheSize);
			if (!cacheString){
				ms_warning("Unable to load content of ZRTP ZID cache to decrypt message");
				errcode = 500;
				return errcode;
			}
			cacheString[cacheSize] = '\0';
			cacheSize += 1;
			fclose(CACHEFD);
			cacheXml = xmlParseDoc((xmlChar*)cacheString);
			ms_free(cacheString);
			retval = lime_decryptMultipartMessage(cacheXml, (uint8_t *)msg->message, &decrypted_body, &decrypted_content_type, bctbx_time_string_to_sec(lp_config_get_string(lc->config, "sip", "lime_key_validity", "0")));
			if (retval != 0) {
				ms_warning("Unable to decrypt message, reason : %s", lime_error_code_to_string(retval));
				if (decrypted_body) ms_free(decrypted_body);
				xmlFreeDoc(cacheXml);
				errcode = 488;
				return errcode;
			} else {
				/* dump updated cache to a string */
				xmlChar *xmlStringOutput;
				int xmlStringLength;
				xmlDocDumpFormatMemoryEnc(cacheXml, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
				/* write it to the cache file */
				CACHEFD = fopen(zrtp_secrets_cache, "wb+");
				if (fwrite(xmlStringOutput, 1, xmlStringLength, CACHEFD)<=0){
					ms_warning("Fail to write cache");
				}
				xmlFree(xmlStringOutput);
				fclose(CACHEFD);
				if (msg->message) {
					ms_free(msg->message);
				}
				msg->message = (char *)decrypted_body;

				if (decrypted_content_type != NULL) {
					linphone_chat_message_set_content_type(msg, decrypted_content_type);
				} else {
					if (strcmp("application/cipher.vnd.gsma.rcs-ft-http+xml", msg->content_type) == 0) {
						linphone_chat_message_set_content_type(msg, "application/vnd.gsma.rcs-ft-http+xml");
					} else {
						linphone_chat_message_set_content_type(msg, "text/plain");
					}
				}
			}

			xmlFreeDoc(cacheXml);
		}
	}
	return errcode;
}

int lime_im_encryption_engine_process_outgoing_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	LinphoneCore *lc = linphone_im_encryption_engine_get_core(engine);
	int errcode = -1;
	char *new_content_type = "xml/cipher";
	if(linphone_core_lime_enabled(room->lc)) {
		if (linphone_chat_room_lime_available(room)) {
			if (msg->content_type) {
				if (strcmp(msg->content_type, "application/vnd.gsma.rcs-ft-http+xml") == 0) {
					/* It's a file transfer, content type shall be set to application/cipher.vnd.gsma.rcs-ft-http+xml
					   TODO: As of january 2017, the content type is now included in the encrypted body, this
					   application/cipher.vnd.gsma.rcs-ft-http+xml is kept for compatibility with previous versions,
					   but may be dropped in the future to use xml/cipher instead. */
					new_content_type = "application/cipher.vnd.gsma.rcs-ft-http+xml";
				} else if (strcmp(msg->content_type, "application/im-iscomposing+xml") == 0) {
					/* We don't encrypt composing messages */
					return errcode;
				}
			}

			/* access the zrtp cache to get keys needed to cipher the message */
			const char *zrtp_secrets_cache = linphone_core_get_zrtp_secrets_file(lc);
			FILE *CACHEFD = fopen(zrtp_secrets_cache, "rb+");
			errcode = 0;
			if (CACHEFD == NULL) {
				ms_warning("Unable to access ZRTP ZID cache to encrypt message");
				errcode = 488;
			} else {
				size_t cacheSize;
				char *cacheString;
				xmlDocPtr cacheXml;
				int retval;
				uint8_t *crypted_body = NULL;
				char *peer = linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(room));

				cacheString=ms_load_file_content(CACHEFD, &cacheSize);
				if (!cacheString){
					ms_warning("Unable to load content of ZRTP ZID cache to encrypt message");
					errcode = 500;
					return errcode;
				}
				cacheString[cacheSize] = '\0';
				cacheSize += 1;
				fclose(CACHEFD);
				cacheXml = xmlParseDoc((xmlChar*)cacheString);
				ms_free(cacheString);
				retval = lime_createMultipartMessage(cacheXml, msg->content_type, (uint8_t *)msg->message, (uint8_t *)peer, &crypted_body);
				if (retval != 0) {
					ms_warning("Unable to encrypt message for %s : %s", peer, lime_error_code_to_string(retval));
					if (crypted_body) ms_free(crypted_body);
					errcode = 488;
				} else {
					/* dump updated cache to a string */
					xmlChar *xmlStringOutput;
					int xmlStringLength;
					xmlDocDumpFormatMemoryEnc(cacheXml, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
					/* write it to the cache file */
					CACHEFD = fopen(zrtp_secrets_cache, "wb+");
					if (fwrite(xmlStringOutput, 1, xmlStringLength, CACHEFD)<=0){
						ms_warning("Unable to write zid cache");
					}
					xmlFree(xmlStringOutput);
					fclose(CACHEFD);
					if (msg->message) {
						ms_free(msg->message);
					}
					msg->message = (char *)crypted_body;
					msg->content_type = ms_strdup(new_content_type);
				}
				ms_free(peer);
				xmlFreeDoc(cacheXml);
			}
		} else {
			if (linphone_core_lime_enabled(lc) == LinphoneLimeMandatory) {
				ms_warning("Unable to access ZRTP ZID cache to encrypt message");
				errcode = 488;
			}
		}
	}
	return errcode;
}

int lime_im_encryption_engine_process_downloading_file_cb(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const uint8_t *buffer, size_t size, uint8_t *decrypted_buffer) {
	if (linphone_content_get_key(msg->file_transfer_information) == NULL) return -1;
	
	if (buffer == NULL || size == 0) {
		return lime_decryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information), NULL, 0, NULL, NULL);
	}
	
	return lime_decryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information),
						 (unsigned char *)linphone_content_get_key(msg->file_transfer_information), size, (char *)decrypted_buffer,
						 (char *)buffer);
}

int lime_im_encryption_engine_process_uploading_file_cb(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const uint8_t *buffer, size_t *size, uint8_t *encrypted_buffer) {
	size_t file_size = linphone_content_get_size(msg->file_transfer_information);
	if (linphone_content_get_key(msg->file_transfer_information) == NULL) return -1;
	
	if (buffer == NULL || *size == 0) {
		return lime_encryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information), NULL, 0, NULL, NULL);
	}
	
	if (file_size == 0) {
		ms_warning("File size has not been set, encryption will fail if not done in one step (if file is larger than 16K)");
	} else if (offset + *size < file_size) {
		*size -= (*size % 16);
	}
	
	return lime_encryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information),
					(unsigned char *)linphone_content_get_key(msg->file_transfer_information), *size,
					(char *)buffer, (char *)encrypted_buffer);
}

bool_t lime_im_encryption_engine_is_file_encryption_enabled_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room) {
	LinphoneCore *lc = linphone_im_encryption_engine_get_core(engine);
	return linphone_chat_room_lime_available(room) && linphone_core_lime_for_file_sharing_enabled(lc);
}

void lime_im_encryption_engine_generate_file_transfer_key_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	char keyBuffer [FILE_TRANSFER_KEY_SIZE]; /* temporary storage of generated key: 192 bits of key + 64 bits of initial vector */
	/* generate a random 192 bits key + 64 bits of initial vector and store it into the
		* file_transfer_information->key field of the msg */
	sal_get_random_bytes((unsigned char *)keyBuffer, FILE_TRANSFER_KEY_SIZE);
	linphone_content_set_key(msg->file_transfer_information, keyBuffer, FILE_TRANSFER_KEY_SIZE); /* key is duplicated in the content private structure */
}

#else /* HAVE_LIME */

bool_t lime_is_available() { return FALSE; }
int lime_decryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) { return LIME_NOT_ENABLED;}
int lime_decryptMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t **output, char **content_type, uint64_t validityTimeSpan) { return LIME_NOT_ENABLED;}
int lime_createMultipartMessage(xmlDocPtr cacheBuffer, const char *content_type, uint8_t *message, uint8_t *peerURI, uint8_t **output) { return LIME_NOT_ENABLED;}
int lime_encryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {return LIME_NOT_ENABLED;}
void lime_freeKeys(limeURIKeys_t *associatedKeys){
}
int lime_getCachedSndKeysByURI(xmlDocPtr cacheBuffer, limeURIKeys_t *associatedKeys){
	return LIME_NOT_ENABLED;
}
int lime_encryptMessage(limeKey_t *key, const uint8_t *plainMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *encryptedMessage) {
	return LIME_NOT_ENABLED;
}
int lime_setCachedKey(xmlDocPtr cacheBuffer, limeKey_t *associatedKey, uint8_t role, uint64_t validityTimeSpan) {
	return LIME_NOT_ENABLED;
}
int lime_getCachedRcvKeyByZid(xmlDocPtr cacheBuffer, limeKey_t *associatedKey) {
	return LIME_NOT_ENABLED;
}
int lime_decryptMessage(limeKey_t *key, uint8_t *encryptedMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *plainMessage) {
	return LIME_NOT_ENABLED;
}
bool_t linphone_chat_room_lime_available(LinphoneChatRoom *cr) {
	return FALSE;
}
int lime_im_encryption_engine_process_incoming_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	return 500;
}
int lime_im_encryption_engine_process_outgoing_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	return 500;
}
int lime_im_encryption_engine_process_downloading_file_cb(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const uint8_t *buffer, size_t size, uint8_t *decrypted_buffer) {
	return 500;
}
int lime_im_encryption_engine_process_uploading_file_cb(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const uint8_t *buffer, size_t *size, uint8_t *encrypted_buffer) {
	return 500;
}
bool_t lime_im_encryption_engine_is_file_encryption_enabled_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room) {
	return FALSE;
}
void lime_im_encryption_engine_generate_file_transfer_key_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	
}
#endif /* HAVE_LIME */

char *lime_error_code_to_string(int errorCode) {
	switch (errorCode) {
		case LIME_INVALID_CACHE: return "Invalid ZRTP cache";
		case LIME_UNABLE_TO_DERIVE_KEY: return "Unable to derive Key";
		case LIME_UNABLE_TO_ENCRYPT_MESSAGE: return "Unable to encrypt message";
		case LIME_UNABLE_TO_DECRYPT_MESSAGE: return "Unable to decrypt message";
		case LIME_NO_VALID_KEY_FOUND_FOR_PEER: return "No valid key found";
		case LIME_PEER_KEY_HAS_EXPIRED: return "Any key matching peer Uri has expired";
		case LIME_INVALID_ENCRYPTED_MESSAGE: return "Invalid encrypted message";
		case LIME_NOT_ENABLED: return "Lime not enabled at build";
	}
	return "Unknow error";

}
