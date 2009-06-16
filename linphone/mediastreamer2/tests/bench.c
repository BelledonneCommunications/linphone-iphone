
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


#include "mediastreamer2/msticker.h"

#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilerec.h"

#include <signal.h>

#define MAX_RTP_SIZE	1500

static int run=1;

static void stop(int signum){
	run=0;
}

struct test_session {
	RtpSession *rtps;
	
	MSFilter *fplayer;
	MSFilter *encoder;
	MSFilter *rtpsend;
	
	MSFilter *rtprecv;
	MSFilter *decoder;
	MSFilter *frecorder;
};

struct bench_config {
	int num_session;
	int num_session_record;
	
	int port_origin;
	char *ip_destination;
	int port_destination;
	
	int payload;
	int rate;
	int ptime;
	char *wavfile;
	
	MSTicker *ticker;
	MSList *tsessions; /* list of struct test_session */
};

#define NUM_SESSION 50 /* num of session to start per block */
#define NUM_SESSION_RECORD 1

struct bench_config cfg[] = {
	{	NUM_SESSION,NUM_SESSION_RECORD,
		8000,"127.0.0.1",9000,8,8000,20,"test1.wav",NULL,NULL	},
	{	NUM_SESSION,NUM_SESSION_RECORD,
		9000,"127.0.0.1",8000,8,8000,20,"test1.wav",NULL,NULL	},
	
	{	NUM_SESSION,NUM_SESSION_RECORD,
		10000,"127.0.0.1",11000,8,8000,20,"test1.wav",NULL,NULL	},
	{	NUM_SESSION,NUM_SESSION_RECORD,
		11000,"127.0.0.1",10000,8,8000,20,"test1.wav",NULL,NULL	},
	
	{	0,0,0,'\0',0,0,0,0,NULL,NULL,NULL	},
};

RtpSession *create_duplex_rtpsession(int locport){
	RtpSession *rtpr;
	rtpr=rtp_session_new(RTP_SESSION_SENDRECV);
	rtp_session_set_recv_buf_size(rtpr,MAX_RTP_SIZE);
	rtp_session_set_scheduling_mode(rtpr,0);
	rtp_session_set_blocking_mode(rtpr,0);
	rtp_session_enable_adaptive_jitter_compensation(rtpr,FALSE);
	rtp_session_set_symmetric_rtp(rtpr,TRUE);
	rtp_session_set_local_addr(rtpr,"0.0.0.0",locport);
	rtp_session_signal_connect(rtpr,"timestamp_jump",(RtpCallback)rtp_session_resync,(long)NULL);
	rtp_session_signal_connect(rtpr,"ssrc_changed",(RtpCallback)rtp_session_resync,(long)NULL);
	return rtpr;
}

