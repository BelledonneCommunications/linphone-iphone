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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/mediastream.h"
#ifdef VIDEO_ENABLED
#include "mediastreamer2/msv4l.h"
#endif

#include <signal.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cond=1;

static void stop_handler(int signum)
{
	cond--;
	if (cond<0) exit(-1);
}

static bool_t parse_addr(const char *addr, char *ip, int len, int *port)
{
	const char *semicolon=NULL;
	int iplen;
	int slen;
	const char *p;

	*port=0;
	semicolon=strchr(addr,':');
	for (p=addr+strlen(addr)-1;p>addr;p--){
		if (*p==':') {
			semicolon=p;
			break;
		}
	}
	if (semicolon==NULL) return FALSE;
	iplen=semicolon-addr;
	slen=MIN(iplen,len-1);
	strncpy(ip,addr,slen);
	ip[slen]='\0';
	*port=atoi(semicolon+1);
	return TRUE;
}

static void display_items(void *user_data, uint32_t csrc, rtcp_sdes_type_t t, const char *content, uint8_t content_len){
	char str[256];
	int len=MIN(sizeof(str)-1,content_len);
	strncpy(str,content,len);
	str[len]='\0';
	switch(t){
		case RTCP_SDES_CNAME:
			ms_message("Found CNAME=%s",str);
		break;
		case RTCP_SDES_TOOL:
			ms_message("Found TOOL=%s",str);
		break;
		case RTCP_SDES_NOTE:
			ms_message("Found NOTE=%s",str);
		break;
		default:
			ms_message("Unhandled SDES item (%s)",str);
	}
}

static void parse_rtcp(mblk_t *m){
	do{
		if (rtcp_is_RR(m)){
			ms_message("Receiving RTCP RR");
		}else if (rtcp_is_SR(m)){
			ms_message("Receiving RTCP SR");
		}else if (rtcp_is_SDES(m)){
			ms_message("Receiving RTCP SDES");
			rtcp_sdes_parse(m,display_items,NULL);
		}else {
			ms_message("Receiving unhandled RTCP message");
		}
	}while(rtcp_next_packet(m));
}

static void parse_events(OrtpEvQueue *q){
	OrtpEvent *ev;
	while((ev=ortp_ev_queue_get(q))!=NULL){
		OrtpEventData *d=ortp_event_get_data(ev);
		switch(ortp_event_get_type(ev)){
			case ORTP_EVENT_RTCP_PACKET_RECEIVED:
				parse_rtcp(d->packet);
			break;
			default:
				ms_warning("Unhandled ortp event.");
		}
		ortp_event_destroy(ev);
	}
}

const char *usage="mediastream --local <port> --remote <ip:port> --payload <payload type number>\n"
								"[ --fmtp <fmtpline>]\n"
								"[ --jitter <miliseconds>]\n"
								"[ --width <pixels>]\n"
								"[ --height <pixels> ]\n"
								"[ --bitrate <bits per seconds>]\n";
static void run_media_streams(int localport, const char *remote_ip, int remoteport, int payload, const char *fmtp, int jitter, bool_t ec, int bitrate, MSVideoSize vs);


