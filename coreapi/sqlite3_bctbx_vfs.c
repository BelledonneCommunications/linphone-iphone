/*
sqlite3_bctbx_vfs.c
Copyright (C) 2016 Belledonne Communications SARL

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

#ifdef SQLITE_STORAGE_ENABLED
#include "private.h"

#include "sqlite3_bctbx_vfs.h"
#include <sqlite3.h>

#ifndef _WIN32_WCE
#include <errno.h>
#endif /*_WIN32_WCE*/


#ifndef _WIN32
#if !defined(__QNXNTO__)
#	include <langinfo.h>
#	include <locale.h>
#	include <iconv.h>
#	include <string.h>
#endif

#endif


/**
 * Closes the file whose file descriptor is stored in the file handle p.
 * @param  p 	sqlite3_file file handle pointer.
 * @return      SQLITE_OK if successful,  SQLITE_IOERR_CLOSE otherwise.
 */
static int sqlite3bctbx_Close(sqlite3_file *p){

	int ret;
	sqlite3_bctbx_file_t *pFile = (sqlite3_bctbx_file_t*) p;

	ret = bctbx_file_close(pFile->pbctbx_file);
	if (!ret){
		return SQLITE_OK;
	}
	else{
		free(pFile);
		return SQLITE_IOERR_CLOSE ;
	}
}


/**
 * Read count bytes from the open file given by p, starting at offset and puts them in
 * the buffer pointed by buf.
 * Calls bctbx_file_read.
 *
 * @param  p  		sqlite3_file file handle pointer.
 * @param  buf    	buffer to write the read bytes to.
 * @param  count  	number of bytes to read
 * @param  offset 	file offset where to start reading
 * @return 			SQLITE_OK if read bytes equals count,
 *                  SQLITE_IOERR_SHORT_READ if the number of bytes read is inferior to count
 *                  SQLITE_IOERR_READ if an error occurred.
 */
static int sqlite3bctbx_Read(sqlite3_file *p, void *buf, int count, sqlite_int64 offset){
	int ret;
	sqlite3_bctbx_file_t *pFile = (sqlite3_bctbx_file_t*) p;
	if (pFile){
		ret = bctbx_file_read(pFile->pbctbx_file, buf, count, (off_t)offset);
		if( ret==count ){
			return SQLITE_OK;
		}
		else if( ret >= 0 ){
			/*fill in unread portion of buffer, as requested by sqlite3 documentation*/
			memset(((uint8_t*)buf) + ret, 0, count-ret);
			return SQLITE_IOERR_SHORT_READ;
		}else {

			return SQLITE_IOERR_READ;
		}
	}
	return SQLITE_IOERR_READ;
}

/**
 * Writes directly to the open file given through the p argument.
 * Calls bctbx_file_write .
 * @param  p       sqlite3_file file handle pointer.
 * @param  buf     Buffer containing data to write
 * @param  count   Size of data to write in bytes
 * @param  offset  File offset where to write to
 * @return         SQLITE_OK on success, SQLITE_IOERR_WRITE if an error occurred.
 */
static int sqlite3bctbx_Write(sqlite3_file *p, const void *buf, int count, sqlite_int64 offset){
	sqlite3_bctbx_file_t *pFile = (sqlite3_bctbx_file_t*) p;
	int ret;
	if (pFile ){
		ret = bctbx_file_write(pFile->pbctbx_file, buf, count, (off_t)offset);
		if(ret > 0 ) return SQLITE_OK;
		else {
			return SQLITE_IOERR_WRITE;
		}
	}
	return SQLITE_IOERR_WRITE;
}


/**
 * Saves the file size associated with the file handle p into the argument pSize.
 * @param  p 	sqlite3_file file handle pointer.
 * @return 		SQLITE_OK if read bytes equals count,
 *              SQLITE_IOERR_FSTAT if the file size returned is negative
 *              SQLITE_ERROR if an error occurred.
 */
static int sqlite3bctbx_FileSize(sqlite3_file *p, sqlite_int64 *pSize){

	int64_t rc;                         /* Return code from fstat() call */
	sqlite3_bctbx_file_t *pFile = (sqlite3_bctbx_file_t*) p;
	if (pFile->pbctbx_file){
		rc = bctbx_file_size(pFile->pbctbx_file);
		if( rc < 0 ) {
			return SQLITE_IOERR_FSTAT;
		}
		if (pSize){
			*pSize = rc;
			return SQLITE_OK;
		}
	}
	return SQLITE_ERROR;


}


