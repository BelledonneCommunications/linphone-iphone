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


#include "mscodec.h"

#ifndef _WIN32
#	include "msGSMdecoder.h"
#	include "msLPC10decoder.h"
#endif
#include "msMUlawdec.h"
#include "msAlawdec.h"

#ifdef TRUESPEECH
extern MSCodecInfo TrueSpeechinfo;
#endif

#ifdef VIDEO_ENABLED
extern void ms_AVCodec_init();
#endif

#define UDP_HDR_SZ 8
#define RTP_HDR_SZ 12
#define IP4_HDR_SZ 20   /*20 is the minimum, but there may be some options*/




/* register all statically linked codecs */
void ms_codec_register_all()
{
#ifndef _WIN32
	ms_filter_register(MS_FILTER_INFO(&GSMinfo));
	ms_filter_register(MS_FILTER_INFO(&LPC10info));
#endif
	ms_filter_register(MS_FILTER_INFO(&MULAWinfo));
	ms_filter_register(MS_FILTER_INFO(&ALAWinfo));
#ifdef TRUESPEECH
        ms_filter_register(MS_FILTER_INFO(&TrueSpeechinfo));
#endif
#ifdef VIDEO_ENABLED
	ms_AVCodec_init();
#endif
	
}

/* returns a list of MSCodecInfo */
GList * ms_codec_get_all_audio()
{
	GList *audio_codecs=NULL;
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if (info->type==MS_FILTER_AUDIO_CODEC){
			audio_codecs=g_list_append(audio_codecs,info);
		}
		elem=g_list_next(elem);
	}
	return audio_codecs;
}


MSCodecInfo * ms_audio_codec_info_get(gchar *name)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ( (info->type==MS_FILTER_AUDIO_CODEC) ){
			MSCodecInfo *codinfo=(MSCodecInfo *)info;
			if (strcmp(codinfo->description,name)==0){
				return MS_CODEC_INFO(info);
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

MSCodecInfo * ms_video_codec_info_get(gchar *name)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ( (info->type==MS_FILTER_VIDEO_CODEC) ){
			MSCodecInfo *codinfo=(MSCodecInfo *)info;
			if (strcmp(codinfo->description,name)==0){
				return MS_CODEC_INFO(info);
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

/* returns a list of MSCodecInfo */
GList * ms_codec_get_all_video()
{
	GList *video_codecs=NULL;
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if (info->type==MS_FILTER_VIDEO_CODEC){
			video_codecs=g_list_append(video_codecs,info);
		}
		elem=g_list_next(elem);
	}
	return video_codecs;
}

MSFilter * ms_encoder_new(gchar *name)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ((info->type==MS_FILTER_AUDIO_CODEC) || (info->type==MS_FILTER_VIDEO_CODEC)){
			MSCodecInfo *codinfo=(MSCodecInfo *)elem->data;
			if (strcmp(info->name,name)==0){
				return codinfo->encoder();
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

MSFilter * ms_decoder_new(gchar *name)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ((info->type==MS_FILTER_AUDIO_CODEC) || (info->type==MS_FILTER_VIDEO_CODEC)){
			MSCodecInfo *codinfo=(MSCodecInfo *)elem->data;
			if (strcmp(info->name,name)==0){
				return codinfo->decoder();
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

MSFilter * ms_encoder_new_with_pt(gint pt)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ((info->type==MS_FILTER_AUDIO_CODEC) || (info->type==MS_FILTER_VIDEO_CODEC)){
			MSCodecInfo *codinfo=(MSCodecInfo *)elem->data;
			if (codinfo->pt==pt){
				return codinfo->encoder();
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

MSFilter * ms_decoder_new_with_pt(gint pt)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ((info->type==MS_FILTER_AUDIO_CODEC) || (info->type==MS_FILTER_VIDEO_CODEC)){
			MSCodecInfo *codinfo=(MSCodecInfo *)elem->data;
			if (codinfo->pt==pt){
				return codinfo->decoder();
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

MSFilter * ms_decoder_new_with_string_id(gchar *id)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ((info->type==MS_FILTER_AUDIO_CODEC) || (info->type==MS_FILTER_VIDEO_CODEC)){
			MSCodecInfo *codinfo=(MSCodecInfo *)elem->data;
			if (strcasecmp(codinfo->description,id)==0){
				return codinfo->decoder();
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}

MSFilter * ms_encoder_new_with_string_id(gchar *id)
{
	GList *elem=filter_list;
	MSFilterInfo *info;
	while (elem!=NULL)
	{
		info=(MSFilterInfo *)elem->data;
		if ((info->type==MS_FILTER_AUDIO_CODEC) || (info->type==MS_FILTER_VIDEO_CODEC)){
			MSCodecInfo *codinfo=(MSCodecInfo *)elem->data;
			if (strcasecmp(codinfo->description,id)==0){
				return codinfo->encoder();
			}
		}
		elem=g_list_next(elem);
	}
	return NULL;
}
/* return 0 if codec can be used with bandwidth, -1 else*/
int ms_codec_is_usable(MSCodecInfo *codec,double bandwidth)
{
	double codec_band;
	double npacket;
	double packet_size;
	
	if (((MSFilterInfo*)codec)->type==MS_FILTER_AUDIO_CODEC)
	{
		/* calculate the total bandwdith needed by codec (including headers for rtp, udp, ip)*/		
		/* number of packet per second*/
		npacket=2.0*(double)(codec->rate)/(double)(codec->fr_size);
		packet_size=(double)(codec->dt_size)+UDP_HDR_SZ+RTP_HDR_SZ+IP4_HDR_SZ;
		codec_band=packet_size*8.0*npacket;
	}
	else return -1;
	return(codec_band<bandwidth);
}
