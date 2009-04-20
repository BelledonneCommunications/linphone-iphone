/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

#ifndef FFMPEG_PRIV_H
#define FFMPEG_PRIV_H

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#if defined(HAVE_LIBAVCODEC_AVCODEC_H)
/* new layout */
# include <libavcodec/avcodec.h>
# include <libavutil/avutil.h>
#else
/* old layout */
# include <ffmpeg/avcodec.h>
# include <ffmpeg/avutil.h>
#endif

#if defined(HAVE_LIBSWSCALE_SWSCALE_H)
/* new layout */
#  include <libswscale/swscale.h>
# elif !defined(HAVE_LIBAVCODEC_AVCODEC_H)
/* old layout */
# include <ffmpeg/swscale.h>
#else 
/* swscale.h not delivered: use linphone private version */
#  include "swscale.h"
#endif


#if LIBAVCODEC_VERSION_MAJOR <= 51
/*should work as long as nobody uses avformat.h*/
typedef struct AVPacket{
	uint8_t *data;
	int size;
}AVPacket;

static inline int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture,
                         int *got_picture_ptr,
                         AVPacket *avpkt){
	return avcodec_decode_video(avctx,picture, got_picture_ptr,avpkt->data,avpkt->size);
}
#endif

#endif /* FFMPEG_PRIV_H */
