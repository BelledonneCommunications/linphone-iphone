/*
	The objective of the media_api is to construct and run the necessary processing 
	on audio and video data flows for a given call (two party call) or conference.
	Copyright (C) 2001  Sharath Udupa skuds@gmx.net

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MEDIA_API_H
#define MEDIA_API_H

/* some mediastreamer include files....*/

#include "ms.h"

/* audio codecs ; all these header are not really required as we should use ms_codec_new..() to 
create codec filters*/
/*Commented by Sharath Udupa
#include <mscodec.h>
#include <msMUlawdec.h>
#include <msMUlawenc.h>
#include <msAlawdec.h>
#include <msAlawenc.h>
#include <msGSMdecoder.h>
#include <msGSMencoder.h>
#include <msLPC10decoder.h>
#include <msLPC10encoder.h>

#ifdef BUILD_FFMPEG
#include <msavdecoder.h>
#include <msavencoder.h>*/
#endif

/* some usefull filters of the mediastreamer */
#include "mscopy.h"
#include "msfdispatcher.h"
#include "msqdispatcher.h"

/* some synchronisation sources */
#include <msnosync.h>
#include <mstimer.h>

/* some streams sources and sinks */
#include <msossread.h>
#include <msosswrite.h>
#include <msread.h>
#include <mswrite.h>
#include <msringplayer.h>
#include <msrtprecv.h>
#include <msrtpsend.h>
#include <msv4l.h>
#include <msvideooutput.h>

#endif