/************************ PLACE HOLDER FUNCTIONS ***********************/
/** These functions were implemented to please the SQLite VFS
implementation. Some of them are just stubs, some do a very limited job. */


/**
 * Returns the device characteristics for the file. Stub function
 * to fill the SQLite VFS function pointer structure .
 * @param  p 	sqlite3_file file handle pointer.
 * @return		value 4096.
 */
static int sqlite3bctbx_DeviceCharacteristics(sqlite3_file *p){
	int rc = 0x00001000;
	return rc;
}

/**
 * Stub function for information and control over the open file.
 * @param  p    sqlite3_file file handle pointer.
 * @param  op   operation
 * @param  pArg unused
 * @return      SQLITE_OK on success, SALITE_NOTFOUND otherwise.
 */
static int sqlite3bctbx_FileControl(sqlite3_file *p, int op, void *pArg){
#ifdef SQLITE_FCNTL_MMAP_SIZE
	if (op == SQLITE_FCNTL_MMAP_SIZE) return SQLITE_OK;
#endif
	return SQLITE_NOTFOUND;

}

/**
 * The lock file mechanism is not used with this VFS : checking
 * the reserved lock is always OK.
 * @param  pUnused sqlite3_file file handle pointer.
 * @param  pResOut set to 0 since there is no lock mechanism.
 * @return         SQLITE_OK
 */
static int sqlite3bctbx_nolockCheckReservedLock(sqlite3_file *pUnused, int *pResOut){
	*pResOut = 0;
	return SQLITE_OK;
}

/**
 * The lock file mechanism is not used with this VFS : locking the file
 * is always OK.
 * @param  pUnused sqlite3_file file handle pointer.
 * @param  unused  unused
 * @return         SQLITE_OK
 */
static int sqlite3bctbx_nolockLock(sqlite3_file *pUnused, int unused){
	return SQLITE_OK;
}

/**
 * The lock file mechanism is not used with this VFS : unlocking the file
  * is always OK.
 * @param  pUnused sqlite3_file file handle pointer.
 * @param  unused  unused
 * @return         SQLITE_OK
 */
static int sqlite3bctbx_nolockUnlock(sqlite3_file *pUnused, int unused){
	return SQLITE_OK;
}


/**
 * Simply sync the file contents given through the file handle p
 * to the persistent media.
 * @param  p 	 sqlite3_file file handle pointer.
 * @param  flags unused
 * @return       SQLITE_OK on success, SLITE_IOERR_FSYNC if an error occurred.
 */
static int sqlite3bctbx_Sync(sqlite3_file *p, int flags){
	sqlite3_bctbx_file_t *pFile = (sqlite3_bctbx_file_t*)p;
#if _WIN32
	int ret;
	ret = FlushFileBuffers((HANDLE)_get_osfhandle(pFile->pbctbx_file->fd));
	return (ret!=0 ? SQLITE_OK : SQLITE_IOERR_FSYNC);
#else
	int rc = fsync(pFile->pbctbx_file->fd);
	return (rc==0 ? SQLITE_OK : SQLITE_IOERR_FSYNC);
#endif
}

/************************ END OF PLACE HOLDER FUNCTIONS ***********************/



