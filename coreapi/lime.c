/*
linphone
Copyright (C) 2017  Belledonne Communications SARL

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
#include "bzrtp/bzrtp.h"

#define FILE_TRANSFER_KEY_SIZE 32

/**
 * @brief check at runtime if LIME is available
 *
 * @return TRUE when Lime was fully compiled, FALSE when it wasn't
 */
bool_t lime_is_available(void) { return TRUE; }

int lime_getCachedSndKeysByURI(void *cachedb, limeURIKeys_t *associatedKeys) {
	sqlite3 *db = (sqlite3 *)cachedb;
	size_t keysFound = 0; /* used to detect the no key found error because of validity expired */
	char *stmt = NULL;
	int ret;
	sqlite3_stmt *sqlStmt = NULL;
	int length =0;
	uint8_t pvsOne[1] = {0x01}; /* used to bind this specific byte value to a blob WHERE constraint in the query */

	if (cachedb == NULL ) { /* there is no cache return error */
		return LIME_INVALID_CACHE;
	}

	/* reset number of associated keys and their buffer */
	associatedKeys->associatedZIDNumber = 0;
	associatedKeys->peerKeys = NULL;

	/* query the DB: join ziduri, lime and zrtp tables : retrieve zuid(for easier key update in cache), peerZID, sndKey, sndSId, sndIndex, valid where self and peer ZIDs are matching constraint and pvs is raised */
	/* Note: retrieved potentially expired keys, just to be able to send a different status to caller(no keys found is not expired key found) */
	/* if we do not have self uri in associatedKeys, just retrieve any available key matching peer URI */
	if (associatedKeys->selfURI == NULL) {
		stmt = sqlite3_mprintf("SELECT zu.zuid, zu.zid as peerZID, l.sndkey, l.sndSId, l.sndIndex, l.valid FROM ziduri as zu INNER JOIN zrtp as z ON z.zuid=zu.zuid INNER JOIN lime as l ON z.zuid=l.zuid WHERE zu.peeruri=? AND z.pvs=?;");
		ret = sqlite3_prepare_v2(db, stmt, -1, &sqlStmt, NULL);
		sqlite3_free(stmt);
		if (ret != SQLITE_OK) {
			return LIME_INVALID_CACHE;
		}
		sqlite3_bind_text(sqlStmt, 1, associatedKeys->peerURI,-1, SQLITE_TRANSIENT);
		sqlite3_bind_blob(sqlStmt, 2, pvsOne, 1, SQLITE_TRANSIENT);
	} else { /* we have a self URI, so include it in the query */
		stmt = sqlite3_mprintf("SELECT zu.zuid, zu.zid as peerZID, l.sndkey, l.sndSId, l.sndIndex, l.valid FROM ziduri as zu INNER JOIN zrtp as z ON z.zuid=zu.zuid INNER JOIN lime as l ON z.zuid=l.zuid WHERE zu.selfuri=? AND zu.peeruri=? AND z.pvs=?;");
		ret = sqlite3_prepare_v2(db, stmt, -1, &sqlStmt, NULL);
		sqlite3_free(stmt);
		if (ret != SQLITE_OK) {
			return LIME_INVALID_CACHE;
		}
		sqlite3_bind_text(sqlStmt, 1, associatedKeys->selfURI,-1, SQLITE_TRANSIENT);
		sqlite3_bind_text(sqlStmt, 2, associatedKeys->peerURI,-1, SQLITE_TRANSIENT);
		sqlite3_bind_blob(sqlStmt, 3, pvsOne, 1, SQLITE_TRANSIENT);
	}

	/* parse all retrieved rows */
	while ((ret = sqlite3_step(sqlStmt)) == SQLITE_ROW) {
		/* allocate a new limeKey_t structure to hold the retreived keys */
		limeKey_t *currentPeerKey = (limeKey_t *)bctbx_malloc0(sizeof(limeKey_t));
		bctoolboxTimeSpec currentTimeSpec;
		bctoolboxTimeSpec validityTimeSpec;
		validityTimeSpec.tv_sec=0;
		validityTimeSpec.tv_nsec=0;

		/* get zuid from column 0 */
		currentPeerKey->zuid = sqlite3_column_int(sqlStmt, 0);

		/* retrieve values : peerZid, sndKey, sndSId, sndIndex, valid from columns 1,2,3,4,5 */
		length = sqlite3_column_bytes(sqlStmt, 1);
		if (length==12) { /* peerZID */
			memcpy(currentPeerKey->peerZID, sqlite3_column_blob(sqlStmt, 1), length);
		} else { /* something wrong with that one, skip it */
			continue;
		}

		length = sqlite3_column_bytes(sqlStmt, 2);
		if (length==32) { /* sndKey */
			memcpy(currentPeerKey->key, sqlite3_column_blob(sqlStmt, 2), length);
		} else { /* something wrong with that one, skip it */
			continue;
		}

		length = sqlite3_column_bytes(sqlStmt, 3);
		if (length==32) { /* sndSId */
			memcpy(currentPeerKey->sessionId, sqlite3_column_blob(sqlStmt, 3), length);
		} else { /* something wrong with that one, skip it */
			continue;
		}

		length = sqlite3_column_bytes(sqlStmt, 4);
		if (length==4) { /* sndIndex : 4 bytes of a uint32_t, stored as a blob in big endian */
			uint8_t *sessionId = (uint8_t *)sqlite3_column_blob(sqlStmt, 4);
			currentPeerKey->sessionIndex = ((uint32_t)(sessionId[0]))<<24 |
							((uint32_t)(sessionId[1]))<<16 |
							((uint32_t)(sessionId[2]))<<8 |
							((uint32_t)(sessionId[3]));
		} else { /* something wrong with that one, skip it */
			continue;
		}

		length = sqlite3_column_bytes(sqlStmt, 5);
		if (length==8) { /* sndIndex : 8 bytes of a int64_t, stored as a blob in big endian */
			uint8_t *validity = (uint8_t *)sqlite3_column_blob(sqlStmt, 5);
			validityTimeSpec.tv_sec = ((uint64_t)(validity[0]))<<56 |
							((uint64_t)(validity[1]))<<48 |
							((uint64_t)(validity[2]))<<40 |
							((uint64_t)(validity[3]))<<32 |
							((uint64_t)(validity[4]))<<24 |
							((uint64_t)(validity[5]))<<16 |
							((uint64_t)(validity[6]))<<8 |
							((uint64_t)(validity[7]));
		} else { /* something wrong with that one, skip it */
			continue;
		}

		/* count is a found even if it may be expired */
		keysFound++;

		/* check validity */
		bctbx_get_utc_cur_time(&currentTimeSpec);
		if (validityTimeSpec.tv_sec == 0 || bctbx_timespec_compare(&currentTimeSpec, &validityTimeSpec)<0) {
			associatedKeys->associatedZIDNumber +=1;
			/* extend array of pointer to limeKey_t structures to add the one we found */
			associatedKeys->peerKeys = (limeKey_t **)bctbx_realloc(associatedKeys->peerKeys, (associatedKeys->associatedZIDNumber)*sizeof(limeKey_t *));

			/* add the new entry at the end */
			associatedKeys->peerKeys[associatedKeys->associatedZIDNumber-1] = currentPeerKey;
		} else {
			free(currentPeerKey);
		}
	}

	sqlite3_finalize(sqlStmt);

	/* something is wrong with the cache? */
	if (ret!=SQLITE_DONE) {
		return LIME_INVALID_CACHE;
	}

	/* we're done, check what we have */
	if (associatedKeys->associatedZIDNumber == 0) {
		if (keysFound == 0) {
			return LIME_NO_VALID_KEY_FOUND_FOR_PEER;
		} else {
			return LIME_PEER_KEY_HAS_EXPIRED;
		}
	}
	return 0;

}