int init_bench(struct bench_config *bench)
{
	PayloadType *pt;
	int pos;
	int val;
	int count;
	bench->ticker=ms_ticker_new();

	count = 0;
	/* creates the couple of encoder/decoder */
	pt=rtp_profile_get_payload(&av_profile,bench->payload);
	if (pt==NULL){
		ms_error("audiostream.c: undefined payload type.");
		return count;
	}
	if (pt->clock_rate!=8000 && pt->clock_rate!=16000 && pt->clock_rate!=32000){
		ms_error("audiostream.c: wrong rate.");
		return count;
	}
	for (pos=0;pos<bench->num_session;pos++)
		{
			struct test_session *ts = (struct test_session *)ortp_malloc(sizeof(struct test_session));
			memset(ts, 0, sizeof(struct test_session));
			
			ts->rtps = create_duplex_rtpsession(bench->port_origin+pos*2);
			if (ts->rtps==NULL)
				{
					ms_error("bench.c: cannot create rtp_session!");
					ortp_free(ts);
					return count;
				}
			
			rtp_session_set_payload_type(ts->rtps,bench->payload);
			rtp_session_set_remote_addr_full(ts->rtps,
											 bench->ip_destination,
											 bench->port_destination+pos*2,
											 bench->port_destination+1+pos*2);
			
			ts->fplayer = ms_filter_new(MS_FILE_PLAYER_ID);
			if (strstr(bench->wavfile, ".au")==NULL)
				ts->encoder = ms_filter_create_encoder(pt->mime_type);
			ts->rtpsend = ms_filter_new(MS_RTP_SEND_ID);
			
			ts->rtprecv = ms_filter_new(MS_RTP_RECV_ID);
			ts->decoder = ms_filter_create_decoder(pt->mime_type);
			ts->frecorder = ms_filter_new(MS_FILE_REC_ID);
			
			if ((ts->encoder==NULL && strstr(bench->wavfile, ".au")==NULL)
				|| (ts->decoder==NULL )){
				ms_error("bench.c: No decoder available for payload %i.",bench->payload);
				if (ts->fplayer) ms_filter_destroy(ts->fplayer);
				if (ts->encoder) ms_filter_destroy(ts->encoder);
				if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
				if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
				if (ts->decoder) ms_filter_destroy(ts->decoder);
				if (ts->frecorder) ms_filter_destroy(ts->frecorder);
				ortp_free(ts);
				return count;
			}
			if (ts->fplayer==NULL){
				ms_error("bench.c: missing player filter.");
				if (ts->fplayer) ms_filter_destroy(ts->fplayer);
				if (ts->encoder) ms_filter_destroy(ts->encoder);
				if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
				if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
				if (ts->decoder) ms_filter_destroy(ts->decoder);
				if (ts->frecorder) ms_filter_destroy(ts->frecorder);
				ortp_free(ts);
				return count;
			}
			if (ts->frecorder==NULL){
				ms_error("bench.c: missing recorder filter.");
				if (ts->fplayer) ms_filter_destroy(ts->fplayer);
				if (ts->encoder) ms_filter_destroy(ts->encoder);
				if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
				if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
				if (ts->decoder) ms_filter_destroy(ts->decoder);
				if (ts->frecorder) ms_filter_destroy(ts->frecorder);
				ortp_free(ts);
				return count;
			}
			if (ts->rtpsend==NULL){
				ms_error("bench.c: missing rtpsend filter.");
				if (ts->fplayer) ms_filter_destroy(ts->fplayer);
				if (ts->encoder) ms_filter_destroy(ts->encoder);
				if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
				if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
				if (ts->decoder) ms_filter_destroy(ts->decoder);
				if (ts->frecorder) ms_filter_destroy(ts->frecorder);
				ortp_free(ts);
				return count;
			}
			if (ts->rtprecv==NULL){
				ms_error("bench.c: missing rtprecv filter.");
				if (ts->fplayer) ms_filter_destroy(ts->fplayer);
				if (ts->encoder) ms_filter_destroy(ts->encoder);
				if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
				if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
				if (ts->decoder) ms_filter_destroy(ts->decoder);
				if (ts->frecorder) ms_filter_destroy(ts->frecorder);
				ortp_free(ts);
				return count;
			}
			
			ms_filter_call_method(ts->rtpsend,MS_RTP_SEND_SET_SESSION,ts->rtps);
			ms_filter_call_method(ts->rtprecv,MS_RTP_RECV_SET_SESSION,ts->rtps);
			
			ms_filter_call_method (ts->rtprecv, MS_FILTER_SET_SAMPLE_RATE,
								   &pt->clock_rate);
			
			ms_filter_call_method (ts->frecorder, MS_FILTER_SET_SAMPLE_RATE,
								   &pt->clock_rate);
			
			val = ms_filter_call_method(ts->fplayer,MS_FILE_PLAYER_OPEN,(void*)bench->wavfile);
			if (val!=0)
				{
					ms_error("bench.c: Cannot open wav file (%s)", bench->wavfile);
					if (ts->fplayer) ms_filter_destroy(ts->fplayer);
					if (ts->encoder) ms_filter_destroy(ts->encoder);
					if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
					if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
					if (ts->decoder) ms_filter_destroy(ts->decoder);
					if (ts->frecorder) ms_filter_destroy(ts->frecorder);
					ortp_free(ts);
					return count;
				}
			
			val=0;
			ms_filter_call_method (ts->fplayer, MS_FILTER_GET_SAMPLE_RATE,
								   &val);
			if (val!=pt->clock_rate)
				{
					ms_error("bench.c: unsupported rate for wav file: codec=%i / file=%i",
							 pt->clock_rate, val);
					if (ts->fplayer) ms_filter_destroy(ts->fplayer);
					if (ts->encoder) ms_filter_destroy(ts->encoder);
					if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
					if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
					if (ts->decoder) ms_filter_destroy(ts->decoder);
					if (ts->frecorder) ms_filter_destroy(ts->frecorder);
					ortp_free(ts);
					return count;
				}
			ms_filter_call_method (ts->fplayer, MS_FILTER_GET_NCHANNELS,
								   &val);
			
			if (val!=1)
				{
					ms_error("bench.c: unsupported number of channel for wav file: codec=1 / file=%i",
							 val);
					if (ts->fplayer) ms_filter_destroy(ts->fplayer);
					if (ts->encoder) ms_filter_destroy(ts->encoder);
					if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);				
					if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
					if (ts->decoder) ms_filter_destroy(ts->decoder);
					if (ts->frecorder) ms_filter_destroy(ts->frecorder);
					ortp_free(ts);
					return count;
				}
			ms_filter_call_method_noarg(ts->fplayer,MS_FILE_PLAYER_START);
			
			if (strstr(bench->wavfile, ".au")==NULL)
				{
					ms_filter_link(ts->fplayer,0,ts->encoder,0);
					ms_filter_link(ts->encoder,0,ts->rtpsend,0);
				}
			else
				{
					ms_filter_link(ts->fplayer,0,ts->rtpsend,0);
				}
			
			ms_filter_link(ts->rtprecv,0,ts->decoder,0);
			ms_filter_link(ts->decoder,0,ts->frecorder,0);
			
			ms_ticker_attach(bench->ticker,ts->fplayer);
			ms_ticker_attach(bench->ticker,ts->rtprecv);
			
			if (pos < bench->num_session_record)
			{
				char rec_file[128];
				snprintf(rec_file, sizeof(rec_file), "rec_%s_%i.wav",
						 bench->ip_destination,
						 bench->port_destination+pos*2);
				ms_filter_call_method(ts->frecorder,MS_FILE_REC_OPEN,(void*)rec_file);
				ms_filter_call_method_noarg(ts->frecorder,MS_FILE_REC_START);
			}
			
			bench->tsessions = ms_list_append(bench->tsessions, (void*)ts);
			count++;
		}

	return count;
}