int main(int argc, char * argv[])
{
	int i;
	int localport=0,remoteport=0,payload=0;
	char ip[50];
	const char *fmtp=NULL;
	int jitter=50;
	int bitrate=0;
	MSVideoSize vs;
	bool_t ec=FALSE;
	/*create the rtp session */
	ortp_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	rtp_profile_set_payload(&av_profile,115,&payload_type_lpc1015);
	rtp_profile_set_payload(&av_profile,110,&payload_type_speex_nb);
	rtp_profile_set_payload(&av_profile,111,&payload_type_speex_wb);
	rtp_profile_set_payload(&av_profile,112,&payload_type_ilbc);
#ifdef VIDEO_ENABLED
	rtp_profile_set_payload(&av_profile,98,&payload_type_h263_1998);
	rtp_profile_set_payload(&av_profile,97,&payload_type_theora);
	rtp_profile_set_payload(&av_profile,99,&payload_type_mp4v);
	rtp_profile_set_payload(&av_profile,100,&payload_type_x_snow);
	rtp_profile_set_payload(&av_profile,102,&payload_type_h264);
#endif

	vs.width=MS_VIDEO_SIZE_CIF_W;
	vs.height=MS_VIDEO_SIZE_CIF_H;
	if (argc<4) {
		printf(usage);
		return -1;
	}
	for (i=1;i<argc;i++){
		if (strcmp(argv[i],"--local")==0){
			i++;
			localport=atoi(argv[i]);
		}else if (strcmp(argv[i],"--remote")==0){
			i++;
			if (!parse_addr(argv[i],ip,sizeof(ip),&remoteport)) {
				printf(usage);
				return -1;
			}
			printf("Remote addr: ip=%s port=%i\n",ip,remoteport);
		}else if (strcmp(argv[i],"--payload")==0){
			i++;
			payload=atoi(argv[i]);
		}else if (strcmp(argv[i],"--fmtp")==0){
			i++;
			fmtp=argv[i];
		}else if (strcmp(argv[i],"--jitter")==0){
			i++;
			jitter=atoi(argv[i]);
		}else if (strcmp(argv[i],"--bitrate")==0){
			i++;
			bitrate=atoi(argv[i]);
		}else if (strcmp(argv[i],"--width")==0){
			i++;
			vs.width=atoi(argv[i]);
		}else if (strcmp(argv[i],"--height")==0){
			i++;
			vs.height=atoi(argv[i]);
		}else if (strcmp(argv[i],"--ec")==0){
			ec=TRUE;
		}
		
		
	}
	run_media_streams(localport,ip,remoteport,payload,fmtp,jitter,ec,bitrate,vs);
	return 0;
}

void run_media_streams(int localport,  const char *remote_ip, int remoteport, int payload, const char *fmtp, int jitter, bool_t ec, int bitrate, MSVideoSize vs)
{
	AudioStream *audio=NULL;
#ifdef VIDEO_ENABLED
	VideoStream *video=NULL;
#endif
	RtpSession *session=NULL;
	PayloadType *pt;
	RtpProfile *profile=rtp_profile_clone_full(&av_profile);
	OrtpEvQueue *q=ortp_ev_queue_new();	

	ms_init();
	signal(SIGINT,stop_handler);
	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		printf("Error: no payload defined with number %i.",payload);
		exit(-1);
	}
	if (fmtp!=NULL) payload_type_set_send_fmtp(pt,fmtp);
	if (bitrate>0) pt->normal_bitrate=bitrate;

	if (pt->type!=PAYLOAD_VIDEO){
		printf("Starting audio stream.\n");
		audio=audio_stream_start(profile,localport,remote_ip,remoteport,payload,jitter, ec);
		if (audio) session=audio->session;
	}else{
#ifdef VIDEO_ENABLED
		printf("Starting video stream.\n");
		video=video_stream_new(localport, ms_is_ipv6(remote_ip));
		video_stream_set_sent_video_size(video,vs);
		video_stream_start(video,profile,
					remote_ip,
					remoteport,remoteport+1,
					payload,
					jitter,
					ms_web_cam_manager_get_default_cam(ms_web_cam_manager_get()));
		session=video->session;
#else
		printf("Error: video support not compiled.\n");
#endif
	}
	rtp_session_register_event_queue(session,q);
	while(cond)
	{
		int n;
        	for(n=0;n<100;++n){
#ifdef WIN32
			MSG msg;
			Sleep(10);
			while (PeekMessage(&msg, NULL, 0, 0,1)){
        			TranslateMessage(&msg);
        			DispatchMessage(&msg);
			}
#else
			struct timespec ts;
			ts.tv_sec=0;
			ts.tv_nsec=10000000;
			nanosleep(&ts,NULL);
#endif
#if defined(VIDEO_ENABLED)
			if (video) video_stream_iterate(video);
#endif
		}
		ortp_global_stats_display();
		if (session){
			printf("Bandwidth usage: download=%f kbits/sec, upload=%f kbits/sec\n",
				rtp_session_compute_recv_bandwidth(session)*1e-3,
				rtp_session_compute_send_bandwidth(session)*1e-3);
			parse_events(q);
		}
	}
	
	printf("stoping all...\n");
	
	if (audio) audio_stream_stop(audio);
#ifdef VIDEO_ENABLED
	if (video) video_stream_stop(video);
#endif
	ortp_ev_queue_destroy(q);
	rtp_profile_destroy(profile);
}
