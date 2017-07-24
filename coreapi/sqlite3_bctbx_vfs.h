/*
sqlite3_bctbx_vfs_t.h
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

#ifndef sqlite3_bctx_vfs_h
#define sqlite3_bctx_vfs_h

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <bctoolbox/vfs.h>

#include "sqlite3.h"


/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 512


/**
 * sqlite3_bctbx_file_t VFS file structure.
 */
typedef struct sqlite3_bctbx_file_t sqlite3_bctbx_file_t;
struct sqlite3_bctbx_file_t {
	sqlite3_file base;              /* Base class. Must be first. */
	bctbx_vfs_file_t* pbctbx_file;
};



/**
 * Very simple VFS structure based on sqlite3_vfs. 
 * Only the Open function is implemented, 
 */
typedef struct sqlite3_bctbx_vfs_t sqlite3_bctbx_vfs_t;
struct sqlite3_bctbx_vfs_t {
	sqlite3_bctbx_vfs_t *pNext;      /* Next registered VFS */
	const char *vfsName;       /* Virtual file system name */
	int (*xOpen)(sqlite3_vfs* pVfs, const char *fName, sqlite3_file *pFile,int flags, int *pOutFlags);
	
};


/****************************************************
VFS API to register this VFS to sqlite3 VFS
*****************************************************/

/**
 * Returns a sqlite3_vfs pointer to the VFS named sqlite3bctbx_vfs 
 * implemented in this file.
 * Methods not implemented:
 *			xDelete 
 *			xAccess 
 *			xFullPathname 
 *			xDlOpen 
 *			xDlError 
 *			xDlSym 
 *			xDlClose 
 *			xRandomness 
 *			xSleep 
 *			xCurrentTime , xCurrentTimeInt64,
 *			xGetLastError
 *			xGetSystemCall
 *			xSetSystemCall
 *			xNextSystemCall
 *			To make the VFS available to SQLite
 * @return  Pointer to bctbx_vfs.
 */
sqlite3_vfs *sqlite3_bctbx_vfs_create(void);

/**
 * Registers sqlite3bctbx_vfs to SQLite VFS. If makeDefault is 1,
 * the VFS will be used by default.
 * Methods not implemented by sqlite3_bctbx_vfs_t are initialized to the one 
 * used by the unix-none VFS where all locking file operations are no-ops. 
 * @param  makeDefault  set to 1 to make the newly registered VFS be the default one, set to 0 instead.
 */
void sqlite3_bctbx_vfs_register(int makeDefault);


/**
 * Unregisters sqlite3bctbx_vfs from SQLite.
 */
void sqlite3_bctbx_vfs_unregister(void);

#endif
