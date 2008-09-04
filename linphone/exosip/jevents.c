/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include "eXosip2.h"
#include <eXosip.h>
#include <osip2/osip_condv.h>

extern eXosip_t eXosip;


static void fill_dialog_params(eXosip_event_t *je, osip_dialog_t *dialog)
{
	char *tmp;
	if (dialog->remote_uri!=NULL){
		osip_to_to_str(dialog->remote_uri, &tmp);
		if (tmp!=NULL)
		{
		  snprintf(je->remote_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	}
	if (dialog->local_uri!=NULL)
	{
		osip_to_to_str(dialog->local_uri, &tmp);
		if (tmp!=NULL)
		{
		  snprintf(je->local_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	}
	if (dialog->remote_contact_uri!=NULL){
	  osip_contact_to_str(dialog->remote_contact_uri,&tmp);
	  if (tmp!=NULL){
		  snprintf(je->remote_contact,255,"%s",tmp);
		  osip_free(tmp);
	  }			  
	}
}

eXosip_event_t *
eXosip_event_init_for_call(int type,
			   eXosip_call_t *jc,
			   eXosip_dialog_t *jd)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->jc = jc;
  je->jd = jd;

  je->cid = jc->c_id;
  if (jd!=NULL)
    je->did = jd->d_id;

  je->external_reference = jc->external_reference;

  /* fill in usefull info */
  if (type==EXOSIP_CALL_NEW
      || type==EXOSIP_CALL_ACK
      || type==EXOSIP_CALL_NOANSWER
      || type==EXOSIP_CALL_PROCEEDING
      || type==EXOSIP_CALL_RINGING
      || type==EXOSIP_CALL_ANSWERED
      || type==EXOSIP_CALL_REDIRECTED
      || type==EXOSIP_CALL_REQUESTFAILURE
      || type==EXOSIP_CALL_SERVERFAILURE
      || type==EXOSIP_CALL_GLOBALFAILURE

      || type==EXOSIP_OPTIONS_NOANSWER
      || type==EXOSIP_OPTIONS_PROCEEDING
      || type==EXOSIP_OPTIONS_ANSWERED
      || type==EXOSIP_OPTIONS_REDIRECTED
      || type==EXOSIP_OPTIONS_REQUESTFAILURE
      || type==EXOSIP_OPTIONS_SERVERFAILURE
      || type==EXOSIP_OPTIONS_GLOBALFAILURE
      || type==EXOSIP_OPTIONS_NEW

      || type==EXOSIP_INFO_NOANSWER
      || type==EXOSIP_INFO_PROCEEDING
      || type==EXOSIP_INFO_ANSWERED
      || type==EXOSIP_INFO_REDIRECTED
      || type==EXOSIP_INFO_REQUESTFAILURE
      || type==EXOSIP_INFO_SERVERFAILURE
      || type==EXOSIP_INFO_GLOBALFAILURE
      || type==EXOSIP_INFO_NEW

      || type==EXOSIP_CALL_CANCELLED
      || type==EXOSIP_CALL_TIMEOUT
      || type==EXOSIP_CALL_HOLD
      || type==EXOSIP_CALL_OFFHOLD
      || type==EXOSIP_CALL_CLOSED
      || type==EXOSIP_CALL_STARTAUDIO

      || type==EXOSIP_CALL_REFERED
      || type==EXOSIP_CALL_REFER_STATUS

      || type==EXOSIP_CALL_RELEASED)
    {
      if (jc->c_sdp_port[0]!='\0')
	je->local_sdp_audio_port = osip_atoi(jc->c_sdp_port);

      if (jd!=NULL&&jd->d_dialog!=NULL)
	{
	  osip_transaction_t *tr;
	  osip_header_t *subject;
	  char *tmp;
	  
	  fill_dialog_params(je,jd->d_dialog);

	  if (type==EXOSIP_OPTIONS_NOANSWER
	      || type==EXOSIP_OPTIONS_PROCEEDING
	      || type==EXOSIP_OPTIONS_ANSWERED
	      || type==EXOSIP_OPTIONS_REDIRECTED
	      || type==EXOSIP_OPTIONS_REQUESTFAILURE
	      || type==EXOSIP_OPTIONS_SERVERFAILURE
	      || type==EXOSIP_OPTIONS_GLOBALFAILURE
	      || type==EXOSIP_OPTIONS_NEW)
	    tr = eXosip_find_last_options(jc, jd);
	  else if (type==EXOSIP_INFO_NOANSWER
		   || type==EXOSIP_INFO_PROCEEDING
		   || type==EXOSIP_INFO_ANSWERED
		   || type==EXOSIP_INFO_REDIRECTED
		   || type==EXOSIP_INFO_REQUESTFAILURE
		   || type==EXOSIP_INFO_SERVERFAILURE
		   || type==EXOSIP_INFO_GLOBALFAILURE
		   || type==EXOSIP_INFO_NEW)
	    tr = eXosip_find_last_info(jc, jd);
	  else if (type==EXOSIP_CALL_REFERED)
	    tr = eXosip_find_last_refer(jc, jd);
	  else if (type==EXOSIP_CALL_REFER_STATUS)
	    tr = eXosip_find_last_inc_notify_for_refer(jc, jd);
	  else
	    tr = eXosip_find_last_invite(jc, jd);
	  if (tr!=NULL && tr->orig_request!=NULL)
	    {
	      osip_message_get_subject(tr->orig_request, 0, &subject);
	      if (subject!=NULL && subject->hvalue!=NULL && subject->hvalue[0]!='\0')
		snprintf(je->subject, 255, "%s", subject->hvalue);
	      osip_message_header_get_byname(tr->orig_request, "refer-to", 0,
					     &subject);
	      if (subject!=NULL && subject->hvalue!=NULL && subject->hvalue[0]!='\0')
		snprintf(je->refer_to, 255, "%s", subject->hvalue);

	      osip_uri_to_str(tr->orig_request->req_uri, &tmp);
	      if (tmp!=NULL)
		{
		  snprintf(je->req_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	    }
	  
	}else{
		osip_transaction_t *tr;
		char *tmp;
		tr=eXosip_find_last_invite(jc,NULL);
		/* no dialog established but we need to fill remote_contact */
		if (tr!=NULL && tr->last_response!=NULL)
		{
			osip_contact_t *ctt=NULL;
			snprintf(je->reason_phrase, 49, "%s", tr->last_response->reason_phrase);
			je->status_code = tr->last_response->status_code;
			osip_message_get_contact(tr->last_response,0,&ctt);
			if (ctt){
				tmp=NULL;
				osip_contact_to_str(ctt,&tmp);
				if (tmp){
					strncpy(je->remote_contact,tmp,255);
				}
			}
		}
	}
    }
  
  return je;
}

int
eXosip_event_add_status(eXosip_event_t *je, osip_message_t *response)
{
  if (response!=NULL && response->reason_phrase!=NULL)
    {
      snprintf(je->reason_phrase, 49, "%s", response->reason_phrase);
      je->status_code = response->status_code;
    }
  return 0;
}

int
eXosip_event_add_sdp_info(eXosip_event_t *je, osip_message_t *message)
{
  osip_content_type_t *ctt;
  osip_mime_version_t *mv;
  sdp_message_t *sdp;
  osip_body_t *oldbody;
  int pos;
  int gotpayload = 0;



  /* 
     search for remote_sdp_audio_port & remote_sdp_audio_ip
     in the last SIP message 
     extract the payload to be used for connection either from negotioation context
     or from the last SIP message
  */

  

  if (message==NULL) return -1;

  /* get content-type info */
  ctt = osip_message_get_content_type(message);
  mv  = osip_message_get_mime_version(message);
  if (mv==NULL && ctt==NULL)
    return 0; /* previous message was not correct or empty */
  if (mv!=NULL)
    {
      /* look for the SDP body */
      /* ... */
    }
  else if (ctt!=NULL)
    {
      if (ctt->type==NULL || ctt->subtype==NULL)
	/* it can be application/sdp or mime... */
	return -1;
      if (osip_strcasecmp(ctt->type, "application")!=0 ||
	  osip_strcasecmp(ctt->subtype, "sdp")!=0 )
	{ return -1; }
    }
  

  if (je->jc)
    {
      int pl = eXosip_retrieve_sdp_negotiation_result(je->jc->c_ctx, je->payload_name, sizeof(je->payload_name));
      if (pl >= 0)
	{
	  je->payload = pl;
	  gotpayload = 1;
	}
    }


  sdp = NULL;
  pos = 0;
  while (!osip_list_eol(message->bodies, pos))
    {
      int i;
      oldbody = (osip_body_t *)osip_list_get(message->bodies, pos);
      pos++;
      sdp_message_init(&sdp);
      i = sdp_message_parse(sdp,oldbody->body);
      if (i==0)
	{
	  int len = strlen(oldbody->body);
	  if (len<999)
	    osip_strncpy(je->sdp_body, oldbody->body, len);
	  else
	    osip_strncpy(je->sdp_body, oldbody->body, 999);
	  
	  break;
	}
      sdp_message_free(sdp);
      sdp = NULL;
    }



  if (sdp!=NULL)
    {
      int j=0;
      if (sdp->c_connection !=NULL
	  && sdp->c_connection->c_addr !=NULL )
	{
	  snprintf(je->remote_sdp_audio_ip, 49, "%s",
		   sdp->c_connection->c_addr);
	}


      for (j=0; !osip_list_eol(sdp->m_medias, j); j++)
	{
	  sdp_media_t *med = (sdp_media_t*) osip_list_get(sdp->m_medias, j);
	  if (med==NULL)
	    {
	      snprintf(je->remote_sdp_audio_ip, 49, "Y a probleme!");
	    }
	
	  if (med->m_media!=NULL &&
	      0==osip_strcasecmp(med->m_media, "audio"))
	    {
	      sdp_connection_t *conn;
	      int pos_attr;
	      char *payload = (char *) osip_list_get (med->m_payloads, 0);

	      if (!gotpayload)
		je->payload = 0;

	      if (!gotpayload && payload!=NULL)
		{
		  je->payload = osip_atoi(payload);
		  /* copy payload name! */
		  for (pos_attr=0;
		       !osip_list_eol(med->a_attributes, pos_attr);
		       pos_attr++)
		    {
		      sdp_attribute_t *attr;
		      attr = (sdp_attribute_t *)osip_list_get(med->a_attributes, pos_attr);
		      if (0==osip_strncasecmp(attr->a_att_field, "rtpmap", 6))
			{
			  if ((je->payload<10 && 
			       0==osip_strncasecmp(attr->a_att_value, payload, 1))
			      ||(je->payload>9 && je->payload<100 && 
				 0==osip_strncasecmp(attr->a_att_value, payload, 2))
			      ||(je->payload>100 && je->payload<128 &&
				 0==osip_strncasecmp(attr->a_att_value, payload, 3)))
			    {
			      snprintf(je->payload_name, 49, "%s", attr->a_att_value);
			    }
			}
		    }
		}
	      

	      je->remote_sdp_audio_port = osip_atoi(med->m_port);
	      conn = (sdp_connection_t*) osip_list_get(med->c_connections, 0);
	      if (conn!=NULL && conn->c_addr!=NULL)
		{
		  snprintf(je->remote_sdp_audio_ip, 49, "%s",
			   conn->c_addr);
		}
	      sdp_message_free(sdp);
	      return 0;
	    }
	}
      sdp_message_free(sdp);
    }
  return -1;
}


eXosip_event_t *
eXosip_event_init_for_subscribe(int type,
				eXosip_subscribe_t *js,
				eXosip_dialog_t *jd)
{
  char *tmp;
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->js = js;
  je->jd = jd;

  je->sid = js->s_id;
  if (jd!=NULL)
    je->did = jd->d_id;

  je->ss_status = js->s_ss_status;
  je->online_status = js->s_online_status;
  je->ss_reason = js->s_ss_reason;

  /* je->external_reference = js->external_reference; */

  if (jd!=NULL&&jd->d_dialog!=NULL)
    {
      fill_dialog_params(je,jd->d_dialog);
    }

  /* fill in usefull info */
  if (type==EXOSIP_SUBSCRIPTION_NEW
      || type==EXOSIP_SUBSCRIPTION_NOANSWER
      || type==EXOSIP_SUBSCRIPTION_PROCEEDING
      || type==EXOSIP_SUBSCRIPTION_ANSWERED
      || type==EXOSIP_SUBSCRIPTION_REDIRECTED
      || type==EXOSIP_SUBSCRIPTION_REQUESTFAILURE
      || type==EXOSIP_SUBSCRIPTION_SERVERFAILURE
      || type==EXOSIP_SUBSCRIPTION_GLOBALFAILURE
      || type==EXOSIP_SUBSCRIPTION_RELEASED)
    {
      if (jd!=NULL&&jd->d_dialog!=NULL)
	{
	  osip_transaction_t *tr;
	  tr = eXosip_find_last_out_subscribe(js, jd);
	  if (tr!=NULL && tr->orig_request!=NULL)
	    {
	      osip_uri_to_str(tr->orig_request->req_uri, &tmp);
	      if (tmp!=NULL)
		{
		  snprintf(je->req_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	    }
	  if (tr!=NULL && tr->last_response!=NULL)
	    {
	      snprintf(je->reason_phrase, 49, "%s", tr->last_response->reason_phrase);
	      je->status_code = tr->last_response->status_code;
	    }
	}
    }
  else if (type==EXOSIP_SUBSCRIPTION_NOTIFY)
    {
      if (jd!=NULL&&jd->d_dialog!=NULL)
	{
	  osip_transaction_t *tr;
	  tr = eXosip_find_last_inc_notify(js, jd);
	  if (tr!=NULL && tr->orig_request!=NULL)
	    {
	      osip_uri_to_str(tr->orig_request->req_uri, &tmp);
	      if (tmp!=NULL)
		{
		  snprintf(je->req_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	    }
	  if (tr!=NULL && tr->last_response!=NULL)
	    {
	      snprintf(je->reason_phrase, 49, "%s", tr->last_response->reason_phrase);
	      je->status_code = tr->last_response->status_code;
	    }
	}
    }
  
  return je;
}

eXosip_event_t *
eXosip_event_init_for_notify(int type,
			     eXosip_notify_t *jn,
			     eXosip_dialog_t *jd)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->jn = jn;
  je->jd = jd;

  je->nid = jn->n_id;
  if (jd!=NULL)
    je->did = jd->d_id;

  je->ss_status = jn->n_ss_status;
  je->online_status = jn->n_online_status;
  je->ss_reason = jn->n_ss_reason;

  /*je->external_reference = jc->external_reference; */

  /* fill in usefull info */
  if (type==EXOSIP_IN_SUBSCRIPTION_NEW
      || type==EXOSIP_IN_SUBSCRIPTION_RELEASED)
    {
      if (jd!=NULL&&jd->d_dialog!=NULL)
	{
	  osip_transaction_t *tr;
	  char *tmp;
	  fill_dialog_params(je,jd->d_dialog);
	  tr = eXosip_find_last_inc_subscribe(jn, jd);
	  if (tr!=NULL && tr->orig_request!=NULL)
	    {
	      osip_uri_to_str(tr->orig_request->req_uri, &tmp);
	      if (tmp!=NULL)
		{
		  snprintf(je->req_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	    }
	  if (tr!=NULL && tr->last_response!=NULL)
	    {
	      snprintf(je->reason_phrase, 49, "%s", tr->last_response->reason_phrase);
	      je->status_code = tr->last_response->status_code;
	    }
	}
    }
  
  return je;
}

eXosip_event_t *
eXosip_event_init_for_reg(int type,
			  eXosip_reg_t *jr)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->jr = jr;
  je->rid = jr->r_id;
  snprintf(je->remote_uri, 255, "%s", jr->r_aor);  
  snprintf(je->req_uri, 255,    "%s", jr->r_registrar);
  return je;
}

eXosip_event_t *
eXosip_event_init_for_message(int type, osip_transaction_t *tr,
			      osip_message_t *sip)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  
  /* fill in usefull info */
  { 
    char *tmp;
    
    /* Request URI */
    
    osip_uri_to_str(sip->req_uri, &tmp);
    if (tmp!=NULL)
      {
	snprintf(je->req_uri, 255, "%s", tmp);
	osip_free(tmp);
      }
    
    /* FROM & TO */
    
    
    if (sip->from != NULL)
      {
	osip_from_to_str(sip->from, &tmp);
	if (tmp!=NULL)
	  {
	    snprintf(je->remote_uri, 255, "%s", tmp);
	    osip_free(tmp);
	  }
      }
    if (sip->to != NULL)
      {
	osip_to_to_str(sip->to, &tmp);
	if (tmp!=NULL)
	  {
	    snprintf(je->local_uri, 255, "%s", tmp);
	    osip_free(tmp);
	  }
      }
  }
  
  return je;
}

int
eXosip_event_init(eXosip_event_t **je, int type)
{
  *je = (eXosip_event_t *) osip_malloc(sizeof(eXosip_event_t));
  if (*je==NULL) return -1;
  
  memset(*je, 0, sizeof(eXosip_event_t));
  (*je)->type = type;

  if (type==EXOSIP_CALL_NOANSWER)
    {
      sprintf((*je)->textinfo, "No answer for this Call!");
    }
  else if (type==EXOSIP_CALL_PROCEEDING)
    {
      sprintf((*je)->textinfo, "Call is being processed!");
    }
  else if (type==EXOSIP_CALL_RINGING)
    {
      sprintf((*je)->textinfo, "Remote phone is ringing!");
    }
  else if (type==EXOSIP_CALL_ANSWERED)
    {
      sprintf((*je)->textinfo, "Remote phone has answered!");
    }
  else if (type==EXOSIP_CALL_REDIRECTED)
    {
      sprintf((*je)->textinfo, "Call is redirected!");
    }
  else if (type==EXOSIP_CALL_REQUESTFAILURE)
    {
      sprintf((*je)->textinfo, "4xx received for Call!");
    }
  else if (type==EXOSIP_CALL_SERVERFAILURE)
    {
      sprintf((*je)->textinfo, "5xx received for Call!");
    }
  else if (type==EXOSIP_CALL_GLOBALFAILURE)
    {
      sprintf((*je)->textinfo, "6xx received for Call!");
    }
  else if (type==EXOSIP_CALL_NEW)
    {
      sprintf((*je)->textinfo, "New call received!");
    }
  else if (type==EXOSIP_CALL_ACK)
    {
      sprintf((*je)->textinfo, "ACK received!");
    }
  else if (type==EXOSIP_CALL_CANCELLED)
    {
      sprintf((*je)->textinfo, "Call has been cancelled!");
    }
  else if (type==EXOSIP_CALL_TIMEOUT)
    {
      sprintf((*je)->textinfo, "Timeout. Gived up!");
    }
  else if (type==EXOSIP_CALL_HOLD)
    {
      sprintf((*je)->textinfo, "Call is on Hold!");
    }
  else if (type==EXOSIP_CALL_OFFHOLD)
    {
      sprintf((*je)->textinfo, "Call is off Hold!");
    }
  else if (type==EXOSIP_CALL_CLOSED)
    {
      sprintf((*je)->textinfo, "Bye Received!");
    }
  else if (type==EXOSIP_CALL_RELEASED)
    {
      sprintf((*je)->textinfo, "Call Context is released!");
    }
  else if (type==EXOSIP_REGISTRATION_SUCCESS)
    {
      sprintf((*je)->textinfo, "User is successfully registred!");
    }
  else if (type==EXOSIP_REGISTRATION_FAILURE)
    {
      sprintf((*je)->textinfo, "Registration failed!");
    }
  else if (type==EXOSIP_OPTIONS_NEW)
    {
      sprintf((*je)->textinfo, "New OPTIONS received!");
    }
  else if (type==EXOSIP_OPTIONS_NOANSWER)
    {
      sprintf((*je)->textinfo, "No answer for this OPTIONS!");
    }
  else if (type==EXOSIP_OPTIONS_PROCEEDING)
    {
      sprintf((*je)->textinfo, "OPTIONS is being processed!");
    }
  else if (type==EXOSIP_OPTIONS_ANSWERED)
    {
      sprintf((*je)->textinfo, "2xx received for OPTIONS!");
    }
  else if (type==EXOSIP_OPTIONS_REDIRECTED)
    {
      sprintf((*je)->textinfo, "3xx received for OPTIONS!");
    }
  else if (type==EXOSIP_OPTIONS_REQUESTFAILURE)
    {
      sprintf((*je)->textinfo, "4xx received for OPTIONS!");
    }
  else if (type==EXOSIP_OPTIONS_SERVERFAILURE)
    {
      sprintf((*je)->textinfo, "5xx received for OPTIONS!");
    }
  else if (type==EXOSIP_OPTIONS_GLOBALFAILURE)
    {
      sprintf((*je)->textinfo, "5xx received for OPTIONS!");
    }
  else if (type==EXOSIP_INFO_NEW)
    {
      sprintf((*je)->textinfo, "New INFO received!");
    }
  else if (type==EXOSIP_INFO_NOANSWER)
    {
      sprintf((*je)->textinfo, "No answer for this INFO!");
    }
  else if (type==EXOSIP_INFO_PROCEEDING)
    {
      sprintf((*je)->textinfo, "INFO is being processed!");
    }
  else if (type==EXOSIP_INFO_ANSWERED)
    {
      sprintf((*je)->textinfo, "2xx received for INFO!");
    }
  else if (type==EXOSIP_INFO_REDIRECTED)
    {
      sprintf((*je)->textinfo, "3xx received for INFO!");
    }
  else if (type==EXOSIP_INFO_REQUESTFAILURE)
    {
      sprintf((*je)->textinfo, "4xx received for INFO!");
    }
  else if (type==EXOSIP_INFO_SERVERFAILURE)
    {
      sprintf((*je)->textinfo, "5xx received for INFO!");
    }
  else if (type==EXOSIP_INFO_GLOBALFAILURE)
    {
      sprintf((*je)->textinfo, "6xx received for INFO!");
    }
  else if (type==EXOSIP_MESSAGE_NEW)
    {
      sprintf((*je)->textinfo, "New MESSAGE received!");
    }
  else if (type==EXOSIP_MESSAGE_SUCCESS)
    {
      sprintf((*je)->textinfo, "User has successfully received our MESSAGE!");
    }
  else if (type==EXOSIP_MESSAGE_FAILURE)
    {
      sprintf((*je)->textinfo, "Error received for our MESSAGE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_NEW)
    {
      sprintf((*je)->textinfo, "New SUBSCRIBE received!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_NOANSWER)
    {
      sprintf((*je)->textinfo, "No answer for this SUBSCRIBE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_PROCEEDING)
    {
      sprintf((*je)->textinfo, "SUBSCRIBE is being processed!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_ANSWERED)
    {
      sprintf((*je)->textinfo, "2xx received for SUBSCRIBE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_REDIRECTED)
    {
      sprintf((*je)->textinfo, "3xx received for SUBSCRIBE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_REQUESTFAILURE)
    {
      sprintf((*je)->textinfo, "4xx received for SUBSCRIBE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_SERVERFAILURE)
    {
      sprintf((*je)->textinfo, "5xx received for SUBSCRIBE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_GLOBALFAILURE)
    {
      sprintf((*je)->textinfo, "5xx received for SUBSCRIBE!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_NOTIFY)
    {
      sprintf((*je)->textinfo, "NOTIFY request for subscription!");
    }
  else if (type==EXOSIP_SUBSCRIPTION_RELEASED)
    {
      sprintf((*je)->textinfo, "Subscription has terminate!");
    }
  else if (type==EXOSIP_IN_SUBSCRIPTION_NEW)
    {
      sprintf((*je)->textinfo, "New incoming SUBSCRIBE!");
    }
  else if (type==EXOSIP_IN_SUBSCRIPTION_RELEASED)
    {
      sprintf((*je)->textinfo, "Incoming Subscription has terminate!");
    }
  else
    {
      (*je)->textinfo[0] = '\0';
    }
  return 0;
}

void
eXosip_event_free(eXosip_event_t *je)
{
  if (je==NULL) return;
  if (je->i_ctt!=NULL)
    osip_content_type_free(je->i_ctt);
  if (je->i_bodies!=NULL)
    {
      int pos;
      for (pos=0;!osip_list_eol(je->i_bodies, pos);)
	{
	  osip_body_t *body;
	  body = (osip_body_t *)osip_list_get(je->i_bodies, pos);
	  osip_list_remove(je->i_bodies, pos);
	  osip_body_free(body);
	}
    }
  osip_free(je);
}

eXosip_call_t *
eXosip_event_get_callinfo(eXosip_event_t *je)
{
  return je->jc;
}

eXosip_dialog_t *
eXosip_event_get_dialoginfo(eXosip_event_t *je)
{
  return je->jd;
}

eXosip_reg_t *
eXosip_event_get_reginfo(eXosip_event_t *je)
{
  return je->jr;
}

eXosip_notify_t *
eXosip_event_get_notifyinfo(eXosip_event_t *je)
{
  return je->jn;
}

eXosip_subscribe_t *
eXosip_event_get_subscribeinfo(eXosip_event_t *je)
{
  return je->js;
}

int
eXosip_event_add(eXosip_event_t *je)
{
  int i = osip_fifo_add(eXosip.j_events, (void *) je);
  osip_cond_signal((struct osip_cond *)eXosip.j_cond);
  __eXosip_wakeup_event();
  return i;
}

#if 0
#ifdef CLOCK_REALTIME
/* if CLOCK_REALTIME exist, then clock_gettime should be defined */

#define OSIP_CLOCK_REALTIME CLOCK_REALTIME

void
__eXosip_clock_gettime(clockid_t cid, struct timespec *time)
{
  clock_gettime(cid, time);
}

#elif defined (WIN32) || defined (_WIN32_WCE)

#include <sys/types.h>
#include <sys/timeb.h>

#define OSIP_CLOCK_REALTIME 4002

void
__eXosip_clock_gettime(unsigned int clock_id, struct timespec *time)
{
   struct _timeb time_val;

   if (clock_id != OSIP_CLOCK_REALTIME)
     return;

   _ftime (&time_val);
   time->tv_sec = time_val.time;
   time->tv_nsec = time_val.millitm * 1000000;
   return;
}
#endif
#endif

eXosip_event_t *
eXosip_event_wait(int tv_s, int tv_ms)
{
  eXosip_event_t *je = NULL;

#if 0 /* this does not seems to work. by now */
#if defined (CLOCK_REALTIME) || defined (WIN32) || defined (_WIN32_WCE)
  int i;

  struct timespec deadline;
  struct timespec interval;
  long tot_ms = (tv_s*1000) + tv_ms;
  
  static struct osip_mutex *mlock = NULL;

  if (mlock==NULL) mlock = osip_mutex_init();

  je = (eXosip_event_t *) osip_fifo_tryget(eXosip.j_events);
  if(je) return je;

  interval.tv_sec = tot_ms / 1000;
  interval.tv_nsec = (tot_ms % 1000) * 1000000L;
  
  __eXosip_clock_gettime(OSIP_CLOCK_REALTIME, &deadline);
  
  if ((deadline.tv_nsec += interval.tv_nsec) >= 1000000000L)
    {
      deadline.tv_nsec -= 1000000000L;
      deadline.tv_sec += 1;
    }
  else 
    deadline.tv_nsec += interval.tv_nsec;
  
  deadline.tv_sec += interval.tv_sec;
  
  i = osip_cond_timedwait ((struct osip_cond *)eXosip.j_cond,
			   (struct osip_mutex *)mlock,
			   &deadline);
  
#endif
#else
  /* basic replacement */
    {
	  fd_set fdset;
	  struct timeval tv;
	  int max, i;
      FD_ZERO(&fdset);
#if defined (WIN32) || defined (_WIN32_WCE)
      FD_SET((unsigned int)jpipe_get_read_descr(eXosip.j_socketctl_event), &fdset);
#else
      FD_SET(jpipe_get_read_descr(eXosip.j_socketctl_event), &fdset);
#endif
      max = jpipe_get_read_descr(eXosip.j_socketctl_event);
	  tv.tv_sec = tv_s;
	  tv.tv_usec = tv_ms*1000;

      je = (eXosip_event_t *) osip_fifo_tryget(eXosip.j_events);
      if (je!=NULL) return je;

      if (tv_s==0 && tv_ms==0)
		return NULL;

      i = select(max+1, &fdset, NULL, NULL, &tv);
      if (i <= 0)
	return 0;

      if (FD_ISSET (jpipe_get_read_descr(eXosip.j_socketctl_event), &fdset))
	  {
		  char buf[500];
		  jpipe_read (eXosip.j_socketctl_event, buf, 499);
	  }

      je = (eXosip_event_t *) osip_fifo_tryget(eXosip.j_events);
      if (je!=NULL) return je;
	}
#endif

  return je;
}

eXosip_event_t *
eXosip_event_get()
{
  eXosip_event_t *je;
  je = (eXosip_event_t *) osip_fifo_get(eXosip.j_events);
  return je;
}
