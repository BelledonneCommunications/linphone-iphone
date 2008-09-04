/*
	The objective of the media_api is to construct and run the necessary processing 
	on audio and video data flows for a given call (two party call) or conference.
	Copyright (C) 2001  Sharath Udupa skuds@gmx.net

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
#include "common.h"
#include "mediaflow.h"
#include "callmember.h"


MediaFlow *media_flow_new(char *id_string, int type){
  MediaFlow *flow = (MediaFlow *) g_malloc(sizeof(MediaFlow));  //malloc required?
  api_trace("media_flow_new: creating %s",id_string);
  flow->id = id_string;
  flow->type = type;
  flow->flowDirections = NULL;
  flow->members = NULL;
  return flow;
}

int media_flow_destroy(MediaFlow *flow){
	g_free(flow);
	return 1;
}

int media_flow_setup_fd(MediaFlow *flow, CallMember* csource, CallMember *cdestination, int direction){
	GList *source, *destination;
	char *dir;
	FlowDirections *fd = (FlowDirections *) g_malloc(sizeof(FlowDirections));
	if(direction == MEDIA_FLOW_DUPLEX) dir = "DUPLEX";
	else if(direction == MEDIA_FLOW_HALF_DUPLEX) dir = "HALF_DUPLEX";
	api_trace("media_flow_setup_fd: setting up %s flow for %s , %s",dir, csource->name, cdestination->name);
	source = g_list_find_custom(flow->members, csource, &find);
	destination =g_list_find_custom(flow->members, cdestination, &find);
	if(source == NULL){
		api_error("media_flow_setup_fd: Invalid source %s specified", csource->name);
	}
	if(destination == NULL){
		api_error("media_flow_setup_fd: Invalid destination %s specified", cdestination->name);
		//ERROR handling to be done here
	}
	fd->source = (Members*)source->data;
	fd->destination = (Members*)destination->data;
	fd->type = direction;
	flow->flowDirections = g_list_append(flow->flowDirections, fd);
	return 1;
}

int find(gconstpointer mem, gconstpointer cmember){
	if(!strcmp(((Members*)mem)->member->name, ((CallMember*)cmember)->name)){
		return 0;
	}
	return 1;
}

int media_flow_start_fd(FlowDirections *fd, MSSync *sync){
	Members *source, *destination;
	source = fd->source;
	destination = fd->destination;
	if(fd->type == MEDIA_FLOW_DUPLEX){
		fd->recv = set_MSFilter(source->tx_endpoint,1,fd);
		fd->dec = set_CODECFilter(source->member->profile, source->tx_endpoint->pt,MEDIA_API_DECODER);
		fd->play = set_MSFilter(destination->rx_endpoint,0,fd);
		
		ms_filter_add_link(fd->recv,fd->dec);
		ms_filter_add_link(fd->dec,fd->play);
		ms_sync_attach(sync, fd->recv);
		
		fd->read = set_MSFilter(destination->tx_endpoint,1,fd);
		fd->enc = set_CODECFilter(destination->member->profile, destination->tx_endpoint->pt,MEDIA_API_ENCODER);
		fd->send = set_MSFilter(source->rx_endpoint,0,fd);
		
		ms_filter_add_link(fd->read, fd->enc);
		ms_filter_add_link(fd->enc, fd->send);
		ms_sync_attach(sync, fd->read);
		
	}
	else if(fd->type == MEDIA_FLOW_HALF_DUPLEX){
	
		fd->recv = set_MSFilter(source->tx_endpoint,1,fd);
		fd->dec = set_CODECFilter(sourcec->member->profile, source->tx_endpoint->pt,MEDIA_API_DECODER);
		fd->play = set_MSFilter(destination->rx_endpoint,0,fd);
		
		ms_filter_add_link(fd->recv,fd->dec);
		ms_filter_add_link(fd->dec,fd->play);
		ms_sync_attach(sync, fd->recv); 
	}
	return 1;
}


MSFilter *set_MSFilter(EndPoint *endpoint, int type, FlowDirections *fdir){
	MSFilter *filter;
	RtpSession *rtps;
	switch(endpoint->protocol){
		case MEDIA_RTP:
			rtps = rtp_session_new(RTP_SESSION_RECVONLY);
			rtp_session_set_local_addr(rtps,"0.0.0.0",8000);
			rtp_session_set_scheduling_mode(rtps,0);
			rtp_session_set_blocking_mode(rtps,0);
			
			if(type == 1){
				filter = ms_rtp_recv_new();
				ms_rtp_recv_set_session(MS_RTP_RECV(filter), rtps);
				fdir->rtpSessions = g_list_append(fdir->rtpSessions, rtps);
				return filter;
			}
			else{
				//ms_rtp_send_new
			}
		case MEDIA_OSS:
			if(type == 1){
				filter = ms_oss_read_new();
				ms_sound_read_set_device(MS_SOUND_READ(filter),0);
				return filter;
			}
			else{
				filter = ms_oss_write_new();
				ms_sound_write_set_device(MS_SOUND_WRITE(filter),0);
				return filter;
			}
		case MEDIA_FILE:
			if(type == 1){
				filter = ms_read_new(endpoint->file);
				return filter;
			}
			if(type == 0){
				filter = ms_write_new(endpoint->file);
				return filter;
			}

	}
}

MSFilter *set_CODECFilter(RtpProfile *profile, int pt, int mode){
	PayloadType *payload;
	
	switch(mode){
		case MEDIA_API_DECODER:	
			payload = rtp_profile_get_payload(profile, pt);
			if(payload == NULL){
				api_error("media_api: undefined payload in URL\n");
				return NULL;
			}
			return ms_decoder_new_with_string_id(payload->mime_type);
			
			//Commented this to include the new RtpProfile
			/*if(pt != -1) return ms_decoder_new_with_pt(pt);
			 *else return ms_copy_new();
			 */
		case MEDIA_API_ENCODER: 
			
			payload = rtp_profile_get_payload(profile, pt);
			if(payload == NULL){
				api_error("media_api: undefined payload in URL\n");
				return NULL;
			}
			return ms_encoder_new_with_string_id(payload->mime_type);
			/*if(pt != -1) return ms_encoder_new_with_pt(pt);
			 *else return ms_copy_new();
			 */
	}
}
	