static char* ConvertFromUtf8Filename(const char* fName){
#if _WIN32
	char* convertedFilename;
	int nChar, nb_byte;
	LPWSTR wideFilename;
	
	nChar = MultiByteToWideChar(CP_UTF8, 0, fName, -1, NULL, 0);
	if (nChar == 0) return NULL;
	wideFilename = bctbx_malloc(nChar*sizeof(wideFilename[0]));
	if (wideFilename == NULL) return NULL;
	nChar = MultiByteToWideChar(CP_UTF8, 0, fName, -1, wideFilename, nChar);
	if (nChar == 0) {
		bctbx_free(wideFilename);
		wideFilename = 0;
	}
	
	nb_byte = WideCharToMultiByte(CP_ACP, 0, wideFilename, -1, 0, 0, 0, 0);
	if (nb_byte == 0) return NULL;
	convertedFilename = bctbx_malloc(nb_byte);
	if (convertedFilename == NULL) return NULL;
	nb_byte = WideCharToMultiByte(CP_ACP, 0, wideFilename, -1, convertedFilename, nb_byte, 0, 0);
	if (nb_byte == 0) {
		bctbx_free(convertedFilename);
		convertedFilename = 0;
	}
	bctbx_free(wideFilename);
	return convertedFilename;
#elif defined(__QNXNTO__) || (defined(ANDROID) && defined(__LP64__))
	return bctbx_strdup(fName);
#else
	#define MAX_PATH_SIZE 1024
	char db_file_utf8[MAX_PATH_SIZE] = {'\0'};
	char db_file_locale[MAX_PATH_SIZE] = "";
	char *outbuf=db_file_locale, *inbuf=db_file_utf8;
	size_t inbyteleft = MAX_PATH_SIZE, outbyteleft = MAX_PATH_SIZE;
	iconv_t cb;
	
	if (strcasecmp("UTF-8", nl_langinfo(CODESET)) == 0) {
		strncpy(db_file_locale, fName, MAX_PATH_SIZE - 1);
	} else {
		strncpy(db_file_utf8, fName, MAX_PATH_SIZE-1);
		cb = iconv_open(nl_langinfo(CODESET), "UTF-8");
		if (cb != (iconv_t)-1) {
			int ret;
			ret = iconv(cb, &inbuf, &inbyteleft, &outbuf, &outbyteleft);
			if(ret == -1) db_file_locale[0] = '\0';
			iconv_close(cb);
		}
	}
	return bctbx_strdup(db_file_locale);
#endif
}

/**
 * Opens the file fName and populates the structure pointed by p
 * with the necessary io_methods
 * Methods not implemented for version 1 : xTruncate, xSectorSize.
 * Initializes some fields in the p structure, some of which where already
 * initialized by SQLite.
 * @param  pVfs      sqlite3_vfs VFS pointer.
 * @param  fName     filename
 * @param  p         file handle pointer
 * @param  flags     db file access flags
 * @param  pOutFlags flags used by SQLite
 * @return           SQLITE_CANTOPEN on error, SQLITE_OK on success.
 */
static  int sqlite3bctbx_Open(sqlite3_vfs *pVfs, const char *fName, sqlite3_file *p, int flags, int *pOutFlags ){
	static const sqlite3_io_methods sqlite3_bctbx_io = {
		1,										/* iVersion         Structure version number */
		sqlite3bctbx_Close,                 	/* xClose */
		sqlite3bctbx_Read,                  	/* xRead */
		sqlite3bctbx_Write,                 	/* xWrite */
		NULL,									/* xTruncate */
		sqlite3bctbx_Sync,
		sqlite3bctbx_FileSize,
		sqlite3bctbx_nolockLock,
		sqlite3bctbx_nolockUnlock,
		sqlite3bctbx_nolockCheckReservedLock,
		sqlite3bctbx_FileControl,
		NULL,									/* xSectorSize */
		sqlite3bctbx_DeviceCharacteristics
		/*other function points follows, all NULL but not present in all sqlite3 versions.*/
	};

	sqlite3_bctbx_file_t * pFile = (sqlite3_bctbx_file_t*)p; /*File handle sqlite3_bctbx_file_t*/
	int openFlags = 0;
	char* wFname;

	/*returns error if filename is empty or file handle not initialized*/
	if (pFile == NULL || fName == NULL){
		return SQLITE_IOERR;
	}
	
	/* Set flags  to open the file with */
	if( flags&SQLITE_OPEN_EXCLUSIVE ) openFlags  |= O_EXCL;
	if( flags&SQLITE_OPEN_CREATE )    openFlags |= O_CREAT;
	if( flags&SQLITE_OPEN_READONLY )  openFlags |= O_RDONLY;
	if( flags&SQLITE_OPEN_READWRITE ) openFlags |= O_RDWR;

#if defined(_WIN32)
	openFlags |= O_BINARY;
#endif
	wFname = ConvertFromUtf8Filename(fName);
	if (wFname != NULL) {
		pFile->pbctbx_file = bctbx_file_open2(bctbx_vfs_get_default(), wFname, openFlags);
		bctbx_free(wFname);
	} else {
		pFile->pbctbx_file = NULL;
	}
	
	if( pFile->pbctbx_file == NULL){
		return SQLITE_CANTOPEN;
	}

	if( pOutFlags ){
    	*pOutFlags = flags;
  	}
	pFile->base.pMethods = &sqlite3_bctbx_io;

	return SQLITE_OK;
}



