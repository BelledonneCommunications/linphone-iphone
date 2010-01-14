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


#ifndef msrtp_hh
#define msrtp_hh

#include "msfilter.h"
#include "ice.h"
#include "ortp/ortp.h"

#define MS_RTP_RECV_SET_SESSION		MS_FILTER_METHOD(MS_RTP_RECV_ID,0,RtpSession*)

#define MS_RTP_SEND_SET_SESSION		MS_FILTER_METHOD(MS_RTP_SEND_ID,0,RtpSession*)

#define MS_RTP_SEND_SEND_DTMF		MS_FILTER_METHOD(MS_RTP_SEND_ID,1,const char)

#define MS_RTP_SEND_MUTE_MIC		MS_FILTER_METHOD_NO_ARG(MS_RTP_SEND_ID,3)

#define MS_RTP_SEND_UNMUTE_MIC		MS_FILTER_METHOD_NO_ARG(MS_RTP_SEND_ID,4)

#define MS_RTP_SEND_SET_RELAY_SESSION_ID	MS_FILTER_METHOD(MS_RTP_SEND_ID,5,const char *)

#define MS_RTP_SEND_SET_DTMF_DURATION	MS_FILTER_METHOD(MS_RTP_SEND_ID,1,int)

extern MSFilterDesc ms_rtp_send_desc;
extern MSFilterDesc ms_rtp_recv_desc;

#endif
