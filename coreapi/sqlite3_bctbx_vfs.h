#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "sqlite3.h"

#include <bctoolbox/bc_vfs.h>

/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 512


/**
 */
typedef struct sqlite3_bctbx_file sqlite3_bctbx_file;
struct sqlite3_bctbx_file {
	sqlite3_file base;              /* Base class. Must be first. */
	bctbx_vfs_file bctbx_file;
};



/**
 */
typedef struct sqlite3_bctbx_vfs sqlite3_bctbx_vfs;
struct sqlite3_bctbx_vfs {
	sqlite3_bctbx_vfs *pNext;      /* Next registered VFS */
	const char *vfsName;       /* Virtual file system name */
	int (*xOpen)(sqlite3_vfs* pVfs, const char *fName, sqlite3_file *pFile,int flags, int *pOutFlags);
	// int (*xDelete)(bc_vfs*, const char *vfsName, int syncDir);
	// int (*xFullPathname)(bc_vfs*, const char *vfsName, int nOut, char *zOut);
	
};

sqlite3_vfs *sqlite3_bctbx_vfs_create(void);
//int bctbx_file_read(bc_vfs_file* pFile, void *buf, int count, uint64_t offset);
//int bctbx_file_close(bc_vfs_file* pFile);
//bc_vfs_file* bctbx_file_open(sqlite3_vfs* pVfs, const char *fName,  const char* mode);
