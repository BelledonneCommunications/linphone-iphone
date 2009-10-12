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

#ifndef allfilters_h
#define allfilters_h

/* this is the enum where to add your own filter id.
Please take care of always add new IDs at the end in order to preserve the binary interface*/
/*this id is used for type checking of methods, events with filters */
/*it must be used also to create filters */
typedef enum MSFilterId{
	MS_FILTER_NOT_SET_ID,
	MS_FILTER_PLUGIN_ID,	/* no type checking will be performed on plugins */
	MS_FILTER_BASE_ID,
	MS_ALSA_READ_ID,
	MS_ALSA_WRITE_ID,
	MS_OSS_READ_ID,
	MS_OSS_WRITE_ID,
	MS_ULAW_ENC_ID,
	MS_ULAW_DEC_ID,
	MS_ALAW_ENC_ID,
	MS_ALAW_DEC_ID,
	MS_RTP_SEND_ID,
	MS_RTP_RECV_ID,
	MS_FILE_PLAYER_ID,
	MS_FILE_REC_ID,
	MS_DTMF_GEN_ID,
	MS_SPEEX_ENC_ID,
	MS_SPEEX_DEC_ID,
	MS_GSM_ENC_ID,
	MS_GSM_DEC_ID,
	MS_V4L_ID,
	MS_SDL_OUT_ID,
	MS_H263_ENC_ID,
	MS_H263_DEC_ID,
	MS_ARTS_READ_ID,
	MS_ARTS_WRITE_ID,
	MS_WINSND_READ_ID,
	MS_WINSND_WRITE_ID,
	MS_SPEEX_EC_ID,
	MS_PIX_CONV_ID,
	MS_TEE_ID,
	MS_SIZE_CONV_ID,
	MS_CONF_ID,
	MS_THEORA_ENC_ID,
	MS_THEORA_DEC_ID,
	MS_PASND_READ_ID,
	MS_PASND_WRITE_ID,
	MS_MPEG4_ENC_ID,
	MS_MPEG4_DEC_ID,
	MS_MJPEG_DEC_ID,
	MS_JOIN_ID,
	MS_RESAMPLE_ID,
	MS_VIDEO_OUT_ID,
	MS_VOLUME_ID,
	MS_SNOW_DEC_ID,
	MS_SNOW_ENC_ID,
	MS_CA_READ_ID,
	MS_CA_WRITE_ID,
	MS_WINSNDDS_READ_ID,
	MS_WINSNDDS_WRITE_ID,
	MS_STATIC_IMAGE_ID,
	MS_V4L2_CAPTURE_ID,
	MS_H263_OLD_DEC_ID,
	MS_H263_OLD_ENC_ID,
	MS_MIRE_ID,
	MS_VFW_ID,
	MS_ICE_ID,
	MS_VOID_SINK_ID,
	MS_DSCAP_ID,
	MS_AQ_READ_ID,
	MS_AQ_WRITE_ID,
	MS_EQUALIZER_ID,
	MS_JPEG_DEC_ID,
	MS_JPEG_ENC_ID,
} MSFilterId;


#endif