static int uninit_bench(struct bench_config *bench)
{
	MSList *it;
	for(it=bench->tsessions;it!=NULL;it=bench->tsessions){
		struct test_session *ts = (struct test_session *)it->data;
		bench->tsessions = ms_list_remove_link(bench->tsessions, it);

		ms_ticker_detach(bench->ticker,ts->fplayer);
		ms_ticker_detach(bench->ticker,ts->rtprecv);
		
		ms_filter_call_method_noarg(ts->frecorder,MS_FILE_REC_CLOSE);
		
		if (strstr(bench->wavfile, ".au")==NULL)
			{
				ms_filter_unlink(ts->fplayer,0,ts->encoder,0);
				ms_filter_unlink(ts->encoder,0,ts->rtpsend,0);
			}
		else
			{
				ms_filter_unlink(ts->fplayer,0,ts->rtpsend,0);
			}
		
		ms_filter_unlink(ts->rtprecv,0,ts->decoder,0);
		ms_filter_unlink(ts->decoder,0,ts->frecorder,0);
			
		if (ts->fplayer) ms_filter_destroy(ts->fplayer);
		if (ts->encoder) ms_filter_destroy(ts->encoder);
		if (ts->rtpsend) ms_filter_destroy(ts->rtpsend);
		
		if (ts->rtprecv) ms_filter_destroy(ts->rtprecv);
		if (ts->decoder) ms_filter_destroy(ts->decoder);
		if (ts->frecorder) ms_filter_destroy(ts->frecorder);

		ortp_free(ts);
	}
	
	ms_ticker_destroy(bench->ticker);
	return 0;
}


int main(int argc, char *argv[]){
	int pos;
	int count;
	ortp_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	ms_init();
	rtp_profile_set_payload(&av_profile,115,&payload_type_lpc1015);
	rtp_profile_set_payload(&av_profile,110,&payload_type_speex_nb);
	rtp_profile_set_payload(&av_profile,111,&payload_type_speex_wb);
	rtp_profile_set_payload(&av_profile,112,&payload_type_ilbc);
	
	signal(SIGINT,stop);

	count=0;
	for (pos=0;cfg[pos].num_session!=0;pos++)
		{
			count = count + init_bench(&cfg[pos]);
			ms_sleep(10);
		}

	ms_message("Number of session started: %i.", count);
	
	while(run)
		ms_sleep(1);

	for (pos=0;cfg[pos].num_session!=0;pos++)
		{
			uninit_bench(&cfg[pos]);
		}

	return 0;
}

