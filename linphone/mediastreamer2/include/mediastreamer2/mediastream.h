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


#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/mswebcam.h"
#include "mediastreamer2/msvideo.h"
#include "ortp/ortp.h"
#include "ortp/event.h"

typedef enum EchoLimiterType{
	ELInactive,
	ELControlMic,
	ELControlSpeaker
} EchoLimiterType;

struct _AudioStream
{
	MSTicker *ticker;
	RtpSession *session;
	MSFilter *soundread;
	MSFilter *soundwrite;
	MSFilter *encoder;
	MSFilter *decoder;
	MSFilter *rtprecv;
	MSFilter *rtpsend;
	MSFilter *dtmfgen;
	MSFilter *ec;/*echo canceler*/
	MSFilter *volsend,*volrecv; /*MSVolumes*/
	MSFilter *resampler;
	MSFilter *equalizer;
	uint64_t last_packet_count;
	time_t last_packet_time;
	EchoLimiterType el_type; /*use echo limiter: two MSVolume, measured input level controlling local output level*/
	bool_t play_dtmfs;
	bool_t use_gc;
	bool_t use_agc;
	bool_t eq_active;
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _AudioStream AudioStream;

struct _RingStream
{
	MSTicker *ticker;
	MSFilter *source;
	MSFilter *sndwrite;
};

typedef struct _RingStream RingStream;



/* start a thread that does sampling->encoding->rtp_sending|rtp_receiving->decoding->playing */
AudioStream *audio_stream_start (RtpProfile * prof, int locport, const char *remip,
				 int remport, int payload_type, int jitt_comp, bool_t echo_cancel);

AudioStream *audio_stream_start_with_sndcards(RtpProfile * prof, int locport, const char *remip4, int remport, int payload_type, int jitt_comp, MSSndCard *playcard, MSSndCard *captcard, bool_t echocancel);

int audio_stream_start_with_files (AudioStream * stream, RtpProfile * prof,
					    const char *remip, int remport, int rem_rtcp_port,
					    int pt, int jitt_comp,
					    const char * infile,  const char * outfile);

void audio_stream_play(AudioStream *st, const char *name);
void audio_stream_record(AudioStream *st, const char *name);

void audio_stream_set_rtcp_information(AudioStream *st, const char *cname, const char *tool);

void audio_stream_play_received_dtmfs(AudioStream *st, bool_t yesno);

/* those two function do the same as audio_stream_start() but in two steps
this is useful to make sure that sockets are open before sending an invite; 
or to start to stream only after receiving an ack.*/
AudioStream *audio_stream_new(int locport, bool_t ipv6);
int audio_stream_start_now(AudioStream * stream, RtpProfile * prof,  const char *remip, int remport, int rem_rtcp_port, int payload_type, int jitt_comp,MSSndCard *playcard, MSSndCard *captcard, bool_t echo_cancel);
void audio_stream_set_relay_session_id(AudioStream *stream, const char *relay_session_id);
/*returns true if we are still receiving some data from remote end in the last timeout seconds*/
bool_t audio_stream_alive(AudioStream * stream, int timeout);

/*enable echo-limiter dispositve: one MSVolume in input branch controls a MSVolume in the output branch*/
void audio_stream_enable_echo_limiter(AudioStream *stream, EchoLimiterType type);

/*enable gain control, to be done before start() */
void audio_stream_enable_gain_control(AudioStream *stream, bool_t val);

/*enable automatic gain control, to be done before start() */
void audio_stream_enable_automatic_gain_control(AudioStream *stream, bool_t val);

void audio_stream_set_mic_gain(AudioStream *stream, float gain);

/*enable parametric equalizer in the stream that goes to the speaker*/
void audio_stream_enable_equalizer(AudioStream *stream, bool_t enabled);

void audio_stream_equalizer_set_gain(AudioStream *stream, int frequency, float gain, int freq_width);

/* stop the audio streaming thread and free everything*/
void audio_stream_stop (AudioStream * stream);

RingStream *ring_start (const char * file, int interval, MSSndCard *sndcard);
RingStream *ring_start_with_cb(const char * file, int interval, MSSndCard *sndcard, MSFilterNotifyFunc func, void * user_data);
void ring_stop (RingStream * stream);


/* send a dtmf */
int audio_stream_send_dtmf (AudioStream * stream, char dtmf);

void audio_stream_set_default_card(int cardindex);


/*****************
  Video Support
 *****************/


struct _VideoStream
{
	MSTicker *ticker;
	RtpSession *session;
	MSFilter *source;
	MSFilter *pixconv;
	MSFilter *sizeconv;
	MSFilter *tee;
	MSFilter *output;
	MSFilter *encoder;
	MSFilter *decoder;
	MSFilter *rtprecv;
	MSFilter *rtpsend;
	OrtpEvQueue *evq;
	MSVideoSize sent_vsize;
	int corner; /*for selfview*/
	bool_t adapt_bitrate;
};

typedef struct _VideoStream VideoStream;

VideoStream *video_stream_new(int locport, bool_t use_ipv6);
void video_stream_enable_adaptive_bitrate_control(VideoStream *s, bool_t yesno);
int video_stream_start(VideoStream * stream, RtpProfile *profile, const char *remip, int remport, int rem_rtcp_port,
		int payload, int jitt_comp, MSWebCam *device);
void video_stream_set_relay_session_id(VideoStream *stream, const char *relay_session_id);
void video_stream_set_rtcp_information(VideoStream *st, const char *cname, const char *tool);
/*function to call periodically to handle various events */
void video_stream_iterate(VideoStream *stream);
void video_stream_send_vfu(VideoStream *stream);
void video_stream_stop(VideoStream * stream);
void video_stream_set_sent_video_size(VideoStream *stream, MSVideoSize vsize);
void video_stream_enable_self_view(VideoStream *stream, bool_t val);
unsigned long video_stream_get_native_window_id(VideoStream *stream);


VideoStream * video_preview_start(MSWebCam *device, MSVideoSize vsize);
void video_preview_stop(VideoStream *stream);

int video_stream_recv_only_start(VideoStream * stream, RtpProfile *profile, const char *remip, int remport, int payload, int jitt_comp);
int video_stream_send_only_start(VideoStream * stream, RtpProfile *profile, const char *remip, int remport,
		int rem_rtcp_port, int payload, int jitt_comp, MSWebCam *device);
void video_stream_recv_only_stop(VideoStream *stream);
void video_stream_send_only_stop(VideoStream *stream);


bool_t ms_is_ipv6(const char *address);

#ifdef __cplusplus
}
#endif


#endif
