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

#ifndef MSCODEC_H
#define MSCODEC_H

#include "msfilter.h"

struct _MSCodecInfo
{
	MSFilterInfo info;
	MSFilterNewFunc encoder;
	MSFilterNewFunc decoder;
	gint fr_size; /* size in char of the uncompressed frame */
	gint dt_size;	/* size in char of the compressed frame */
	gint bitrate;  /* the minimum bit rate in bits/second */
	gint rate;		/*frequency */
	gint pt;			/* the payload type number associated with this codec*/
	gchar *description;		/* a rtpmap field to describe the codec */
	guint is_usable:1; /* linphone set this flag to remember if it can use this codec considering the total bandwidth*/
	guint is_selected:1; /* linphone (user) set this flag if he allows this codec to be used*/
};

typedef struct _MSCodecInfo MSCodecInfo;

MSFilter * ms_encoder_new(gchar *name);
MSFilter * ms_decoder_new(gchar *name);

MSFilter * ms_encoder_new_with_pt(gint pt);
MSFilter * ms_decoder_new_with_pt(gint pt);

MSFilter * ms_encoder_new_with_string_id(gchar *id);
MSFilter * ms_decoder_new_with_string_id(gchar *id);

/* return 0 if codec can be used with bandwidth, -1 else*/
int ms_codec_is_usable(MSCodecInfo *codec,double bandwidth);

GList * ms_codec_get_all_audio();

GList * ms_codec_get_all_video();

MSCodecInfo * ms_audio_codec_info_get(gchar *name);
MSCodecInfo * ms_video_codec_info_get(gchar *name);

/* register all statically linked codecs */
void ms_codec_register_all();

#define MS_CODEC_INFO(codinfo)	((MSCodecInfo*)codinfo)

#endif
