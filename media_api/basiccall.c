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

#include "basiccall.h"
#include "../mediastreamer/mscodec.h"

#define ONESYNC 10
#define MULTISYNC 20

BasicCall *basic_call_new(){
	BasicCall *bc = (BasicCall*) g_malloc(sizeof(BasicCall));
	api_trace("basic_call_new: creating a basic call");
	bc->memberA = call_member_new("memberA");
	bc->memberB = call_member_new("memberB");
	return bc;
}

CallMember *basic_call_get_member(BasicCall *call, int member_nu){
	api_trace("basic_call_get_member: called for %d",member_nu);
	if(member_nu == MemberA){
		return call->memberA;
	}
	else if(member_nu == MemberB){
		return call->memberB;
	}
}

void basic_call_add_flow(BasicCall *call, MediaFlow *flow){
	api_trace("basic_call_add_flow: called for %s",flow->id);
	call->flows = g_list_append( call->flows, flow);
	return 1;
}

int find_mediaflow(gconstpointer llist, gconstpointer flow){
	//MediaFlow *mf = (MediaFlow *) ((BasicCallFlow*)llist)->mediaFlow;
	if(((MediaFlow*)flow)->id == ((MediaFlow*)llist)->id){
		return 0;
	}
	return 1;
}

int basic_call_start_flow(BasicCall *call, MediaFlow *flow){
	int i=0;
	int syncFlag=0;
	int nFlowDirections;
	MSSync *sync;
	Members *source, *destination;
	FlowDirections *fd;
	GList *elem, *selem;
	GList *snd_read = NULL, *snd_write = NULL, *filter = NULL;
	
	//Commented by Sharat
	//This is initialized in media_api.c
	//when should these functions be really called?
	//ms_init(); 
	//ortp_init(); 
	
	api_trace("basic_call_start_flow: called for flow %s", flow->id);
	
	elem = g_list_find_custom( call->flows, flow, &find_mediaflow);
	if(elem == NULL){
		api_error("basic_call_start_flow: Called for unregistered mediaflow %s", flow->id);
	}
	
	nFlowDirections = g_list_length(flow->flowDirections);
	if(flow->type == MEDIA_FLOW_VOICE){
		syncFlag = ONESYNC;
		sync = ms_timer_new();
	}
	else{
		syncFlag = MULTISYNC;
	}

	for(i=0;i< nFlowDirections; i++){
		
		if(syncFlag == MULTISYNC){
			sync = ms_timer_new();
		}
		fd = (FlowDirections*)g_list_nth_data(flow->flowDirections,i);
		source = fd->source;
		destination = fd->destination;

		media_flow_start_fd(fd, sync);
		if(fd->type == MEDIA_FLOW_DUPLEX){
			switch(source->tx_endpoint->protocol){
				case MEDIA_ALSA:
				case MEDIA_OSS:
					snd_read = g_list_append(snd_read, fd->recv);
			}
			switch(destination->rx_endpoint->protocol){
				case MEDIA_ALSA:
				case MEDIA_OSS:
					snd_write = g_list_append(snd_write, fd->play);
			}
			
			switch(destination->tx_endpoint->protocol){
				case MEDIA_ALSA:
				case MEDIA_OSS:
					snd_read = g_list_append(snd_read, fd->read);
			}
			
			switch(source->rx_endpoint->protocol){
				case MEDIA_ALSA:
				case MEDIA_OSS:
					snd_write = g_list_append(snd_write, fd->send);
			}
			
		}
		else if(fd->type == MEDIA_FLOW_HALF_DUPLEX){
			
			switch(source->tx_endpoint->protocol){
				case MEDIA_ALSA:
				case MEDIA_OSS:
					snd_read = g_list_append(snd_read, fd->recv);
			}
			switch(destination->rx_endpoint->protocol){
				case MEDIA_ALSA:
				case MEDIA_OSS:
					snd_write = g_list_append(snd_write, fd->play);
			}
		}
		if(syncFlag == MULTISYNC){
			flow->sync = g_list_append(flow->sync, sync);
		}
	}
	if(syncFlag == ONESYNC){
		ms_start(sync);
		flow->sync = g_list_append(flow->sync, sync);
	}
	if(syncFlag == MULTISYNC){
		selem = flow->sync;
		while(selem != NULL){
			ms_start(selem->data);
			selem = g_list_next(selem);
		}
	}
	filter = snd_read;
	while(filter != NULL){
		ms_sound_read_start(MS_SOUND_READ((MSFilter*)filter->data));
		filter = g_list_next(filter);
	}

	filter = snd_write;
	while(filter != NULL){
		ms_sound_write_start(MS_SOUND_WRITE((MSFilter*)filter->data));
		filter = g_list_next(filter);
	}
	return 1;
}

int basic_call_stop_flow(BasicCall *call, MediaFlow *flow){

}
