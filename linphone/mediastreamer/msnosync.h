/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org
  										
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "mssync.h"

#include <sys/time.h>
#define MSNOSYNC_MAX_FILTERS 10

/* MSNoSync derivates from MSSync base class*/

typedef struct _MSNoSync
{
	/* the MSSync must be the first field of the object in order to the object mechanism to work*/
	MSSync  sync;
	MSFilter *filters[MSNOSYNC_MAX_FILTERS];
	int started;
	struct timeval start,current;
} MSNoSync;


typedef struct _MSNoSyncClass
{
	/* the MSSyncClass must be the first field of the class in order to the class mechanism to work*/
	MSSyncClass parent_class;
} MSNoSyncClass;


/*private*/

void ms_nosync_init(MSNoSync *sync);
void ms_nosync_class_init(MSNoSyncClass *sync);

void ms_nosync_destroy(MSNoSync *nosync);
void ms_nosync_synchronize(MSNoSync *nosync);

/*public*/

/* casts a MSSync object into a MSNoSync */
#define MS_NOSYNC(sync) ((MSNoSync*)(sync))
/* casts a MSSync class into a MSNoSync class */
#define MS_NOSYNC_CLASS(klass) ((MSNoSyncClass*)(klass))

MSSync *ms_nosync_new();
