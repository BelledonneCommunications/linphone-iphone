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
/* An implementation of SndCard : the OssCard */

#ifndef OSS_CARD_H
#define OSS_CARD_H

#include "sndcard.h"

#define OSS_CARD_BUFFERS 3
struct _OssCard
{
	SndCard parent;
	gchar *dev_name;            /* /dev/dsp0 for example */
	gchar *mixdev_name;         /* /dev/mixer0 for example */
	gint fd;   /* the file descriptor of the open soundcard, 0 if not open*/
	gint ref;
	gchar *readbuf;
	gint readpos;
	gchar *writebuf;
	gint writepos; 
};

typedef struct _OssCard OssCard;
	
SndCard * oss_card_new(char *devname, char *mixdev_name);

typedef OssCard HpuxSndCard;

#endif
