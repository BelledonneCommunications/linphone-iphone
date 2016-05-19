/*
sqlite3_bctbx_vfs.h
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include "sqlite3.h"

#include <bctoolbox/bc_vfs.h>


/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 512


/**
 * sqlite3_bctbx_file VFS file structure.
 */
typedef struct sqlite3_bctbx_file sqlite3_bctbx_file;
struct sqlite3_bctbx_file {
	sqlite3_file base;              /* Base class. Must be first. */
	bctbx_vfs_file_t* pbctbx_file;
};



/**
 * Very simple VFS structure based on sqlite3_vfs. 
 * Only the Open function is implemented, 
 */
typedef struct sqlite3_bctbx_vfs sqlite3_bctbx_vfs;
struct sqlite3_bctbx_vfs {
	sqlite3_bctbx_vfs *pNext;      /* Next registered VFS */
	const char *vfsName;       /* Virtual file system name */
	int (*xOpen)(sqlite3_vfs* pVfs, const char *fName, sqlite3_file *pFile,int flags, int *pOutFlags);
	
};


/****************************************************
VFS API to register this VFS to sqlite3 VFS
*****************************************************/
sqlite3_vfs *sqlite3_bctbx_vfs_create(void);
void sqlite3_bctbx_vfs_register(int makeDefault);
void sqlite3_bctbx_vfs_unregister(void);