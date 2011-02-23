/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

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

#include "sal.h"
#include "offeranswer.h"

static bool_t only_telephone_event(const MSList *l){
	PayloadType *p=(PayloadType*)l->data;
	if (strcasecmp(p->mime_type,"telephone-event")!=0){
		return FALSE;
	}
	return TRUE;
}

static PayloadType * find_payload_type_best_match(const MSList *l, const PayloadType *refpt){
	PayloadType *pt;
	char value[10];
	const MSList *elem;
	PayloadType *candidate=NULL;

	for (elem=l;elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		/* the compare between G729 and G729A is for some stupid uncompliant phone*/
		if ( (strcasecmp(pt->mime_type,refpt->mime_type)==0  ||
		    (strcasecmp(pt->mime_type, "G729") == 0 && strcasecmp(refpt->mime_type, "G729A") == 0 ))
			&& pt->clock_rate==refpt->clock_rate){
			candidate=pt;
			/*good candidate, check fmtp for H264 */
			if (strcasecmp(pt->mime_type,"H264")==0){
				if (pt->recv_fmtp!=NULL && refpt->recv_fmtp!=NULL){
					int mode1=0,mode2=0;
					if (fmtp_get_value(pt->recv_fmtp,"packetization-mode",value,sizeof(value))){
						mode1=atoi(value);
					}
					if (fmtp_get_value(refpt->recv_fmtp,"packetization-mode",value,sizeof(value))){
						mode2=atoi(value);
					}
					if (mode1==mode2)
					    break; /*exact match */
				}
			}else break;
		}
	}
	return candidate;
}

static MSList *match_payloads(const MSList *local, const MSList *remote, bool_t reading_response, bool_t one_matching_codec){
	const MSList *e2;
	MSList *res=NULL;
	PayloadType *matched;
	bool_t found_codec=FALSE;
	
	for(e2=remote;e2!=NULL;e2=e2->next){
		PayloadType *p2=(PayloadType*)e2->data;
		matched=find_payload_type_best_match(local,p2);
		if (matched){
			PayloadType *newp;
			int local_number=payload_type_get_number(matched);
			int remote_number=payload_type_get_number(p2);

			if (one_matching_codec){
				if (strcasecmp(matched->mime_type,"telephone-event")!=0){
					if (found_codec){/* we have found a real codec already*/
						continue; /*this codec won't be added*/
					}else found_codec=TRUE;
				}
			}
			
			newp=payload_type_clone(matched);
			if (p2->send_fmtp)
				payload_type_set_send_fmtp(newp,p2->send_fmtp);
			res=ms_list_append(res,newp);
			/* we should use the remote numbering even when parsing a response */
			payload_type_set_number(newp,remote_number);
			if (reading_response && remote_number!=local_number){
				ms_warning("For payload type %s, proposed number was %i but the remote phone answered %i",
				          newp->mime_type, local_number, remote_number);
				/*
				 We must add this payload type with our local numbering in order to be able to receive it.
				 Indeed despite we must sent with the remote numbering, we must be able to receive with
				 our local one.
				*/
				newp=payload_type_clone(matched);
				payload_type_set_number(newp,local_number);
				res=ms_list_append(res,newp);
			}
		}else{
			ms_message("No match for %s/%i",p2->mime_type,p2->clock_rate);
		}
	}
	return res;
}



static SalStreamDir compute_dir_outgoing(SalStreamDir local, SalStreamDir answered){
	SalStreamDir res=local;
	if (local==SalStreamSendRecv){
		if (answered==SalStreamRecvOnly){
			res=SalStreamSendOnly;
		}else if (answered==SalStreamSendOnly){
			res=SalStreamRecvOnly;
		}
	}
	if (answered==SalStreamInactive){
		res=SalStreamInactive;
	}
	return res;
}

static SalStreamDir compute_dir_incoming(SalStreamDir local, SalStreamDir offered){
	SalStreamDir res=SalStreamSendRecv;
	if (local==SalStreamSendRecv){
		if (offered==SalStreamSendOnly)
			res=SalStreamRecvOnly;
		else if (offered==SalStreamRecvOnly)
			res=SalStreamSendOnly;
		else if (offered==SalStreamInactive)
			res=SalStreamInactive;
		else
			res=SalStreamSendRecv;
	}else if (local==SalStreamSendOnly){
		if (offered==SalStreamRecvOnly || offered==SalStreamSendRecv)
			res=SalStreamSendOnly;
		else res=SalStreamInactive;
	}else if (local==SalStreamRecvOnly){
		if (offered==SalStreamSendOnly || offered==SalStreamSendRecv)
			res=SalStreamRecvOnly;
		else
			res=SalStreamInactive;
	}else res=SalStreamInactive;
	return res;
}

