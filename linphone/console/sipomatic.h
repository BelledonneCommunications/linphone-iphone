/***************************************************************************
                          linphone  - sipomatic.c
This is a test program for linphone. It acts as a sip server and answers to linphone's
call.
                             -------------------
    begin                : ven mar  30
    copyright            : (C) 2001 by Simon MORLAT
    email                : simon.morlat@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "../coreapi/linphonecore.h"
#include "../coreapi/sdphandler.h"
#include <eXosip2/eXosip.h>
#undef PACKAGE
#undef VERSION
#include "mediastreamer2/mediastream.h"

#include <ortp/ortp.h>
#include <ortp/telephonyevents.h>


#define ANNOUCE_FILE8000HZ	"hello8000.wav"
#define ANNOUCE_FILE16000HZ	"hello16000.wav"

struct _Sipomatic
{
	ms_mutex_t lock;
	MSList *calls;
	double acceptance_time;
	double max_call_time;
	char *file_path8000hz;
	char *file_path16000hz;
	bool_t ipv6;
};

typedef struct _Sipomatic Sipomatic;
	
void sipomatic_init(Sipomatic *obj, char *url, bool_t ipv6);
void sipomatic_uninit(Sipomatic *obj);
void sipomatic_iterate(Sipomatic *obj);
#define sipomatic_lock(obj) ms_mutex_lock(&(obj)->lock);
#define sipomatic_unlock(obj) ms_mutex_unlock(&(obj)->lock);

void sipomatic_set_annouce_file(Sipomatic *obj, char *file);

struct stream_params{
	int ncodecs;
	int line;
	int localport;
	int remoteport;
	int pt;
	char *remaddr;
};

struct _Call
{
	Sipomatic *root;
	sdp_context_t *sdpc;
	int time;
	int did;
	int tid;
	AudioStream *audio_stream;
#ifdef VIDEO_ENABLED
	VideoStream *video_stream;
#endif
	int state;
	struct _CallParams *params;
	int eof;
	RtpProfile *profile;
	struct stream_params audio;
	struct stream_params video;
};

#define CALL_STATE_INIT 0
#define CALL_STATE_RUNNING 1
#define CALL_STATE_FINISHED 2

typedef struct _Call Call;

	
Call * call_new(Sipomatic *obj, eXosip_event_t *ev);
void call_accept(Call *call);
void call_release(Call *call);
void call_destroy(Call *call);

Call* sipomatic_find_call(Sipomatic *obj,int cid);
