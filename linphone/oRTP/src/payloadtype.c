/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
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

#include "ortp/ortp.h"
#include "ortp/payloadtype.h"
#include "utils.h"

char *payload_type_get_rtpmap(PayloadType *pt)
{
	int len=(int)strlen(pt->mime_type)+15;
	char *rtpmap=(char *) ortp_malloc(len);
	if (pt->channels>0)
		snprintf(rtpmap,len,"%s/%i/%i",pt->mime_type,pt->clock_rate,pt->channels);
	else
		 snprintf(rtpmap,len,"%s/%i",pt->mime_type,pt->clock_rate);
	return rtpmap;
}

PayloadType *payload_type_new()
{
	PayloadType *newpayload=(PayloadType *)ortp_new0(PayloadType,1);
	newpayload->flags|=PAYLOAD_TYPE_ALLOCATED;
	return newpayload;
}


PayloadType *payload_type_clone(PayloadType *payload)
{
	PayloadType *newpayload=(PayloadType *)ortp_new0(PayloadType,1);
	memcpy(newpayload,payload,sizeof(PayloadType));
	newpayload->mime_type=ortp_strdup(payload->mime_type);
	if (payload->recv_fmtp!=NULL) {
		newpayload->recv_fmtp=ortp_strdup(payload->recv_fmtp);
	}
	if (payload->send_fmtp!=NULL){
		newpayload->send_fmtp=ortp_strdup(payload->send_fmtp);
	}
	newpayload->flags|=PAYLOAD_TYPE_ALLOCATED;
	return newpayload;
}

static bool_t canWrite(PayloadType *pt){
	if (!(pt->flags & PAYLOAD_TYPE_ALLOCATED)) {
		ortp_error("Cannot change parameters of statically defined payload types: make your"
			" own copy using payload_type_clone() first.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Sets a recv parameters (fmtp) for the PayloadType.
 * This method is provided for applications using RTP with SDP, but
 * actually the ftmp information is not used for RTP processing.
**/
void payload_type_set_recv_fmtp(PayloadType *pt, const char *fmtp){
	if (canWrite(pt)){
		if (pt->recv_fmtp!=NULL) ortp_free(pt->recv_fmtp);
		if (fmtp!=NULL) pt->recv_fmtp=ortp_strdup(fmtp);
		else pt->recv_fmtp=NULL;
	}
}

/**
 * Sets a send parameters (fmtp) for the PayloadType.
 * This method is provided for applications using RTP with SDP, but
 * actually the ftmp information is not used for RTP processing.
**/
void payload_type_set_send_fmtp(PayloadType *pt, const char *fmtp){
	if (canWrite(pt)){
		if (pt->send_fmtp!=NULL) ortp_free(pt->send_fmtp);
		if (fmtp!=NULL) pt->send_fmtp=ortp_strdup(fmtp);
		else pt->send_fmtp=NULL;
	}
}



void payload_type_append_recv_fmtp(PayloadType *pt, const char *fmtp){
	if (canWrite(pt)){
		if (pt->recv_fmtp==NULL)
			pt->recv_fmtp=ortp_strdup(fmtp);
		else{
			char *tmp=ortp_strdup_printf("%s;%s",pt->recv_fmtp,fmtp);
			ortp_free(pt->recv_fmtp);
			pt->recv_fmtp=tmp;
		}
	}
}

void payload_type_append_send_fmtp(PayloadType *pt, const char *fmtp){
	if (canWrite(pt)){
		if (pt->send_fmtp==NULL)
			pt->send_fmtp=ortp_strdup(fmtp);
		else{
			char *tmp=ortp_strdup_printf("%s;%s",pt->send_fmtp,fmtp);
			ortp_free(pt->send_fmtp);
			pt->send_fmtp=tmp;
		}
	}
}


/**
 * Frees a PayloadType.
**/
void payload_type_destroy(PayloadType *pt)
{
	if (pt->mime_type) ortp_free(pt->mime_type);
	if (pt->recv_fmtp) ortp_free(pt->recv_fmtp);
	if (pt->send_fmtp) ortp_free(pt->send_fmtp);
	ortp_free(pt);
}

/**
 * Parses a fmtp string such as "profile=0;level=10", finds the value matching
 * parameter param_name, and writes it into result. 
 * Despite fmtp strings are not used anywhere within oRTP, this function can
 * be useful for people using RTP streams described from SDP.
 * @param fmtp the fmtp line (format parameters)
 * @param param_name the parameter to search for
 * @param result the value given for the parameter (if found)
 * @param result_len the size allocated to hold the result string
 * @return TRUE if the parameter was found, else FALSE.
**/ 
bool_t fmtp_get_value(const char *fmtp, const char *param_name, char *result, size_t result_len){
	const char *pos=strstr(fmtp,param_name);
	memset(result, '\0', result_len);
	if (pos){
		const char *equal=strchr(pos,'=');
		if (equal){
			int copied;
			const char *end=strchr(equal+1,';');
			if (end==NULL) end=fmtp+strlen(fmtp); /*assuming this is the last param */
			copied=MIN(result_len-1,end-(equal+1));
			strncpy(result,equal+1,copied);
			result[copied]='\0';
			return TRUE;
		}
	}
	return FALSE;
}


int rtp_profile_get_payload_number_from_mime(RtpProfile *profile,const char *mime)
{
	PayloadType *pt;
	int i;
	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++)
	{
		pt=rtp_profile_get_payload(profile,i);
		if (pt!=NULL)
		{
			if (strcasecmp(pt->mime_type,mime)==0){
				return i;
			}
		}
	}
	return -1;
}

int rtp_profile_find_payload_number(RtpProfile*profile,const char *mime,int rate,int channels)
{
	int i;
	PayloadType *pt;
	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++)
	{
		pt=rtp_profile_get_payload(profile,i);
		if (pt!=NULL)
		{
			if (strcasecmp(pt->mime_type,mime)==0 &&
			    pt->clock_rate==rate &&
			    (pt->channels==channels || channels<=0 || pt->channels<=0)) {
				/*we don't look at channels if it is undefined
				ie a negative or zero value*/
			
				return i;
			}
		}
	}
	return -1;
}

int rtp_profile_get_payload_number_from_rtpmap(RtpProfile *profile,const char *rtpmap)
{
	int clock_rate, channels, ret;
	char* subtype = ortp_strdup( rtpmap );
	char* rate_str = NULL;
	char* chan_str = NULL;
	
	
	/* find the slash after the subtype */
	rate_str = strchr(subtype, '/');
	if (rate_str && strlen(rate_str)>1) {
		*rate_str = 0;
		rate_str++;
		
		/* Look for another slash */
		chan_str = strchr(rate_str, '/');
		if (chan_str && strlen(chan_str)>1) {
			*chan_str = 0;
			chan_str++;
		} else {
			chan_str = NULL;
		}
	} else {
		rate_str = NULL;
	}
	
	// Use default clock rate if none given	
	if (rate_str) clock_rate = atoi(rate_str);
	else clock_rate = 8000;

	// Use default number of channels if none given	
	if (chan_str) channels = atoi(chan_str);
	else channels = -1;

	//printf("Searching for payload %s at freq %i with %i channels\n",subtype,clock_rate,ch1annels);
	ret=rtp_profile_find_payload_number(profile,subtype,clock_rate,channels);
	ortp_free(subtype);
	return ret;
}

PayloadType * rtp_profile_find_payload(RtpProfile *prof,const char *mime,int rate,int channels)
{
	int i;
	i=rtp_profile_find_payload_number(prof,mime,rate,channels);
	if (i>=0) return rtp_profile_get_payload(prof,i);
	return NULL;
}


PayloadType * rtp_profile_get_payload_from_mime(RtpProfile *profile,const char *mime)
{
	int pt;
	pt=rtp_profile_get_payload_number_from_mime(profile,mime);
	if (pt==-1) return NULL;
	else return rtp_profile_get_payload(profile,pt);
}


PayloadType * rtp_profile_get_payload_from_rtpmap(RtpProfile *profile, const char *rtpmap)
{
	int pt = rtp_profile_get_payload_number_from_rtpmap(profile,rtpmap);
	if (pt==-1) return NULL;
	else return rtp_profile_get_payload(profile,pt);
}

int rtp_profile_move_payload(RtpProfile *prof,int oldpos,int newpos){
	prof->payload[newpos]=prof->payload[oldpos];
	prof->payload[oldpos]=NULL;
	return 0;
}

RtpProfile * rtp_profile_new(const char *name)
{
	RtpProfile *prof=(RtpProfile*)ortp_new0(RtpProfile,1);
	rtp_profile_set_name(prof,name);
	return prof;
}

/**
 *	Assign payload type number index to payload type desribed in pt for the RTP profile profile.
 * @param profile a RTP profile
 * @param idx the payload type number
 * @param pt the payload type description
 *
**/
void rtp_profile_set_payload(RtpProfile *prof, int idx, PayloadType *pt){
	if (idx<0 || idx>=RTP_PROFILE_MAX_PAYLOADS) {
		ortp_error("Bad index %i",idx);
		return;
	}
	prof->payload[idx]=pt;
}

/**
 * Initialize the profile to the empty profile (all payload type are unassigned).
 *@param profile a RTP profile
 *
**/
void rtp_profile_clear_all(RtpProfile *obj){
	int i;
	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++){
		obj->payload[i]=0;
	}
}