static void initiate_outgoing(const SalStreamDescription *local_offer,
    					const SalStreamDescription *remote_answer,
    					SalStreamDescription *result){
	if (remote_answer->port!=0)
		result->payloads=match_payloads(local_offer->payloads,remote_answer->payloads,TRUE,FALSE);
	result->proto=local_offer->proto;
	result->type=local_offer->type;
	result->dir=compute_dir_outgoing(local_offer->dir,remote_answer->dir);

	if (result->payloads && !only_telephone_event(result->payloads)){
		strcpy(result->addr,remote_answer->addr);
		result->port=remote_answer->port;
		result->bandwidth=remote_answer->bandwidth;
		result->ptime=remote_answer->ptime;
	}else{
		result->port=0;
	}
}


static void initiate_incoming(const SalStreamDescription *local_cap,
    					const SalStreamDescription *remote_offer,
    					SalStreamDescription *result, bool_t one_matching_codec){
	result->payloads=match_payloads(local_cap->payloads,remote_offer->payloads, FALSE, one_matching_codec);
	result->proto=local_cap->proto;
	result->type=local_cap->type;
	result->dir=compute_dir_incoming(local_cap->dir,remote_offer->dir);
	if (result->payloads && !only_telephone_event(result->payloads)){
		strcpy(result->addr,local_cap->addr);
		result->port=local_cap->port;
		result->bandwidth=local_cap->bandwidth;
		result->ptime=local_cap->ptime;	
	}else{
		result->port=0;
	}
}

/**
 * Returns a media description to run the streams with, based on a local offer
 * and the returned response (remote).
**/
int offer_answer_initiate_outgoing(const SalMediaDescription *local_offer,
									const SalMediaDescription *remote_answer,
    							SalMediaDescription *result){
    int i,j;
	const SalStreamDescription *ls,*rs;
    for(i=0,j=0;i<local_offer->nstreams;++i){
		ms_message("Processing for stream %i",i);
		ls=&local_offer->streams[i];
		rs=sal_media_description_find_stream((SalMediaDescription*)remote_answer,ls->proto,ls->type);
    	if (rs) {
			initiate_outgoing(ls,rs,&result->streams[j]);
			++j;
		}
		else ms_warning("No matching stream for %i",i);
    }
	result->nstreams=j;
	result->bandwidth=remote_answer->bandwidth;
	strcpy(result->addr,remote_answer->addr);
	return 0;
}

/**
 * Returns a media description to run the streams with, based on the local capabilities and
 * and the received offer.
 * The returned media description is an answer and should be sent to the offerer.
**/
int offer_answer_initiate_incoming(const SalMediaDescription *local_capabilities,
						const SalMediaDescription *remote_offer,
    					SalMediaDescription *result, bool_t one_matching_codec){
    int i;
	const SalStreamDescription *ls,*rs;
							
    for(i=0;i<remote_offer->nstreams;++i){
		rs=&remote_offer->streams[i];
		ms_message("Processing for stream %i",i);
		ls=sal_media_description_find_stream((SalMediaDescription*)local_capabilities,rs->proto,rs->type);
		if (ls){
    		initiate_incoming(ls,rs,&result->streams[i],one_matching_codec);
		} else {
			/* create an inactive stream for the answer, as there where no matching stream a local capability */
			result->streams[i].dir=SalStreamInactive;
			result->streams[i].port=0;
			result->streams[i].type=rs->type;
			if (rs->type==SalOther){
				strncpy(result->streams[i].typeother,rs->typeother,sizeof(rs->typeother)-1);
			}
		}
    }
	result->nstreams=i;
	strcpy(result->username, local_capabilities->username);
	strcpy(result->addr,local_capabilities->addr);
	result->bandwidth=local_capabilities->bandwidth;
	result->session_ver=local_capabilities->session_ver;
	result->session_id=local_capabilities->session_id;
	return 0;
}
