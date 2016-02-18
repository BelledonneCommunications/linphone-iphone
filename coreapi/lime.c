#include "lime.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIME
#include "linphonecore.h"
#include "bctoolbox/crypto.h"

/**
 * @brief check at runtime if LIME is available
 *
 * @return TRUE when Lime was fully compiled, FALSE when it wasn't
 */
bool_t lime_is_available() { return TRUE; }

/**
 * @brief	convert an hexa char [0-9a-fA-F] into the corresponding unsigned integer value
 * Any invalid char will be converted to zero without any warning
 *
 * @param[in]	inputChar	a char which shall be in range [0-9a-fA-F]
 *
 * @return		the unsigned integer value in range [0-15]
 */
uint8_t lime_charToByte(uint8_t inputChar) {
	/* 0-9 */
	if (inputChar>0x29 && inputChar<0x3A) {
		return inputChar - 0x30;
	}

	/* a-f */
	if (inputChar>0x60 && inputChar<0x67) {
		return inputChar - 0x57; /* 0x57 = 0x61(a) + 0x0A*/
	}

	/* A-F */
	if (inputChar>0x40 && inputChar<0x47) {
		return inputChar - 0x37; /* 0x37 = 0x41(a) + 0x0A*/
	}

	/* shall never arrive here, string is not Hex*/
	return 0;

}

/**
 * @brief	convert a byte which value is in range [0-15] into an hexa char [0-9a-fA-F]
 *
 * @param[in]	inputByte	an integer which shall be in range [0-15]
 *
 * @return		the hexa char [0-9a-f] corresponding to the input
 */
uint8_t lime_byteToChar(uint8_t inputByte) {
	inputByte &=0x0F; /* restrict the input value to range [0-15] */
	/* 0-9 */
	if(inputByte<0x0A) {
		return inputByte+0x30;
	}
	/* a-f */
	return inputByte + 0x57;
}


/**
 * @brief Convert an hexadecimal string into the corresponding byte buffer
 *
 * @param[out]	outputBytes			The output bytes buffer, must have a length of half the input string buffer
 * @param[in]	inputString			The input string buffer, must be hexadecimal(it is not checked by function, any non hexa char is converted to 0)
 * @param[in]	inputStringLength	The lenght in chars of the string buffer, output is half this length
 */
void lime_strToUint8(uint8_t *outputBytes, uint8_t *inputString, uint16_t inputStringLength) {
	int i;
	for (i=0; i<inputStringLength/2; i++) {
		outputBytes[i] = (lime_charToByte(inputString[2*i]))<<4 | lime_charToByte(inputString[2*i+1]);
	}
}

/**
 * @brief Convert a byte buffer into the corresponding hexadecimal string
 *
 * @param[out]	outputString		The output string buffer, must have a length of twice the input bytes buffer
 * @param[in]	inputBytes			The input bytes buffer
 * @param[in]	inputBytesLength	The lenght in bytes buffer, output is twice this length
 */