/**
 * Set a name to the rtp profile. (This is not required)
 * @param profile a rtp profile object
 * @param nm a string
 *
**/
void rtp_profile_set_name(RtpProfile *obj, const char *name){
	if (obj->name!=NULL) ortp_free(obj->name);
	obj->name=ortp_strdup(name);
}

/* ! payload are not cloned*/
RtpProfile * rtp_profile_clone(RtpProfile *prof)
{
	int i;
	PayloadType *pt;
	RtpProfile *newprof=rtp_profile_new(prof->name);
	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++){
		pt=rtp_profile_get_payload(prof,i);
		if (pt!=NULL){
			rtp_profile_set_payload(newprof,i,pt);
		}
	}
	return newprof;
}


/*clone a profile and its payloads */
RtpProfile * rtp_profile_clone_full(RtpProfile *prof)
{
	int i;
	PayloadType *pt;
	RtpProfile *newprof=rtp_profile_new(prof->name);
	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++){
		pt=rtp_profile_get_payload(prof,i);
		if (pt!=NULL){
			rtp_profile_set_payload(newprof,i,payload_type_clone(pt));
		}
	}
	return newprof;
}

void rtp_profile_destroy(RtpProfile *prof)
{
	int i;
	PayloadType *payload;
	if (prof->name) {
		ortp_free(prof->name);
		prof->name = NULL;
	}
	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++)
	{
		payload=rtp_profile_get_payload(prof,i);
		if (payload!=NULL && (payload->flags & PAYLOAD_TYPE_ALLOCATED))
			payload_type_destroy(payload);
	}
	ortp_free(prof);
}
