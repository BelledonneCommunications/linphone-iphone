#include "sqlite3_bctbx_vfs.h"
#include <errno.h>
#include <sqlite3.h>

/**
 * Closes file by closing the associated file stream.
 * Sets the error errno in the argument pErrSrvd after allocating it
 * if an error occurrred.
 * @param  pFile File handle pointer.
 * @param  pErrSvd  Pointer holding the errno value in case an error occurred.
 * @return       0 if successful, -1 otherwise.
 */
static int sqlite3bctbx_Close(sqlite3_file *p){

	/*The file descriptor is not dup'ed, and will be closed when the stream created by fdopen() is closed
	The fclose() function flushes the stream pointed to by fp (writing any buffered output data using fflush(3))
	 and closes the underlying file descriptor. */
	int ret;
	sqlite3_bctbx_file *pFile = (sqlite3_bctbx_file*) p;

	ret = close(pFile->bctbx_file.fd);
	if (!ret){
		// close(pFile->fd);
		//free(p);
		//p = NULL;
		return 0;
	}
	else{
		printf("sqlite3bctbx_Close error %s", strerror(errno));
		free(pFile);
		return SQLITE_IOERR_CLOSE ;
	}
}


/**
 * Read count bytes from the open file given by pFile, starting at offset.
 * Sets the error errno in the argument pErrSrvd after allocating it
 * if an error occurrred.
 * @param  pFile  File handle pointer.
 * @param  buf    buffer to write the read bytes to.
 * @param  count  number of bytes to read
 * @param  offset file offset where to start reading
 * @return BCTBX_VFS_ERROR if erroneous read, number of bytes read (count) otherwise
 */
static int sqlite3bctbx_Read(sqlite3_file *p, void *buf, int count, sqlite_int64 offset){
	int* pErrSvd = NULL;
	int ret;
	sqlite3_bctbx_file *pFile = (sqlite3_bctbx_file*) p;
	if (pFile){
		ret = pFile->bctbx_file.pMethods->pFuncRead(&pFile->bctbx_file, buf, count, offset, pErrSvd);
		if( ret==count ){
			return SQLITE_OK;
		}
		else if( ret >= 0 ){
			if (pErrSvd){
				printf("sqlite3bctbx_Read %s \r\n", strerror(*pErrSvd));
				free(pErrSvd);
			}
			return SQLITE_IOERR_SHORT_READ;
		}
		
		else {
			
			if (pErrSvd)
			{
				printf("sqlite3bctbx_Write : %s \r\n", strerror(*pErrSvd) );
				free(pErrSvd);
			}
			return SQLITE_IOERR_READ;
			
		}
	}
	return SQLITE_IOERR_READ;
}

/**
 * Writes directly to the open file given through the pFile argument.
 * Sets the error errno in the argument pErrSrvd after allocating it
 * if an error occurrred.
 * @param  p       File handle pointer.
 * @param  buf     Buffer containing data to write
 * @param  count   Size of data to write in bytes
 * @param  offset  File offset where to write to
 * @param  pErrSvd [description
 * @return         number of bytes written (can be 0), BCTBX_VFS_ERROR if an error occurred.
 */
static int sqlite3bctbx_Write(sqlite3_file *p, const void *buf, int count, sqlite_int64 offset){
	sqlite3_bctbx_file *pFile = (sqlite3_bctbx_file*) p;
	int ret;
	int* pErrSvd = NULL;
	if (pFile ){
		ret = pFile->bctbx_file.pMethods->pFuncWrite(&pFile->bctbx_file, buf, count, offset, pErrSvd);
		if(ret > 0 ) return SQLITE_OK;
		else {
			if (pErrSvd)
			{
				printf("sqlite3bctbx_Write : %s \r\n", strerror(*pErrSvd) );
				free(pErrSvd);
			}
			return SQLITE_IOERR_WRITE;

		}
	}
	return SQLITE_IOERR_WRITE;
}
		

/**
 * Returns the file size associated with the file handle pFile.
 * @param  pFile File handle pointer.
 * @return       BCTBX_VFS_ERROR if an error occurred, BCTBX_VFS_OK otherwise.
 */
static int sqlite3bctbx_FileSize(sqlite3_file *p, sqlite_int64 *pSize){

	int rc;                         /* Return code from fstat() call */
	sqlite3_bctbx_file *pFile = (sqlite3_bctbx_file*) p;
	if (&pFile->bctbx_file){
		rc = pFile->bctbx_file.pMethods->pFuncFileSize(&pFile->bctbx_file);
		if( rc < 0 ) {
			return SQLITE_IOERR_FSTAT;
		}
		if (pSize){
			*pSize = rc;
			return SQLITE_OK;
		} 
		else
			return SQLITE_IOERR;
	}
	return SQLITE_ERROR;


}

/**
 * [sqlite3bctbx_DeviceCharacteristics description]
 * @param  p [description]
 * @return   [description]
 */
