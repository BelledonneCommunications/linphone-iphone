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


#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include "msrtprecv.h"
#include "msrtpsend.h"
#include "ms.h"
#include "msosswrite.h"
#include "msossread.h"
#include "msread.h"
#include "mswrite.h"
#include "mstimer.h"
#include "mscodec.h"
#include "msspeexdec.h"
#include "msringplayer.h"


struct _AudioStream
{
	MSSync *timer;
	RtpSession *send_session;
	RtpSession *recv_session;
	MSFilter *soundread;
	MSFilter *soundwrite;
	MSFilter *encoder;
	MSFilter *decoder;
	MSFilter *rtprecv;
	MSFilter *rtpsend;
};


typedef struct _AudioStream AudioStream;

struct _RingStream
{
	MSSync *timer;
	MSFilter *source;
	MSFilter *sndwrite;
};

typedef struct _RingStream RingStream;

/* start a thread that does sampling->encoding->rtp_sending|rtp_receiving->decoding->playing */
AudioStream *audio_stream_start (RtpProfile * prof, int locport, char *remip,
				 int remport, int profile, int jitt_comp);

AudioStream *audio_stream_start_with_sndcards(RtpProfile * prof, int locport, char *remip4,
				 int remport, int profile, int jitt_comp, SndCard *playcard, SndCard *captcard);

AudioStream *audio_stream_start_with_files (RtpProfile * prof, int locport,
					    char *remip4, int remport,
					    int profile, int jitt_comp,
					    gchar * infile, gchar * outfile);
void audio_stream_set_rtcp_information(AudioStream *st, const char *cname);


/* stop the above process*/
void audio_stream_stop (AudioStream * stream);

RingStream *ring_start (gchar * file, gint interval, SndCard *sndcard);
RingStream *ring_start_with_cb(gchar * file, gint interval, SndCard *sndcard, MSFilterNotifyFunc func,gpointer user_data);
void ring_stop (RingStream * stream);

/* returns the latency in samples if the audio device with id dev_id is openable in full duplex mode, else 0 */
gint test_audio_dev (int dev_id);

/* send a dtmf */
gint audio_stream_send_dtmf (AudioStream * stream, gchar dtmf);

void audio_stream_set_default_card(int cardindex);


#ifdef VIDEO_ENABLED

/*****************
  Video Support
 *****************/



struct _VideoStream
{
	MSSync *timer;
	RtpSession *send_session;
	RtpSession *recv_session;
	MSFilter *source;
	MSFilter *output;
	MSFilter *encoder;
	MSFilter *decoder;
	MSFilter *rtprecv;
	MSFilter *rtpsend;
	gboolean show_local;
};


typedef struct _VideoStream VideoStream;

VideoStream *video_stream_start(RtpProfile *profile, int locport, char *remip4, int remport,
				      int payload, int jitt_comp, gboolean show_local, const gchar *source, const gchar *device);
void video_stream_set_rtcp_information(VideoStream *st, const char *cname);
void video_stream_stop (VideoStream * stream);

VideoStream * video_preview_start(const gchar *source, const gchar *device);
void video_preview_stop(VideoStream *stream);

VideoStream * video_stream_send_only_start(RtpProfile *profile, int locport, char *remip, int remport,
			 int payload, const gchar *source, const gchar *device);

void video_stream_send_only_stop(VideoStream *stream);

#endif

#endif