sqlite3_vfs *sqlite3_bctbx_vfs_create(void){
  static sqlite3_vfs bctbx_vfs = {
    1,								/* iVersion */
    sizeof(sqlite3_bctbx_file_t),	/* szOsFile */
    MAXPATHNAME,					/* mxPathname */
    NULL,							/* pNext */
    LINPHONE_SQLITE3_VFS,			/* zName */
    NULL,							/* pAppData */
    sqlite3bctbx_Open,				/* xOpen */
    NULL,							/* xDelete */
    NULL,							/* xAccess */
    NULL							/* xFullPathname */
  };
  return &bctbx_vfs;
}

/*static int sqlite3bctbx_winFullPathname(
										sqlite3_vfs *pVfs,            // Pointer to vfs object
										const char *zRelative,        // Possibly relative input path
										int nFull,                    // Size of output buffer in bytes
										char *zFull){
	//LPWSTR zTemp;
	//DWORD nByte;
	// If this path name begins with "/X:", where "X" is any alphabetic
	// character, discard the initial "/" from the pathname.
	//
	//if (zRelative[0] == '/' && sqlite3Isalpha(zRelative[1]) && zRelative[2] == ':'){
	//	zRelative++;
	//}

	 nByte = GetFullPathNameW((LPCWSTR)zRelative, 0, 0, 0);
	 if (nByte == 0){
		return SQLITE_CANTOPEN_FULLPATH;
	 }
	 nByte += 3;
	 zTemp = bctbx_malloc(nByte*sizeof(zTemp[0]));
	 memset(zTemp, 0, nByte*sizeof(zTemp[0]));
	 if (zTemp == 0){
		return SQLITE_IOERR_NOMEM;
	 }
	 nByte = GetFullPathNameW((LPCWSTR)zRelative, nByte, zTemp, 0);
	 if (nByte == 0){
		bctbx_free(zTemp);
		return SQLITE_CANTOPEN_FULLPATH;
	 }
	 if (zTemp){
		sqlite3_snprintf(MIN(nFull, pVfs->mxPathname), zFull, "%s", zTemp);
		bctbx_free(zTemp);
		return SQLITE_OK;
	 }
	 else{
		return SQLITE_IOERR_NOMEM;
	 }
	sqlite3_snprintf(MIN(nFull, pVfs->mxPathname), zFull, "%s", zRelative);
	return SQLITE_OK;
}*/



void sqlite3_bctbx_vfs_register( int makeDefault){
	sqlite3_vfs* pVfsToUse = sqlite3_bctbx_vfs_create();
	#if _WIN32
	sqlite3_vfs* pDefault = sqlite3_vfs_find("win32");
	#else
	sqlite3_vfs* pDefault = sqlite3_vfs_find("unix-none");
	#endif
	pVfsToUse->xCurrentTime = pDefault->xCurrentTime;
	
	pVfsToUse->xAccess =  pDefault->xAccess;
	pVfsToUse->xFullPathname = pDefault->xFullPathname;

	pVfsToUse->xDelete = pDefault->xDelete;
	pVfsToUse->xSleep = pDefault->xSleep;
	pVfsToUse->xRandomness = pDefault->xRandomness;
	pVfsToUse->xGetLastError = pDefault->xGetLastError; /* Not implemented by sqlite3 :place holder */
	/*Functions below should not be a problem sincve we are declaring ourselves
	 in version 1 */

	/* used in version 2
	xCurrentTimeInt64;*/
	/* used in version 3
	xGetSystemCall
	xSetSystemCall
	xNextSystemCall*/

	sqlite3_vfs_register(pVfsToUse, makeDefault);

}


void sqlite3_bctbx_vfs_unregister(void)
{
	sqlite3_vfs* pVfs = sqlite3_vfs_find(LINPHONE_SQLITE3_VFS);
	sqlite3_vfs_unregister(pVfs);
}

#endif /*SQLITE_STORAGE_ENABLED*/
