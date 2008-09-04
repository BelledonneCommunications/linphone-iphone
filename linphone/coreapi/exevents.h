/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@free.fr)

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

#ifndef EXEVENTS_H
#define EXEVENTS_H
#include <eXosip2/eXosip.h>
#include "sdphandler.h"


void linphone_core_process_event(LinphoneCore *lc, eXosip_event_t *ev);

/* these are the SdpHandler callbacks: we are called in to be aware of the content
of the SDP messages exchanged */

int linphone_set_audio_offer(sdp_context_t *ctx);
int linphone_set_video_offer(sdp_context_t *ctx);
int linphone_accept_audio_offer(sdp_context_t *ctx,sdp_payload_t *payload);
int linphone_accept_video_offer(sdp_context_t *ctx,sdp_payload_t *payload);
int linphone_read_audio_answer(sdp_context_t *ctx,sdp_payload_t *payload);
int linphone_read_video_answer(sdp_context_t *ctx,sdp_payload_t *payload);

void linphone_core_text_received(LinphoneCore *lc, eXosip_event_t *ev);

#endif
