/*
	The objective of the media_api is to construct and run the necessary processing 
	on audio and video data flows for a given call (two party call) or conference.
	Copyright (C) 2001  Sharath Udupa skuds@gmx.net

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

#include "media_api.h"

/* non-standard payload types for oRTP */
PayloadType lpc1015={
    PAYLOAD_AUDIO_PACKETIZED,
    8000,
    0,
    NULL,
    0,
    2400,
    "1015/8000/1"
};

PayloadType speex_nb={
    PAYLOAD_AUDIO_PACKETIZED,
    8000,
    0,
    NULL,
    0,
    15000,
    "speex/8000/1"
};

PayloadType speex_nb_lbr={
    PAYLOAD_AUDIO_PACKETIZED,
    8000,
    0,
    NULL,
    0,
    8000,
    "speex-lbr/8000/1"
};

void media_api_init()
{
	ortp_init();
	ortp_set_debug_file("oRTP",NULL);
	rtp_profile_set_payload(&av_profile,115,&lpc1015);
	rtp_profile_set_payload(&av_profile,110,&speex_nb);
	rtp_profile_set_payload(&av_profile,111,&speex_nb_lbr);
	rtp_profile_set_payload(&av_profile,101,&telephone_event);
	ms_init();
	ms_speex_codec_init();
#ifdef HAVE_AVCODEC
	ms_AVCodec_init();
#endif
}