void lime_int8ToStr(uint8_t *outputString, uint8_t *inputBytes, uint16_t inputBytesLength) {
	int i;
	for (i=0; i<inputBytesLength; i++) {
		outputString[2*i] = lime_byteToChar((inputBytes[i]>>4)&0x0F);
		outputString[2*i+1] = lime_byteToChar(inputBytes[i]&0x0F);
	}
}



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

			if (matchingURIFlag == 1) { /* we found a match for the URI in this peer node, extract the keys, session Id and index values */
				/* allocate a new limeKey_t structure to hold the retreived keys */
				limeKey_t *currentPeerKeys = (limeKey_t *)malloc(sizeof(limeKey_t));
				uint8_t itemFound = 0; /* count the item found, we must get all of the requested infos: 5 nodes*/
				uint8_t pvs = 0;

				peerNodeChildren = cur->xmlChildrenNode; /* reset peerNodeChildren to the first child of node */
				while (peerNodeChildren!=NULL && itemFound<5) {
					xmlChar *nodeContent = NULL;
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"ZID")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(currentPeerKeys->peerZID, nodeContent, 24);
						itemFound++;
					}

					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndKey")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(currentPeerKeys->key, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndSId")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(currentPeerKeys->sessionId, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"sndIndex")) {
						uint8_t sessionIndexBuffer[4]; /* session index is a uint32_t but we first retrieved it as an hexa string, convert it to a 4 uint8_t buffer */
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(sessionIndexBuffer, nodeContent, 8);
						/* convert it back to a uint32_t (MSByte first)*/
						currentPeerKeys->sessionIndex = sessionIndexBuffer[3] + (sessionIndexBuffer[2]<<8) + (sessionIndexBuffer[1]<<16) + (sessionIndexBuffer[0]<<24);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"pvs")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(&pvs, nodeContent, 2); /* pvs is retrieved as a 2 characters hexa string, convert it to an int8 */
						itemFound++;
					}

					xmlFree(nodeContent);
					peerNodeChildren = peerNodeChildren->next;
				}

				/* check if we have all the requested information and the PVS flag is set to 1 */
				if (itemFound == 5 && pvs == 1) {
					associatedKeys->associatedZIDNumber +=1;
					/* extend array of pointer to limeKey_t structures to add the one we found */
					associatedKeys->peerKeys = (limeKey_t **)realloc(associatedKeys->peerKeys, (associatedKeys->associatedZIDNumber)*sizeof(limeKey_t *));

					/* add the new entry at the end */
					associatedKeys->peerKeys[associatedKeys->associatedZIDNumber-1] = currentPeerKeys;

				} else {
					free(currentPeerKeys);
				}
			}
		}
		cur = cur->next;
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
	lime_int8ToStr(peerZidHex, associatedKey->peerZID, 12);
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
						lime_strToUint8(associatedKey->key, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvSId")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(associatedKey->sessionId, nodeContent, 64);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"rcvIndex")) {
						uint8_t sessionIndexBuffer[4]; /* session index is a uint32_t but we first retrieved it as an hexa string, convert it to a 4 uint8_t buffer */
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(sessionIndexBuffer, nodeContent, 8);
						/* convert it back to a uint32_t (MSByte first)*/
						associatedKey->sessionIndex = sessionIndexBuffer[3] + (sessionIndexBuffer[2]<<8) + (sessionIndexBuffer[1]<<16) + (sessionIndexBuffer[0]<<24);
						itemFound++;
					}
					if (!xmlStrcmp(peerNodeChildren->name, (const xmlChar *)"pvs")) {
						nodeContent = xmlNodeListGetString(cacheBuffer, peerNodeChildren->xmlChildrenNode, 1);
						lime_strToUint8(&pvs, nodeContent, 2); /* pvs is retrieved as a 2 characters hexa string, convert it to an int8 */
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

int lime_setCachedKey(xmlDocPtr cacheBuffer, limeKey_t *associatedKey, uint8_t role) {
	xmlNodePtr cur;
	uint8_t peerZidHex[25];
	uint8_t keyHex[65]; /* key is 32 bytes long -> 64 bytes string + null termination */
	uint8_t sessionIdHex[65]; /* sessionId is 32 bytes long -> 64 bytes string + null termination */
	uint8_t sessionIndexHex[9]; /*  sessionInedx is an uint32_t : 4 bytes long -> 8 bytes string + null termination */
	uint8_t itemFound = 0;

	if (cacheBuffer == NULL ) { /* there is no cache return error */
		return LIME_INVALID_CACHE;
	}

	/* get the given ZID into hex format */
	lime_int8ToStr(peerZidHex, associatedKey->peerZID, 12);
	peerZidHex[24]='\0'; /* must be a null terminated string */

	cur = xmlDocGetRootElement(cacheBuffer);
	/* if we found a root element, parse its children node */
	if (cur!=NULL)
	{
		cur = cur->xmlChildrenNode;

	}

	/* convert the given tag content to null terminated Hexadecimal strings */
	lime_int8ToStr(keyHex, associatedKey->key, 32);
	keyHex[64] = '\0';
	lime_int8ToStr(sessionIdHex, associatedKey->sessionId, 32);
	sessionIdHex[64] = '\0';
	sessionIndexHex[0] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>28)&0x0F));
	sessionIndexHex[1] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>24)&0x0F));
	sessionIndexHex[2] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>20)&0x0F));
	sessionIndexHex[3] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>16)&0x0F));
	sessionIndexHex[4] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>12)&0x0F));
	sessionIndexHex[5] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>8)&0x0F));
	sessionIndexHex[6] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex>>4)&0x0F));
	sessionIndexHex[7] = lime_byteToChar((uint8_t)((associatedKey->sessionIndex)&0x0F));
	sessionIndexHex[8] = '\0';

	while (cur!=NULL && itemFound<3) { /* loop on all peer nodes */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"peer"))){ /* found a peer, check his ZID element */
			xmlChar *currentZidHex = xmlNodeListGetString(cacheBuffer, cur->xmlChildrenNode->xmlChildrenNode, 1); /* ZID is the first element of peer */
			if (!xmlStrcmp(currentZidHex, (const xmlChar *)peerZidHex)) { /* we found the peer element we are looking for */
				xmlNodePtr peerNodeChildren = cur->xmlChildrenNode->next;
				while (peerNodeChildren != NULL && itemFound<3) { /* look for the tag we want to write */
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
	bctoolbox_hmacSha256(key->key, 32, inputData, 55, 32, derivedKey);

	/* overwrite the old key with the derived one */
	memcpy(key->key, derivedKey, 32);

	/* increment the session Index */
	key->sessionIndex += 1;
	return 0;
}

void lime_freeKeys(limeURIKeys_t associatedKeys) {
	int i;

	/* free all associated keys */
	for (i=0; i< associatedKeys.associatedZIDNumber; i++) {
		if (associatedKeys.peerKeys[i] != NULL) {
			free(associatedKeys.peerKeys[i]);
			associatedKeys.peerKeys[i] = NULL;
		}
	}

	free(associatedKeys.peerKeys);

	/* free sipURI string */
	free(associatedKeys.peerURI);
}

int lime_encryptMessage(limeKey_t *key, uint8_t *plainMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *encryptedMessage) {
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
	return bctoolbox_aes_gcm_encrypt_and_tag(key->key, 24,
			plainMessage, messageLength,
			authenticatedData, 28,
			key->key+24, 8, /* IV is at the end(last 64 bits) of the given key buffer */
			encryptedMessage, 16, /* the first 16 bytes of output are the authentication tag */
			encryptedMessage+16); /* actual encrypted message starts after 16 bytes of authentication tag */

	return 0;
}

int lime_encryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {
	bctoolbox_aes_gcm_context_t *gcmContext;

	if (*cryptoContext == NULL) { /* first call to the function, allocate a crypto context and initialise it */
		/* key contains 192bits of key || 64 bits of Initialisation Vector, no additional data */
		gcmContext = bctoolbox_aes_gcm_context_new(key, 24, NULL, 0, key+24, 8, BCTOOLBOX_GCM_ENCRYPT);
		*cryptoContext = gcmContext;
	} else { /* this is not the first call, get the context */
		gcmContext = (bctoolbox_aes_gcm_context_t *)*cryptoContext;
	}

	if (length != 0) {
		bctoolbox_aes_gcm_process_chunk(gcmContext, (const uint8_t *)plain, length, (uint8_t *)cipher);
	} else { /* lenght is 0, finish the stream, no tag to be generated */
		bctoolbox_aes_gcm_finish(gcmContext, NULL, 0);
		*cryptoContext = NULL;
	}

	return 0;
}

int lime_decryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {
	bctoolbox_aes_gcm_context_t *gcmContext;

	if (*cryptoContext == NULL) { /* first call to the function, allocate a crypto context and initialise it */
		/* key contains 192bits of key || 64 bits of Initialisation Vector, no additional data */
		gcmContext = bctoolbox_aes_gcm_context_new(key, 24, NULL, 0, key+24, 8, BCTOOLBOX_GCM_DECRYPT);
		*cryptoContext = gcmContext;
	} else { /* this is not the first call, get the context */
		gcmContext = (bctoolbox_aes_gcm_context_t *)*cryptoContext;
	}

	if (length != 0) {
		bctoolbox_aes_gcm_process_chunk(gcmContext, (const unsigned char *)cipher, length, (unsigned char *)plain);
	} else { /* lenght is 0, finish the stream */
		bctoolbox_aes_gcm_finish(gcmContext, NULL, 0);
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
	retval = bctoolbox_aes_gcm_decrypt_and_auth(key->key, 24, /* key is 192 bits long */
			encryptedMessage+16,  messageLength-16, /* encrypted message first 16 bytes store the authentication tag, then is the actual message */
			authenticatedData, 28, /* additionnal data needed for authentication */
			key->key+24, 8, /* last 8 bytes of key is the initialisation vector */
			encryptedMessage, 16, /* first 16 bytes of message is the authentication tag */
			plainMessage);

	/* add the null termination char */
	plainMessage[messageLength-16] = '\0';

	return retval;
}

int lime_createMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t *peerURI, uint8_t **output) {
	uint8_t selfZidHex[25];
	uint8_t selfZid[12]; /* same data but in byte buffer */
	uint32_t encryptedMessageLength;
	limeURIKeys_t associatedKeys;
	xmlDocPtr xmlOutputMessage;
	xmlNodePtr rootNode;
	int i;
	int xmlStringLength;

	/* retrieve selfZIDHex from cache(return a 24 char hexa string + null termination) */
	if (lime_getSelfZid(cacheBuffer, selfZidHex) != 0) {
		return LIME_UNABLE_TO_ENCRYPT_MESSAGE;
	}
	lime_strToUint8(selfZid, selfZidHex, 24);

	/* encrypted message length is plaintext + 16 for tag */
	encryptedMessageLength = strlen((char *)message) + 16;

	/* retrieve keys associated to the peer URI */
	associatedKeys.peerURI = (uint8_t *)malloc(strlen((char *)peerURI)+1);
	strcpy((char *)(associatedKeys.peerURI), (char *)peerURI);
	associatedKeys.associatedZIDNumber  = 0;
	associatedKeys.peerKeys = NULL;

	if (lime_getCachedSndKeysByURI(cacheBuffer, &associatedKeys) != 0) {
		lime_freeKeys(associatedKeys);
		return LIME_UNABLE_TO_ENCRYPT_MESSAGE;
	}

	if (associatedKeys.associatedZIDNumber == 0) {
		lime_freeKeys(associatedKeys);
		return LIME_NO_VALID_KEY_FOUND_FOR_PEER;
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

		/* encrypt message with current key */
		limeKey_t *currentKey = associatedKeys.peerKeys[i];
		/* encrypted message include a 16 bytes tag */
		uint8_t *encryptedMessage = (uint8_t *)malloc(encryptedMessageLength);
		lime_encryptMessage(currentKey, message, strlen((char *)message), selfZid, encryptedMessage);
		/* add a "msg" node the the output message, doc node is :
		 * <msg>
		 * 		<pzid>peerZID</pzid>
		 * 		<index>session index</index>
		 * 		<text>ciphertext</text>
		 * </msg> */
		msgNode = xmlNewDocNode(xmlOutputMessage, NULL, (const xmlChar *)"msg", NULL);
		lime_int8ToStr(peerZidHex, currentKey->peerZID, 12);
		peerZidHex[24] = '\0';
		sessionIndexHex[0] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>28)&0x0F));
		sessionIndexHex[1] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>24)&0x0F));
		sessionIndexHex[2] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>20)&0x0F));
		sessionIndexHex[3] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>16)&0x0F));
		sessionIndexHex[4] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>12)&0x0F));
		sessionIndexHex[5] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>8)&0x0F));
		sessionIndexHex[6] = lime_byteToChar((uint8_t)((currentKey->sessionIndex>>4)&0x0F));
		sessionIndexHex[7] = lime_byteToChar((uint8_t)((currentKey->sessionIndex)&0x0F));
		sessionIndexHex[8] = '\0';

		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"pzid", peerZidHex);
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"index", sessionIndexHex);

		/* convert the cipherText to base 64 */
		bctoolbox_base64_encode(NULL, &b64Size, encryptedMessage, encryptedMessageLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedMessageb64 = malloc(b64Size+1); /* allocate a buffer of requested size +1 for NULL termination */
		bctoolbox_base64_encode(encryptedMessageb64, &b64Size, encryptedMessage, encryptedMessageLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedMessageb64[b64Size] = '\0'; /* libxml need a null terminated string */
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"text", (const xmlChar *)encryptedMessageb64);
		free(encryptedMessage);
		free(encryptedMessageb64);

		/* add the message Node into the doc */
		xmlAddChild(rootNode, msgNode);

		/* update the key used */
		lime_deriveKey(currentKey);
		lime_setCachedKey(cacheBuffer, currentKey, LIME_SENDER);
	}

	/* dump the whole message doc into the output */
	xmlDocDumpFormatMemoryEnc(xmlOutputMessage, output, &xmlStringLength, "UTF-8", 0);
	xmlFreeDoc(xmlOutputMessage);

	lime_freeKeys(associatedKeys);

	return 0;
}