int lime_getCachedRcvKeyByZid(void *cachedb, limeKey_t *associatedKey, const char *selfURI, const char *peerURI) {
	sqlite3 *db = (sqlite3 *)cachedb;
	char *stmt = NULL;
	int ret;
	sqlite3_stmt *sqlStmt = NULL;
	int length =0;
	uint8_t pvsOne[1] = {0x01}; /* used to bind this specific byte value to a blob WHERE constraint in the query */


	if (db == NULL) { /* there is no cache return error */
		ms_error("[LIME] Get Cached Rcv Key by Zid : no cache found");
		return LIME_INVALID_CACHE;
	}

	/* query the DB: join ziduri, lime and zrtp tables : */
	/* retrieve zuid(for easier key update in cache), rcvKey, rcvSId, rcvIndex where self/peer uris and peer zid are matching constraint(unique row) and pvs is raised */
	stmt = sqlite3_mprintf("SELECT zu.zuid, l.rcvkey, l.rcvSId, l.rcvIndex FROM ziduri as zu INNER JOIN zrtp as z ON z.zuid=zu.zuid INNER JOIN lime as l ON z.zuid=l.zuid WHERE zu.selfuri=? AND zu.peeruri=? AND zu.zid=? AND z.pvs=? LIMIT 1;");
	ret = sqlite3_prepare_v2(db, stmt, -1, &sqlStmt, NULL);
	sqlite3_free(stmt);
	if (ret != SQLITE_OK) {
		ms_error("[LIME] Get Cached Rcv Key by Zid can't prepare statement to retrieve key");
		return LIME_INVALID_CACHE;
	}
	sqlite3_bind_text(sqlStmt, 1, selfURI,-1, SQLITE_TRANSIENT);
	sqlite3_bind_text(sqlStmt, 2, peerURI,-1, SQLITE_TRANSIENT);
	sqlite3_bind_blob(sqlStmt, 3, associatedKey->peerZID, 12, SQLITE_TRANSIENT);
	sqlite3_bind_blob(sqlStmt, 4, pvsOne, 1, SQLITE_TRANSIENT);


	if ((ret = sqlite3_step(sqlStmt)) == SQLITE_ROW) { /* we found a row */
		/* get zuid from column 0 */
		associatedKey->zuid = sqlite3_column_int(sqlStmt, 0);

		/* retrieve values : rcvKey, rcvSId, rcvIndex from columns 1,2,3 */
		length = sqlite3_column_bytes(sqlStmt, 1);
		if (length==32) { /* rcvKey */
			memcpy(associatedKey->key, sqlite3_column_blob(sqlStmt, 1), length);
		} else { /* something wrong */
			ms_error("[LIME] Get Cached Rcv Key by Zid fetched a rcvKey with wrong length");
			sqlite3_finalize(sqlStmt);
			return LIME_INVALID_CACHE;
		}

		length = sqlite3_column_bytes(sqlStmt, 2);
		if (length==32) { /* rcvSId */
			memcpy(associatedKey->sessionId, sqlite3_column_blob(sqlStmt, 2), length);
		} else { /* something wrong */
			ms_error("[LIME] Get Cached Rcv Key by Zid fetched a rcvSid with wrong length");
			sqlite3_finalize(sqlStmt);
			return LIME_INVALID_CACHE;
		}

		length = sqlite3_column_bytes(sqlStmt, 3);
		if (length==4) { /* rcvIndex */
			uint8_t *sessionId = (uint8_t *)sqlite3_column_blob(sqlStmt, 3);
			associatedKey->sessionIndex = ((uint32_t)(sessionId[0]))<<24 |
							((uint32_t)(sessionId[1]))<<16 |
							((uint32_t)(sessionId[2]))<<8 |
							((uint32_t)(sessionId[3]));
		} else { /* something wrong */
			sqlite3_finalize(sqlStmt);
			ms_error("[LIME] Get Cached Rcv Key by Zid fetched a rcvIndex with wrong length");
			return LIME_INVALID_CACHE;
		}

		sqlite3_finalize(sqlStmt);
		return 0;
	}

	/* something is wrong with the cache? */
	if (ret!=SQLITE_DONE) {
		ms_error("[LIME] Get Cached Rcv Key by Zid : request gone bad");
		return LIME_INVALID_CACHE;
	}

	/* reach here if the query executed correctly but returned no result */
	return LIME_NO_VALID_KEY_FOUND_FOR_PEER;
}