static int sqlite3bctbx_DeviceCharacteristics(sqlite3_file *p){
	int rc = 0x00001000;
	return rc;
}

/**
 * [sqlite3bctbx_FileControl description]
 * @param  p    [description]
 * @param  op   [description]
 * @param  pArg [description]
 * @return      [description]
 */
static int sqlite3bctbx_FileControl(sqlite3_file *p, int op, void *pArg){
	if (op == SQLITE_FCNTL_MMAP_SIZE) return SQLITE_OK;
	return SQLITE_NOTFOUND;

}

/**
 * [sqlite3bctbx_nolockCheckReservedLock description]
 * @param  pUnused [description]
 * @param  pResOut [description]
 * @return         [description]
 */
static int sqlite3bctbx_nolockCheckReservedLock(sqlite3_file *pUnused, int *pResOut){
	*pResOut = 0;
	return SQLITE_OK;
}

/**
 * [sqlite3bctbx_nolockLock description]
 * @param  pUnused [description]
 * @param  unused  [description]
 * @return         [description]
 */
static int sqlite3bctbx_nolockLock(sqlite3_file *pUnused, int unused){
	return SQLITE_OK;
}

/**
 * [sqlite3bctbx_nolockUnlock description]
 * @param  pUnused [description]
 * @param  unused  [description]
 * @return         [description]
 */
static int sqlite3bctbx_nolockUnlock(sqlite3_file *pUnused, int unused){
	return SQLITE_OK;
}



/**
 * [sqlite3bctbx_Open description]
 * @param  pVfs    [description]
 * @param  fName   [description]
 * @param  mode    [description]
 * @return         [description]
 */
static  int sqlite3bctbx_Open(sqlite3_vfs *pVfs, const char *fName, sqlite3_file *p, int flags, int *pOutFlags ){
	static const sqlite3_io_methods sqlite3_bctbx_io = {
		1,
		sqlite3bctbx_Close,                    /* xClose */
		sqlite3bctbx_Read,                     /* xRead */
		sqlite3bctbx_Write,                    /* xWrite */
		0,
		0,
		sqlite3bctbx_FileSize,                 /* xFileSize */
		sqlite3bctbx_nolockLock,
		sqlite3bctbx_nolockUnlock,
		sqlite3bctbx_nolockCheckReservedLock,
		sqlite3bctbx_FileControl,
		0,
		sqlite3bctbx_DeviceCharacteristics,
	};
	int oflags = 0;
	sqlite3_bctbx_file * pFile = (sqlite3_bctbx_file*)p;

	if (pFile == NULL || fName == NULL){
		return SQLITE_IOERR;
	}


	if( flags&SQLITE_OPEN_EXCLUSIVE ) oflags |= O_EXCL;
	if( flags&SQLITE_OPEN_CREATE )    oflags |= O_CREAT;
	if( flags&SQLITE_OPEN_READONLY )  oflags |= O_RDONLY;
	if( flags&SQLITE_OPEN_READWRITE ) oflags |= O_RDWR;

	

	pFile->bctbx_file.fd = open(fName, flags, S_IRUSR | S_IWUSR);
	if( pFile->bctbx_file.fd < 0 ){
		return SQLITE_CANTOPEN;
	}
//	pFile->bctbx_file.file = fdopen(pFile->bctbx_file.fd, mode);
//	if( pFile->bctbx_file.file == NULL ){
//		return SQLITE_CANTOPEN;
//	}
	if( pOutFlags ){
    	*pOutFlags = oflags;
  	}
	pFile->base.pMethods = &sqlite3_bctbx_io;
	pFile->bctbx_file.pMethods = get_bcio();
	pFile->bctbx_file.filename = (char*)fName;
	//p = (sqlite3_file*) pFile;
	return SQLITE_OK;
}

/*
** This function returns a pointer to the VFS implemented in this file.
** To make the VFS available to SQLite:
**
**   sqlite3_vfs_register(sqlite3_demovfs(), 0);
*/
sqlite3_vfs *sqlite3_bctbx_vfs_create(void){
  static sqlite3_vfs bctbx_vfs = {
    1,                            /* iVersion */
    sizeof(sqlite3_bctbx_file),   /* szOsFile */
    MAXPATHNAME,                  /* mxPathname */
    0,                            /* pNext */
    "sql3_bctbx_vfs",              /* zName */
    0,                            /* pAppData */
    sqlite3bctbx_Open,            /* xOpen */
    0,                   /* xDelete */
    0,                   /* xAccess */
    0,             /* xFullPathname */
    0,                   /* xDlOpen */
    0,                  /* xDlError */
    0,                    /* xDlSym */
    0,                  /* xDlClose */
    0,               /* xRandomness */
    0,                    /* xSleep */
    0,              /* xCurrentTime */
  };
  return &bctbx_vfs;
}