int lime_decryptMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t **output) {
	int retval;
	uint8_t selfZidHex[25];
	uint8_t selfZid[12]; /* same data but in byte buffer */
	limeKey_t associatedKey;
	xmlChar *peerZidHex = NULL;
	xmlNodePtr cur;
	uint8_t *encryptedMessage = NULL;
	size_t encryptedMessageLength = 0;
	uint32_t usedSessionIndex = 0;
	xmlDocPtr xmlEncryptedMessage;

	if (cacheBuffer == NULL) {
		return LIME_INVALID_CACHE;
	}
	/* retrieve selfZIDHex from cache(return a 24 char hexa string + null termination) */
	if (lime_getSelfZid(cacheBuffer, selfZidHex) != 0) {
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}
	lime_strToUint8(selfZid, selfZidHex, 24);

	/* parse the message into an xml doc */
	/* make sure we have a valid xml message before trying to parse it */
	if (memcmp(message, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>", 38) != 0 ) {
		return LIME_INVALID_ENCRYPTED_MESSAGE;
	}
	xmlEncryptedMessage = xmlParseDoc((const xmlChar *)message);
	if (xmlEncryptedMessage == NULL) {
		return LIME_INVALID_ENCRYPTED_MESSAGE;
	}

	/* retrieve the sender ZID which is the first child of root */
	cur = xmlDocGetRootElement(xmlEncryptedMessage);
	if (cur != NULL) {
		cur = cur->xmlChildrenNode;
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"ZID"))){ /* sender ZID found, extract it */
			peerZidHex = xmlNodeListGetString(xmlEncryptedMessage, cur->xmlChildrenNode, 1);
			/* convert it from hexa string to bytes string and set the result in the associatedKey structure */
			lime_strToUint8(associatedKey.peerZID, peerZidHex, strlen((char *)peerZidHex));
			cur = cur->next;
		}
	}

	if (peerZidHex != NULL) {
		/* get from cache the matching key */
		retval = lime_getCachedRcvKeyByZid(cacheBuffer, &associatedKey);

		if (retval != 0) {
			xmlFree(peerZidHex);
			xmlFreeDoc(xmlEncryptedMessage);
			return retval;
		}

		/* retrieve the portion of message which is encrypted with our key */
		while (cur != NULL) { /* loop on all "msg" node in the message */
			xmlNodePtr msgChildrenNode = cur->xmlChildrenNode;
			xmlChar *currentZidHex = xmlNodeListGetString(cacheBuffer, msgChildrenNode->xmlChildrenNode, 1); /* pZID is the first element of msg */
			if (!xmlStrcmp(currentZidHex, (const xmlChar *)selfZidHex)) { /* we found the msg node we are looking for */
				/* get the index (second node in the msg one) */
				xmlChar *sessionIndexHex;
				xmlChar *encryptedMessageb64;

				msgChildrenNode = msgChildrenNode->next;
				sessionIndexHex = xmlNodeListGetString(cacheBuffer, msgChildrenNode->xmlChildrenNode, 1);
				usedSessionIndex = (((uint32_t)lime_charToByte(sessionIndexHex[0]))<<28)
					| (((uint32_t)lime_charToByte(sessionIndexHex[1]))<<24)
					| (((uint32_t)lime_charToByte(sessionIndexHex[2]))<<20)
					| (((uint32_t)lime_charToByte(sessionIndexHex[3]))<<16)
					| (((uint32_t)lime_charToByte(sessionIndexHex[4]))<<12)
					| (((uint32_t)lime_charToByte(sessionIndexHex[5]))<<8)
					| (((uint32_t)lime_charToByte(sessionIndexHex[6]))<<4)
					| (((uint32_t)lime_charToByte(sessionIndexHex[7])));
				xmlFree(sessionIndexHex);

				/* get the encrypted message */
				msgChildrenNode = msgChildrenNode->next;

				/* convert the cipherText from base 64 */
				encryptedMessageb64 = xmlNodeListGetString(cacheBuffer, msgChildrenNode->xmlChildrenNode, 1);
				bctoolbox_base64_decode(NULL, &encryptedMessageLength, encryptedMessageb64, strlen((char *)encryptedMessageb64)); /* encryptedMessageLength is 0, so it will be set to the requested buffer length */
				encryptedMessage = (uint8_t *)malloc(encryptedMessageLength);
				bctoolbox_base64_decode(encryptedMessage, &encryptedMessageLength, encryptedMessageb64, strlen((char *)encryptedMessageb64));

				xmlFree(encryptedMessageb64);
				xmlFree(currentZidHex);
				break;
			}

			cur = cur->next;
			xmlFree(currentZidHex);
			}
	}

	xmlFree(peerZidHex);
	xmlFreeDoc(xmlEncryptedMessage);

	/* do we have retrieved correctly all the needed data */
	if (encryptedMessage == NULL) {
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	/* shall we derive our key before going for decryption */
	if (usedSessionIndex < associatedKey.sessionIndex) {
		/* something wen't wrong with the cache, this shall never happend */
		free(encryptedMessage);
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	if ((usedSessionIndex - associatedKey.sessionIndex > MAX_DERIVATION_NUMBER) ) {
		/* we missed to many messages, ask for a cache reset via a ZRTP call */
		free(encryptedMessage);
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	while (usedSessionIndex>associatedKey.sessionIndex) {
		lime_deriveKey(&associatedKey);
	}

	/* decrypt the message */
	*output = (uint8_t *)malloc(encryptedMessageLength - 16 +1); /* plain message is same length than encrypted one with 16 bytes less for the tag + 1 to add the null termination char */
	retval = lime_decryptMessage(&associatedKey, encryptedMessage, encryptedMessageLength, selfZid, *output);

	free(encryptedMessage);

	if (retval!=0 ) {
		free(*output);
		*output = NULL;
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}

	/* update used key */
	lime_deriveKey(&associatedKey);
	lime_setCachedKey(cacheBuffer, &associatedKey, LIME_RECEIVER);

	return 0;
}


#else /* HAVE_LIME */

bool_t lime_is_available() { return FALSE; }
int lime_decryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) { return LIME_NOT_ENABLED;}
int lime_decryptMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t **output) { return LIME_NOT_ENABLED;}
int lime_createMultipartMessage(xmlDocPtr cacheBuffer, uint8_t *message, uint8_t *peerURI, uint8_t **output) { return LIME_NOT_ENABLED;}
int lime_encryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {return LIME_NOT_ENABLED;}


#endif /* HAVE_LIME */

char *lime_error_code_to_string(int errorCode) {
	switch (errorCode) {
		case LIME_INVALID_CACHE: return "Invalid ZRTP cache";
		case LIME_UNABLE_TO_DERIVE_KEY: return "Unable to derive Key";
		case LIME_UNABLE_TO_ENCRYPT_MESSAGE: return "Unable to encrypt message";
		case LIME_UNABLE_TO_DECRYPT_MESSAGE: return "Unable to decrypt message";
		case LIME_NO_VALID_KEY_FOUND_FOR_PEER: return "No valid key found";
		case LIME_INVALID_ENCRYPTED_MESSAGE: return "Invalid encrypted message";
		case LIME_NOT_ENABLED: return "Lime not enabled at build";
	}
	return "Unknow error";

}