int lime_setCachedKey(void *cachedb, limeKey_t *associatedKey, uint8_t role, uint64_t validityTimeSpan) {
	bctoolboxTimeSpec currentTime;
	/* columns to be written in cache */
	const char *colNamesSender[] = {"sndKey", "sndSId", "sndIndex"}; /* Sender never update the validity period */
	const char *colNamesReceiver[] = {"rcvKey", "rcvSId", "rcvIndex", "valid"};
	uint8_t *colValues[4];
	uint8_t sessionIndex[4]; /* buffer to hold the uint32_t buffer index in big endian */
	size_t colLength[] = {32, 32, 4, 8}; /* data length: keys and session ID : 32 bytes, Index: 4 bytes(uint32_t), validity : 8 bytes(UTC time as int64_t) */
	int colNums;

	if (cachedb == NULL  || associatedKey == NULL) { /* there is no cache return error */
		return LIME_INVALID_CACHE;
	}

	/* wrap values to be written */
	sessionIndex[0] = (associatedKey->sessionIndex>>24)&0xFF;
	sessionIndex[1] = (associatedKey->sessionIndex>>16)&0xFF;
	sessionIndex[2] = (associatedKey->sessionIndex>>8)&0xFF;
	sessionIndex[3] = (associatedKey->sessionIndex)&0xFF;
	colValues[0] = associatedKey->key;
	colValues[1] = associatedKey->sessionId;
	colValues[2] = sessionIndex;

	/* shall we update valid column? Enforce only when receiver, if timeSpan is 0, just ignore */
	if (validityTimeSpan > 0 && role == LIME_RECEIVER) {
		bctbx_get_utc_cur_time(&currentTime);
		bctbx_timespec_add(&currentTime, validityTimeSpan);
		/* store the int64_t in big endian in the cache(cache is not typed, all data seen as blob) */
		colValues[3][0] = (currentTime.tv_sec>>56)&0xFF;
		colValues[3][1] = (currentTime.tv_sec>>48)&0xFF;
		colValues[3][2] = (currentTime.tv_sec>>40)&0xFF;
		colValues[3][3] = (currentTime.tv_sec>>32)&0xFF;
		colValues[3][4] = (currentTime.tv_sec>>24)&0xFF;
		colValues[3][5] = (currentTime.tv_sec>>16)&0xFF;
		colValues[3][6] = (currentTime.tv_sec>>8)&0xFF;
		colValues[3][7] = (currentTime.tv_sec)&0xFF;

		colNums = 4;
	} else {
		colNums = 3; /* do not write the valid column*/
	}

	/* update cache */
	return bzrtp_cache_write(cachedb, associatedKey->zuid, "lime", role==LIME_SENDER?colNamesSender:colNamesReceiver, colValues, colLength, colNums);
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

	bctbx_free(associatedKeys->peerKeys);
	associatedKeys->peerKeys = NULL;

	/* free sipURI strings */
	bctbx_free(associatedKeys->selfURI);
	associatedKeys->selfURI = NULL;
	bctbx_free(associatedKeys->peerURI);
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

int lime_createMultipartMessage(void *cachedb, const char *contentType, uint8_t *message, const char *selfURI, const char *peerURI, uint8_t **output) {
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

	/* retrieve selfZIDHex from cache */
	if (bzrtp_getSelfZID(cachedb, selfURI, selfZid, NULL) != 0) {
		return LIME_UNABLE_TO_ENCRYPT_MESSAGE;
	}
	if (message == NULL || contentType == NULL) {
		return LIME_UNABLE_TO_ENCRYPT_MESSAGE;
	}

	/* encrypted message length is plaintext + 16 for tag */
	encryptedMessageLength = (uint32_t)strlen((char *)message) + 16;
	encryptedContentTypeLength = (uint32_t)strlen((char *)contentType) + 16;

	/* retrieve keys associated to the peer URI */
	associatedKeys.peerURI = bctbx_strdup(peerURI);
	associatedKeys.selfURI = bctbx_strdup(selfURI);
	associatedKeys.associatedZIDNumber  = 0;
	associatedKeys.peerKeys = NULL;

	if ((ret = lime_getCachedSndKeysByURI(cachedb, &associatedKeys)) != 0) {
		lime_freeKeys(&associatedKeys);
		return ret;
	}

	/* create an xml doc to hold the multipart message */
	xmlOutputMessage = xmlNewDoc((const xmlChar *)"1.0");
	/* root tag is "doc" */
	rootNode = xmlNewDocNode(xmlOutputMessage, NULL, (const xmlChar *)"doc", NULL);
	xmlDocSetRootElement(xmlOutputMessage, rootNode);
	/* add the self ZID child, convert it to an hexa string  */
	bctbx_int8_to_str(selfZidHex, selfZid, 12);
	selfZidHex[24] = '\0'; /* add a NULL termination for libxml */
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
		bctbx_int8_to_str(peerZidHex, currentKey->peerZID, 12);
		peerZidHex[24] = '\0';
		bctbx_uint32_to_str(sessionIndexHex, currentKey->sessionIndex);

		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"pzid", peerZidHex);
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"index", sessionIndexHex);

		/* convert the cipherText to base 64 */
		bctbx_base64_encode(NULL, &b64Size, encryptedMessage, encryptedMessageLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedMessageb64 = reinterpret_cast<unsigned char *>(ms_malloc(b64Size+1)); /* allocate a buffer of requested size +1 for NULL termination */
		bctbx_base64_encode(encryptedMessageb64, &b64Size, encryptedMessage, encryptedMessageLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedMessageb64[b64Size] = '\0'; /* libxml need a null terminated string */
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"text", (const xmlChar *)encryptedMessageb64);
		ms_free(encryptedMessage);
		ms_free(encryptedMessageb64);

		/* convert the encrypted content-type to base 64 */
		b64Size = 0;
		bctbx_base64_encode(NULL, &b64Size, encryptedContentType, encryptedContentTypeLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedContentTypeb64 = reinterpret_cast<unsigned char *>(ms_malloc(b64Size+1)); /* allocate a buffer of requested size +1 for NULL termination */
		bctbx_base64_encode(encryptedContentTypeb64, &b64Size, encryptedContentType, encryptedContentTypeLength); /* b64Size is 0, so it is set to the requested output buffer size */
		encryptedContentTypeb64[b64Size] = '\0'; /* libxml need a null terminated string */
		xmlNewTextChild(msgNode, NULL, (const xmlChar *)"content-type", (const xmlChar *)encryptedContentTypeb64);
		ms_free(encryptedContentType);
		ms_free(encryptedContentTypeb64);

		/* add the message Node into the doc */
		xmlAddChild(rootNode, msgNode);

		/* update the key used */
		lime_deriveKey(currentKey);
		lime_setCachedKey(cachedb, currentKey, LIME_SENDER, 0); /* never update validity when sending a message */
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

int lime_decryptMultipartMessage(void *cachedb, uint8_t *message, const char *selfURI, const char *peerURI, uint8_t **output, char **content_type, uint64_t validityTimeSpan) {
	int retval = 0;
	uint8_t selfZidHex[25];
	uint8_t selfZid[12]; /* same data but in byte buffer */
	char xpath_str[MAX_XPATH_LENGTH];
	limeKey_t associatedKey;
	char *peerZidHex = NULL;
	char *sessionIndexHex = NULL;
	xmlparsing_context_t *xml_ctx;
	xmlXPathObjectPtr msg_object;
	uint8_t *encryptedMessage = NULL;
	size_t encryptedMessageLength = 0;
	uint8_t *encryptedContentType = NULL;
	size_t encryptedContentTypeLength = 0;
	uint32_t usedSessionIndex = 0;
	int i;

	if (cachedb == NULL) {
		return LIME_INVALID_CACHE;
	}

	/* retrieve selfZID from cache, and convert it to an Hexa buffer to easily match it against hex string containg in xml message as pzid */
	if (bzrtp_getSelfZID(cachedb, selfURI, selfZid, NULL) != 0) {
		ms_error("[LIME] Couldn't get self ZID"); 
		return LIME_UNABLE_TO_DECRYPT_MESSAGE;
	}
	bctbx_int8_to_str(selfZidHex, selfZid, 12);
	selfZidHex[24]='\0';

	xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)message, 0, NULL, 0);
	if (xml_ctx->doc == NULL) {
		ms_error("[LIME] XML doc is null"); 
		retval = LIME_INVALID_ENCRYPTED_MESSAGE;
		goto error;
	}

	if (linphone_create_xml_xpath_context(xml_ctx) < 0) {
		ms_error("[LIME] Couldn't create xml xpath context"); 
		retval = LIME_INVALID_ENCRYPTED_MESSAGE;
		goto error;
	}

	/* Retrieve the sender ZID */
	peerZidHex = linphone_get_xml_text_content(xml_ctx, "/doc/ZID");
	if (peerZidHex != NULL) {
		/* Convert it from hexa string to bytes string and set the result in the associatedKey structure */
		bctbx_str_to_uint8(associatedKey.peerZID, (const uint8_t *)peerZidHex, (uint16_t)strlen(peerZidHex));

		/* Get the matching key from cache */
		retval = lime_getCachedRcvKeyByZid(cachedb, &associatedKey, selfURI, peerURI);
		if (retval != 0) {
			ms_error("[LIME] Couldn't get cache rcv key by ZID. Returns %04x. PeerZid %s peerURI %s selfURI %s", retval, peerZidHex, peerURI, selfURI);
			linphone_free_xml_text_content(peerZidHex);
			goto error;
		}
		linphone_free_xml_text_content(peerZidHex);

		/* Retrieve the portion of message which is encrypted with our key(seek for a pzid matching our) */
		msg_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/doc/msg");
		if ((msg_object != NULL) && (msg_object->nodesetval != NULL)) {
			for (i = 1; i <= msg_object->nodesetval->nodeNr; i++) {
				char *currentZidHex;
		
				char *encryptedMessageb64;
				char *encryptedContentTypeb64;
				snprintf(xpath_str, sizeof(xpath_str), "/doc/msg[%i]/pzid", i);
				currentZidHex = linphone_get_xml_text_content(xml_ctx, xpath_str);
				if ((currentZidHex != NULL) && (strcmp(currentZidHex, (char *)selfZidHex) == 0)) {
					linphone_free_xml_text_content(currentZidHex);
					/* We found the msg node we are looking for */
					snprintf(xpath_str, sizeof(xpath_str), "/doc/msg[%i]/index", i);
					sessionIndexHex = linphone_get_xml_text_content(xml_ctx, xpath_str);
					if (sessionIndexHex != NULL) {
						usedSessionIndex = bctbx_str_to_uint32((const unsigned char *)sessionIndexHex);
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
			}
		}
		if (msg_object != NULL) xmlXPathFreeObject(msg_object);
	}

	/* do we have retrieved correctly all the needed data */
	if (encryptedMessage == NULL) {
		ms_error("[LIME] Encrypted message is null, something went wrong..."); 
		retval = LIME_UNABLE_TO_DECRYPT_MESSAGE;
		goto error;
	}

	/* shall we derive our key before going for decryption */
	if (usedSessionIndex < associatedKey.sessionIndex) {
		/* something wen't wrong with the cache, this shall never happen */
		uint8_t associatedKeyIndexHex[9];
		bctbx_uint32_to_str(associatedKeyIndexHex, associatedKey.sessionIndex);
		ms_error("[LIME] Session index [%s] < associated key's session index [%s], should not happen !", sessionIndexHex, associatedKeyIndexHex); 
		ms_free(encryptedMessage);
		retval = LIME_UNABLE_TO_DECRYPT_MESSAGE;
		goto error;
	}

	if ((usedSessionIndex - associatedKey.sessionIndex > MAX_DERIVATION_NUMBER) ) {
		/* we missed to many messages, ask for a cache reset via a ZRTP call */
		ms_error("[LIME] Too many messages missed (%i), cache should be reset by ZRTP call", usedSessionIndex - associatedKey.sessionIndex); 
		ms_free(encryptedMessage);
		retval = LIME_UNABLE_TO_DECRYPT_MESSAGE;
		goto error;
	}

	if (associatedKey.sessionIndex != usedSessionIndex) {
		uint8_t associatedKeyIndexHex[9];
		bctbx_uint32_to_str(associatedKeyIndexHex, associatedKey.sessionIndex);
		ms_warning("LIME] unexpected session index [%s] received from [%s] (expected [%s]), [%i] messages will be discarded", 
			sessionIndexHex, peerURI, associatedKeyIndexHex, usedSessionIndex-associatedKey.sessionIndex);
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
		ms_error("[LIME] Couldn't decrypt message"); 
		retval = LIME_UNABLE_TO_DECRYPT_MESSAGE;
		goto error;
	}

	/* Decrypt the content-type */
	if (encryptedContentType != NULL) {
		*content_type = (char *)ms_malloc(encryptedContentTypeLength - 16 + 1); /* content-type is same length than encrypted one with 16 bytes less for the tag + 1 to add the null termination char */
		retval = lime_decryptMessage(&associatedKey, encryptedContentType, (uint32_t)encryptedContentTypeLength, selfZid, *((uint8_t **)content_type));
		ms_free(encryptedContentType);
		if (retval != 0) {
			ms_free(*content_type);
			*content_type = NULL;
			ms_error("[LIME] Couldn't decrypt content type"); 
			retval = LIME_UNABLE_TO_DECRYPT_MESSAGE;
			goto error;
		}
	}

	/* update used key */
	lime_deriveKey(&associatedKey);
	lime_setCachedKey(cachedb, &associatedKey, LIME_RECEIVER, validityTimeSpan);

error:
	if (sessionIndexHex != NULL) {
		linphone_free_xml_text_content(sessionIndexHex);
	}
	linphone_xmlparsing_context_destroy(xml_ctx);
	return retval;
}

bool_t linphone_chat_room_lime_available(LinphoneChatRoom *cr) {
	if (cr) {
		switch (linphone_core_lime_enabled(cr->lc)) {
			case LinphoneLimeDisabled: return FALSE;
			case LinphoneLimeMandatory:
			case LinphoneLimePreferred: {
				void *zrtp_cache_db = linphone_core_get_zrtp_cache_db(cr->lc);
				if (zrtp_cache_db != NULL) {
					bool_t res;
					limeURIKeys_t associatedKeys;
					char *peer = ms_strdup_printf("%s:%s@%s"	, linphone_address_get_scheme(linphone_chat_room_get_peer_address(cr))
												  				, linphone_address_get_username(linphone_chat_room_get_peer_address(cr))
												  				, linphone_address_get_domain(linphone_chat_room_get_peer_address(cr)));
					/* retrieve keys associated to the peer URI */
					associatedKeys.peerURI = bctbx_strdup(peer);
					associatedKeys.selfURI = NULL; /* TODO : there is no sender associated to chatroom so check for any local URI available, shall we add sender to chatroom? */
					associatedKeys.associatedZIDNumber  = 0;
					associatedKeys.peerKeys = NULL;
					/* with NULL is selfURI, just retrieve keys for any local uri found in cache, shall we use a dedicated function which would
					return the list of possible uris and store the selected one in the chatroom ? */
					res = (lime_getCachedSndKeysByURI(zrtp_cache_db, &associatedKeys) == 0);
					lime_freeKeys(&associatedKeys);
					ms_free(peer);
					return res;
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
		errcode = 0;
		int retval;
		void *zrtp_cache_db = NULL; /* use a void * instead of sqlite3 * to avoid problems and ifdef when SQLITE is not available(the get function shall return NULL in that case) */
		uint8_t *decrypted_body = NULL;
		char *decrypted_content_type = NULL;
		char *peerUri = NULL;
		char *selfUri = NULL;

		ms_debug("Content type is known (%s), try to decrypt it", msg->content_type);

		zrtp_cache_db = linphone_core_get_zrtp_cache_db(lc);
		if (zrtp_cache_db == NULL) {
			ms_warning("Unable to load content of ZRTP ZID cache to decrypt message");
			errcode = 500;
			return errcode;
		}
		peerUri = ms_strdup_printf("%s:%s@%s"	, linphone_address_get_scheme(msg->from)
								   				, linphone_address_get_username(msg->from)
								   				, linphone_address_get_domain(msg->from));
		selfUri = ms_strdup_printf("%s:%s@%s"	, linphone_address_get_scheme(msg->to)
								   				, linphone_address_get_username(msg->to)
								   				, linphone_address_get_domain(msg->to));

		retval = lime_decryptMultipartMessage(zrtp_cache_db, (uint8_t *)msg->message, selfUri, peerUri, &decrypted_body, &decrypted_content_type, 
						      bctbx_time_string_to_sec(lp_config_get_string(lc->config, "sip", "lime_key_validity", "0")));
		ms_free(peerUri);
		ms_free(selfUri);
		if (retval != 0) {
			ms_warning("Unable to decrypt message, reason : %s", lime_error_code_to_string(retval));
			if (decrypted_body) ms_free(decrypted_body);
			errcode = 488;
			return errcode;
		} else {
			/* swap encrypted message with plain text message */
			if (msg->message) {
				ms_free(msg->message);
			}
			msg->message = (char *)decrypted_body;
			if (decrypted_content_type != NULL) {
				ms_debug("Decrypted content type is ", decrypted_content_type);
				linphone_chat_message_set_content_type(msg, decrypted_content_type);
				ms_free(decrypted_content_type);
			} else {
				ms_debug("Decrypted content type is unknown, use plain/text or application/vnd.gsma.rcs-ft-http+xml");
				if (strcmp("application/cipher.vnd.gsma.rcs-ft-http+xml", msg->content_type) == 0) {
					linphone_chat_message_set_content_type(msg, "application/vnd.gsma.rcs-ft-http+xml");
				} else {
					linphone_chat_message_set_content_type(msg, "text/plain");
				}
			}
		}
	} else {
		ms_message("Content type is unknown (%s), don't try to decrypt it", msg->content_type);
	}
	return errcode;
}

int lime_im_encryption_engine_process_outgoing_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	LinphoneCore *lc = linphone_im_encryption_engine_get_core(engine);
	int errcode = -1;
	const char *new_content_type = "xml/cipher";
	if(linphone_core_lime_enabled(room->lc)) {
		if (linphone_chat_room_lime_available(room)) {
			void *zrtp_cache_db = NULL; /* use a void * instead of sqlite3 * to avoid problems and ifdef when SQLITE is not available(the get function shall return NULL in that case) */
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
			zrtp_cache_db = linphone_core_get_zrtp_cache_db(lc);
			errcode = 0;
			if (zrtp_cache_db == NULL) {
				ms_warning("Unable to access ZRTP ZID cache to encrypt message");
				errcode = 488;
			} else {
				int retval;
				uint8_t *crypted_body = NULL;
				char *peerUri = ms_strdup_printf("%s:%s@%s"	, linphone_address_get_scheme(linphone_chat_room_get_peer_address(room))
															, linphone_address_get_username(linphone_chat_room_get_peer_address(room))
										   					, linphone_address_get_domain(linphone_chat_room_get_peer_address(room)));
				char *selfUri = ms_strdup_printf("%s:%s@%s"	, linphone_address_get_scheme(msg->from)
										   					, linphone_address_get_username(msg->from)
										   					, linphone_address_get_domain(msg->from));

				retval = lime_createMultipartMessage(zrtp_cache_db, msg->content_type, (uint8_t *)msg->message, selfUri, peerUri, &crypted_body);
				if (retval != 0) { /* fail to encrypt */
					ms_warning("Unable to encrypt message for %s : %s", room->peer, lime_error_code_to_string(retval));
					if (crypted_body) ms_free(crypted_body);
					errcode = 488;
				} else { /* encryption ok, swap plain text message body by encrypted one */
					if (msg->message) {
						ms_free(msg->message);
					}
					msg->message = (char *)crypted_body;
					linphone_chat_message_set_content_type(msg, new_content_type);
				}
				ms_free(peerUri);
				ms_free(selfUri);
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
int lime_decryptMultipartMessage(void *cachedb, uint8_t *message, const char *selfURI, const char *peerURI, uint8_t **output, char **content_type, uint64_t validityTimeSpan) { return LIME_NOT_ENABLED;}
int lime_createMultipartMessage(void *cachedb, const char *contentType, uint8_t *message, const char *selfURI, const char *peerURI, uint8_t **output) { return LIME_NOT_ENABLED;}
int lime_encryptFile(void **cryptoContext, unsigned char *key, size_t length, char *plain, char *cipher) {return LIME_NOT_ENABLED;}
void lime_freeKeys(limeURIKeys_t *associatedKeys){
}
int lime_getCachedSndKeysByURI(void *cachedb, limeURIKeys_t *associatedKeys){
	return LIME_NOT_ENABLED;
}
int lime_encryptMessage(limeKey_t *key, const uint8_t *plainMessage, uint32_t messageLength, uint8_t selfZID[12], uint8_t *encryptedMessage) {
	return LIME_NOT_ENABLED;
}
int lime_setCachedKey(void * cacheDb, limeKey_t *associatedKey, uint8_t role, uint64_t validityTimeSpan) {
	return LIME_NOT_ENABLED;
}
int lime_getCachedRcvKeyByZid(void * cacheDb, limeKey_t *associatedKey, const char *selfURI, const char *peerURI) {
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

const char *lime_error_code_to_string(int errorCode) {
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
