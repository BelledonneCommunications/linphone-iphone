/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
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

/** 
 * \file payloadtype.h
 * \brief Using and creating standart and custom RTP profiles
 *
**/

#ifndef PAYLOADTYPE_H
#define PAYLOADTYPE_H
#include <ortp/port.h>

#ifdef __cplusplus
extern "C"{
#endif

/* flags for PayloadType::flags */

#define	PAYLOAD_TYPE_ALLOCATED (1)
	/* private flags for future use by ortp */
#define	PAYLOAD_TYPE_PRIV1 (1<<1)
#define	PAYLOAD_TYPE_PRIV2 (1<<2)
#define	PAYLOAD_TYPE_PRIV3 (1<<3)
	/* user flags, can be used by the application on top of oRTP */
#define	PAYLOAD_TYPE_USER_FLAG_0 (1<<4)
#define	PAYLOAD_TYPE_USER_FLAG_1 (1<<5)
#define	PAYLOAD_TYPE_USER_FLAG_2 (1<<6)
	/* ask for more if you need*/

#define PAYLOAD_AUDIO_CONTINUOUS 0
#define PAYLOAD_AUDIO_PACKETIZED 1
#define PAYLOAD_VIDEO 2
#define PAYLOAD_OTHER 3  /* ?? */

struct _PayloadType
{
	int type; /**< one of PAYLOAD_* macros*/
	int clock_rate; /**< rtp clock rate*/
	char bits_per_sample;	/* in case of continuous audio data */
	char *zero_pattern;
	int pattern_length;
	/* other useful information for the application*/
	int normal_bitrate;	/*in bit/s */
	char *mime_type; /**<actually the submime, ex: pcm, pcma, gsm*/
	int channels; /**< number of channels of audio */
	char *recv_fmtp; /* various format parameters for the incoming stream */
	char *send_fmtp; /* various format parameters for the outgoing stream */
	int flags;
	void *user_data;
};

#ifndef PayloadType_defined
#define PayloadType_defined
typedef struct _PayloadType PayloadType;
#endif

#define payload_type_set_flag(pt,flag) (pt)->flags|=((int)flag)
#define payload_type_unset_flag(pt,flag) (pt)->flags&=(~(int)flag)
#define payload_type_get_flags(pt)	(pt)->flags

#define RTP_PROFILE_MAX_PAYLOADS 128

/**
 * The RTP profile is a table RTP_PROFILE_MAX_PAYLOADS entries to make the matching
 * between RTP payload type number and the PayloadType that defines the type of
 * media.
**/
struct _RtpProfile
{
	char *name;
	PayloadType *payload[RTP_PROFILE_MAX_PAYLOADS];
};


typedef struct _RtpProfile RtpProfile;

PayloadType *payload_type_new(void);
PayloadType *payload_type_clone(PayloadType *payload);
char *payload_type_get_rtpmap(PayloadType *pt);
void payload_type_destroy(PayloadType *pt);
void payload_type_set_recv_fmtp(PayloadType *pt, const char *fmtp);
void payload_type_set_send_fmtp(PayloadType *pt, const char *fmtp);
void payload_type_append_recv_fmtp(PayloadType *pt, const char *fmtp);
void payload_type_append_send_fmtp(PayloadType *pt, const char *fmtp);


bool_t fmtp_get_value(const char *fmtp, const char *param_name, char *result, size_t result_len);

VAR_DECLSPEC RtpProfile av_profile;

#define payload_type_set_user_data(pt,p)	(pt)->user_data=(p)
#define payload_type_get_user_data(pt)		((pt)->user_data)

#define rtp_profile_get_name(profile) 	(const char*)((profile)->name)

void rtp_profile_set_payload(RtpProfile *prof, int idx, PayloadType *pt);

/**
 *	Set payload type number @index unassigned in the profile.
 *
 *@param profile an RTP profile
 *@param index	the payload type number
**/
#define rtp_profile_clear_payload(profile,index) \
	rtp_profile_set_payload(profile,index,NULL)	

/* I prefer have this function inlined because it is very often called in the code */
/**
 *
 *	Gets the payload description of the payload type @index in the profile.
 *
 *@param profile an RTP profile (a #RtpProfile object)
 *@param index	the payload type number
 *@return the payload description (a PayloadType object)
**/
static inline PayloadType * rtp_profile_get_payload(RtpProfile *prof, int idx){
	if (idx<0 || idx>=RTP_PROFILE_MAX_PAYLOADS) {
		return NULL;
	}
	return prof->payload[idx];
}
void rtp_profile_clear_all(RtpProfile *prof);
void rtp_profile_set_name(RtpProfile *prof, const char *name);
PayloadType * rtp_profile_get_payload_from_mime(RtpProfile *profile,const char *mime);
PayloadType * rtp_profile_get_payload_from_rtpmap(RtpProfile *profile, const char *rtpmap);
int rtp_profile_get_payload_number_from_mime(RtpProfile *profile,const char *mime);
int rtp_profile_get_payload_number_from_rtpmap(RtpProfile *profile, const char *rtpmap);
int rtp_profile_find_payload_number(RtpProfile *prof,const char *mime,int rate, int channels);
PayloadType * rtp_profile_find_payload(RtpProfile *prof,const char *mime,int rate, int channels);
int rtp_profile_move_payload(RtpProfile *prof,int oldpos,int newpos);

RtpProfile * rtp_profile_new(const char *name);
/* clone a profile, payload are not cloned */
RtpProfile * rtp_profile_clone(RtpProfile *prof);


/*clone a profile and its payloads (ie payload type are newly allocated, not reusing payload types of the reference profile) */
RtpProfile * rtp_profile_clone_full(RtpProfile *prof);
/* frees the profile and all its PayloadTypes*/
void rtp_profile_destroy(RtpProfile *prof);


/* some payload types */
/* audio */
VAR_DECLSPEC PayloadType payload_type_pcmu8000;
VAR_DECLSPEC PayloadType payload_type_pcma8000;
VAR_DECLSPEC PayloadType payload_type_pcm8000;
VAR_DECLSPEC PayloadType payload_type_l16_mono;
VAR_DECLSPEC PayloadType payload_type_l16_stereo;
VAR_DECLSPEC PayloadType payload_type_lpc1016;
VAR_DECLSPEC PayloadType payload_type_g729;
VAR_DECLSPEC PayloadType payload_type_g7231;
VAR_DECLSPEC PayloadType payload_type_g7221;
VAR_DECLSPEC PayloadType payload_type_g726_40;
VAR_DECLSPEC PayloadType payload_type_g726_32;
VAR_DECLSPEC PayloadType payload_type_g726_24;
VAR_DECLSPEC PayloadType payload_type_g726_16;
VAR_DECLSPEC PayloadType payload_type_gsm;
VAR_DECLSPEC PayloadType payload_type_lpc;
VAR_DECLSPEC PayloadType payload_type_lpc1015;
VAR_DECLSPEC PayloadType payload_type_speex_nb;
VAR_DECLSPEC PayloadType payload_type_speex_wb;
VAR_DECLSPEC PayloadType payload_type_speex_uwb;
VAR_DECLSPEC PayloadType payload_type_ilbc;
VAR_DECLSPEC PayloadType payload_type_amr;
VAR_DECLSPEC PayloadType payload_type_amrwb;
VAR_DECLSPEC PayloadType payload_type_truespeech;
VAR_DECLSPEC PayloadType payload_type_evrc0;
VAR_DECLSPEC PayloadType payload_type_evrcb0;

/* video */
VAR_DECLSPEC PayloadType payload_type_mpv;
VAR_DECLSPEC PayloadType payload_type_h261;
VAR_DECLSPEC PayloadType payload_type_h263;
VAR_DECLSPEC PayloadType payload_type_h263_1998;
VAR_DECLSPEC PayloadType payload_type_h263_2000;
VAR_DECLSPEC PayloadType payload_type_mp4v;
VAR_DECLSPEC PayloadType payload_type_theora;
VAR_DECLSPEC PayloadType payload_type_h264;
VAR_DECLSPEC PayloadType payload_type_x_snow;
VAR_DECLSPEC PayloadType payload_type_jpeg;

VAR_DECLSPEC PayloadType payload_type_t140;

/* non standard file transfer over UDP */
VAR_DECLSPEC PayloadType payload_type_x_udpftp;

/* telephone-event */
VAR_DECLSPEC PayloadType payload_type_telephone_event;

#ifdef __cplusplus
}
#endif

#endif
