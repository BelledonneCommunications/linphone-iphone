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
#include <eXosip_cfg.h>

extern eXosip_t eXosip;

/* Private functions */
static char *generating_no_sdp_answer(eXosip_call_t *jc, eXosip_dialog_t *jd,
			 osip_message_t *orig_request, char *local_sdp_port);

int
eXosip_build_response_default(int jid, int status)
{
  return -1;
}

int
_eXosip_build_response_default(osip_message_t **dest, osip_dialog_t *dialog,
			       int status, osip_message_t *request)
{
  osip_generic_param_t *tag;
  osip_message_t *response;
  int pos;
  int i;

  if (request==NULL) return -1;

  i = osip_message_init(&response);
  if (i!=0)
    return -1;
  /* initialise osip_message_t structure */
  /* yet done... */

  response->sip_version = (char *)osip_malloc(8*sizeof(char));
  sprintf(response->sip_version,"SIP/2.0");
  osip_message_set_status_code(response, status);

  /* handle some internal reason definitions. */
  if (MSG_IS_NOTIFY(request) && status==481)
    {
      response->reason_phrase = osip_strdup("Subcription Does Not Exist");
    }
  else if (MSG_IS_SUBSCRIBE(request) && status==202)
    {
      response->reason_phrase = osip_strdup("Accepted subscription");
    }
  else
    {
      response->reason_phrase = osip_strdup(osip_message_get_reason(status));
      if (response->reason_phrase==NULL)
	{
	  if (response->status_code == 101)
	    response->reason_phrase = osip_strdup("Dialog Establishement");
	  else
	    response->reason_phrase = osip_strdup("Unknown code");
	}
      response->req_uri     = NULL;
      response->sip_method = NULL;
    }

  i = osip_to_clone(request->to, &(response->to));
  if (i!=0) goto grd_error_1;

  i = osip_to_get_tag(response->to,&tag);
  if (i!=0)
    {  /* we only add a tag if it does not already contains one! */
      if ((dialog!=NULL) && (dialog->local_tag!=NULL))
	/* it should contain the local TAG we created */
	{ osip_to_set_tag(response->to, osip_strdup(dialog->local_tag)); }
      else
	{
	  if (status!=100)
	    osip_to_set_tag(response->to, osip_to_tag_new_random());
	}
    }

  i = osip_from_clone(request->from, &(response->from));
  if (i!=0) goto grd_error_1;

  pos = 0;
  while (!osip_list_eol(request->vias,pos))
    {
      osip_via_t *via;
      osip_via_t *via2;
      via = (osip_via_t *)osip_list_get(request->vias,pos);
      i = osip_via_clone(via, &via2);
      if (i!=-0) goto grd_error_1;
      osip_list_add(response->vias, via2, -1);
      pos++;
    }

  i = osip_call_id_clone(request->call_id, &(response->call_id));
  if (i!=0) goto grd_error_1;
  i = osip_cseq_clone(request->cseq, &(response->cseq));
  if (i!=0) goto grd_error_1;

  if (MSG_IS_SUBSCRIBE(request))
    {
      osip_header_t *exp;
      osip_message_set_header(response, "Event", "presence");
      i = osip_message_get_expires(request, 0, &exp);
      if (exp==NULL)
	{
	  osip_header_t *cp;
	  i = osip_header_clone(exp, &cp);
	  if (cp!=NULL)
	    osip_list_add(response->headers, cp, 0);
	}
    }
    
  osip_message_set_allow(response, "INVITE");
  osip_message_set_allow(response, "ACK");
  osip_message_set_allow(response, "OPTIONS");
  osip_message_set_allow(response, "CANCEL");
  osip_message_set_allow(response, "BYE");
  osip_message_set_allow(response, "SUBSCRIBE");
  osip_message_set_allow(response, "NOTIFY");
  osip_message_set_allow(response, "MESSAGE");
  osip_message_set_allow(response, "INFO");

  *dest = response;
  return 0;

 grd_error_1:
  osip_message_free(response);
  return -1;
}

int
complete_answer_that_establish_a_dialog(osip_message_t *response, osip_message_t *request)
{
  int i;
  int pos=0;
  char contact[1000];
#ifdef SM
  char *locip=NULL;
#else
  char locip[50];
#endif
  /* 12.1.1:
     copy all record-route in response
     add a contact with global scope
  */
  while (!osip_list_eol(request->record_routes, pos))
    {
      osip_record_route_t *rr;
      osip_record_route_t *rr2;
      rr = osip_list_get(request->record_routes, pos);
      i = osip_record_route_clone(rr, &rr2);
      if (i!=0) return -1;
      osip_list_add(response->record_routes, rr2, -1);
      pos++;
    }

#ifdef SM
  eXosip_get_localip_from_via(response,&locip);
#else
  eXosip_guess_ip_for_via(eXosip.ip_family, locip, 49);
#endif
  
  if (eXosip.answer_contact[0])
    snprintf(contact,1000, "%s", eXosip.answer_contact);
  else if (request->to->url->username==NULL)
    snprintf(contact,1000, "<sip:%s:%s>", locip, eXosip.localport);
  else
    snprintf(contact,1000, "<sip:%s@%s:%s>", request->to->url->username,
	     locip, eXosip.localport);

  if (eXosip.j_firewall_ip[0]!='\0')
    {
      osip_contact_t *con = (osip_contact_t *) osip_list_get (request->contacts, 0);
      if (con!=NULL && con->url!=NULL && con->url->host!=NULL)
	{
	  char *c_address = con->url->host;

	  struct addrinfo *addrinfo;
	  struct __eXosip_sockaddr addr;
	  i = eXosip_get_addrinfo(&addrinfo, con->url->host, 5060);
	  if (i==0)
		{
		  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
		  freeaddrinfo (addrinfo);
		  c_address = inet_ntoa (((struct sockaddr_in *) &addr)->sin_addr);
		  OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "eXosip: here is the resolved destination host=%s\n", c_address));
		}

	  /* If c_address is a PUBLIC address, the request was
	     coming from the PUBLIC network. */
	  if (eXosip_is_public_address(c_address))
	    {
	      if (request->to->url->username==NULL)
		snprintf(contact,1000, "<sip:%s:%s>", eXosip.j_firewall_ip,
			 eXosip.localport);
	      else
		snprintf(contact,1000, "<sip:%s@%s:%s>", request->to->url->username,
			 eXosip.j_firewall_ip, eXosip.localport);
	    }
	}
    }

#ifdef SM
  osip_free(locip);
#endif

  osip_message_set_contact(response, contact);
	
  return 0;
}

static char *
generating_no_sdp_answer(eXosip_call_t *jc, eXosip_dialog_t *jd,
			 osip_message_t *orig_request, char *local_sdp_port)
{
  sdp_message_t *local_sdp = NULL;
  char *local_body = NULL;
  char *size;
  int i;
  
  jc->c_ack_sdp = 1;
  if(osip_negotiation_sdp_build_offer(eXosip.osip_negotiation, NULL, &local_sdp, local_sdp_port, NULL) != 0)
    return NULL;
  
  if (local_sdp!=NULL)
    {
      int pos=0;
      while (!sdp_message_endof_media (local_sdp, pos))
	{
	  int k = 0;
	  char *tmp = sdp_message_m_media_get (local_sdp, pos);
	  if (0 == strncmp (tmp, "audio", 5))
	    {
	      char *payload = NULL;
	      do {
		payload = sdp_message_m_payload_get (local_sdp, pos, k);
		if (payload == NULL)
		  {
		  }
		else if (0==strcmp("110",payload))
		  {
		    sdp_message_a_attribute_add (local_sdp,
						 pos,
						 osip_strdup ("AS"),
						 osip_strdup ("110 20"));
		  }
		else if (0==strcmp("111",payload))
		  {
		    sdp_message_a_attribute_add (local_sdp,
						 pos,
						 osip_strdup ("AS"),
						 osip_strdup ("111 20"));
		  }
		k++;
	      } while (payload != NULL);
	    }
	  pos++;
	}
    }
  
  i = sdp_message_to_str(local_sdp, &local_body);
  
  if (local_body!=NULL)
    {
      size= (char *)osip_malloc(7*sizeof(char));
#ifdef __APPLE_CC__
      sprintf(size,"%li",strlen(local_body));
#else
      sprintf(size,"%i",strlen(local_body));
#endif
      osip_message_set_content_length(orig_request, size);
      osip_free(size);
  
      osip_message_set_body(orig_request, local_body, strlen(local_body));
      osip_message_set_content_type(orig_request, "application/sdp");
    }
  else
    osip_message_set_content_length(orig_request, "0");
  
  osip_negotiation_ctx_set_local_sdp(jc->c_ctx, local_sdp);
  
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO3,NULL,"200 OK w/ SDP (RESPONSE TO INVITE w/ NO SDP)=\n%s\n", local_body));
  
  return local_body;
}

char *
generating_sdp_answer(osip_message_t *request, osip_negotiation_ctx_t *context)
{
  sdp_message_t *remote_sdp;
  sdp_message_t *local_sdp = NULL;
  int i;
  char *local_body;
  if (context==NULL)
    return NULL;

  local_body = NULL;
  if (MSG_IS_INVITE(request)||MSG_IS_OPTIONS(request)||MSG_IS_RESPONSE_FOR(request, "INVITE"))
    {
      osip_body_t *body;
      body = (osip_body_t *)osip_list_get(request->bodies,0);
      if(body == NULL)
	return NULL;
      
      /* remote_sdp = (sdp_message_t *) osip_malloc(sizeof(sdp_message_t)); */
      i = sdp_message_init(&remote_sdp);
      if (i!=0) return NULL;
      
      /* WE ASSUME IT IS A SDP BODY AND THAT    */
      /* IT IS THE ONLY ONE, OF COURSE, THIS IS */
      /* NOT TRUE */
      i = sdp_message_parse(remote_sdp,body->body);
      if (i!=0) return NULL;

      i = osip_negotiation_ctx_set_remote_sdp(context, remote_sdp);

      i = osip_negotiation_ctx_execute_negotiation(eXosip.osip_negotiation, context);
      if (i==200)
	{
	  local_sdp = osip_negotiation_ctx_get_local_sdp(context);

	  if (eXosip.j_firewall_ip[0]!='\0')
	  {
		  char *c_address = NULL;
		  int pos=0;
		  /* If remote message contains a Public IP, we have to replace the SDP
			connection address */
		  c_address = sdp_message_c_addr_get(remote_sdp, -1, 0);
		  while (c_address==NULL)
		  {
			  c_address = sdp_message_c_addr_get(remote_sdp, pos, 0);
			  pos++;
			  if (pos>10)
				  break;
		  }
		  if (c_address!=NULL) /* found a connection address: check if it is public */
		  {

			  struct addrinfo *addrinfo;
			  struct __eXosip_sockaddr addr;
			  i = eXosip_get_addrinfo(&addrinfo, c_address, 5060);
			  if (i==0)
				{
				  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
				  freeaddrinfo (addrinfo);
				  c_address = inet_ntoa (((struct sockaddr_in *) &addr)->sin_addr);
				  OSIP_TRACE (osip_trace
					  (__FILE__, __LINE__, OSIP_INFO1, NULL,
					  "eXosip: here is the resolved destination host=%s\n", c_address));
				}

			  if (eXosip_is_public_address(c_address))
			    {
				  /* replace the IP with our firewall ip */
				  sdp_connection_t *conn;
				  pos=-1;
				  conn = sdp_message_connection_get(local_sdp, pos, 0);
				  while (conn!=NULL)
				  {
					  if (conn->c_addr!=NULL )
					  {
						  osip_free(conn->c_addr);
						  conn->c_addr = osip_strdup(eXosip.j_firewall_ip);
					  }
					  pos++;
					  conn = sdp_message_connection_get(local_sdp, pos, 0);
				  }
			    }
		  }
	  }

	  i = sdp_message_to_str(local_sdp, &local_body);

	  remote_sdp = osip_negotiation_ctx_get_remote_sdp(context);
	  sdp_message_free(remote_sdp);
	  osip_negotiation_ctx_set_remote_sdp(context, NULL);

	  if (i!=0) {
	    OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not parse local SDP answer %i\n",i));
	    return NULL;
	  }
	  return local_body;
	}
      else if (i==415)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"WARNING: Unsupported media %i\n",i));
	}
      else
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: while building answer to SDP (%i)\n",i));
	}
      remote_sdp = osip_negotiation_ctx_get_remote_sdp(context);
      sdp_message_free(remote_sdp);
      osip_negotiation_ctx_set_remote_sdp(context, NULL);
    }
  return NULL;
}

int
eXosip_answer_options_1xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_transaction_t *tr;
  osip_message_t *response;
  int i;

  tr = eXosip_find_last_inc_options(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }

  if (jd!=NULL)
    {
      i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);
    }
  else
    {
      i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
    }

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for OPTIONS\n"));
      return -1;
    }

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  return 0;
}

int
eXosip_answer_options_2xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_transaction_t *tr;
  osip_message_t *response;
  sdp_message_t *sdp;
  char *body;
  char size[10];
  int i;

  tr = eXosip_find_last_inc_options(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }
  osip_negotiation_sdp_build_offer(eXosip.osip_negotiation, NULL, &sdp, "10400", NULL);
  if (sdp==NULL)
    {
      return -1;
    }
  if (jd!=NULL)
    {
      i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);
    }
  else
    {
      i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
    }
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for options\n"));
      sdp_message_free(sdp); /* not used */
      return -1;
    }
  i = sdp_message_to_str(sdp, &body);
  sdp_message_free(sdp);
  if (i!=0) {
    osip_message_free(response);
    return -1;
  }
  i = osip_message_set_body(response, body, strlen(body));
  if (i!=0) {
    osip_message_free(response);
    return -1;
  }
#ifdef __APPLE_CC__
  snprintf(size, 9,"%li",strlen(body));
#else
  snprintf(size, 9,"%i",strlen(body));
#endif
  i = osip_message_set_content_length(response, size);
  if (i!=0) {
    osip_free(body);
    osip_message_free(response);
    return -1;
  }
  osip_free(body);
  i = osip_message_set_content_type(response, "application/sdp");
  if (i!=0) {
    osip_message_free(response);
    return -1;
  }

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  return 0;
}

int
eXosip_answer_options_3456xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_transaction_t *tr;
  osip_message_t *response;
  int i;
  tr = eXosip_find_last_inc_options(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }

  if (jd!=NULL)
    {
      i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);
    }
  else
    {
      i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
    }
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for options\n"));
      return -1;
    }

  if (300<=code<=399)
    {
      /* Should add contact fields */
      /* ... */
      osip_message_set_contact(response, jc->c_redirection);
    }

  osip_message_set_content_length(response, "0");
  /*  send message to transaction layer */

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();

  return 0;
}

int
_eXosip2_answer_invite_1xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code, osip_message_t **answer)
{
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }
  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }

  if (jd==NULL)
    i = _eXosip_build_response_default(answer, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(answer, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return -2;
    }

  osip_message_set_content_length(*answer, "0");
  /*  send message to transaction layer */

  if (code>100)
    {
      i = complete_answer_that_establish_a_dialog(*answer, tr->orig_request);
    }
  
  return 0;
}

int
_eXosip2_answer_invite_2xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code, osip_message_t **answer)
{
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);

  if (tr==NULL || tr->orig_request==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer\n"));
      return -1;
    }

  if (jd!=NULL && jd->d_dialog==NULL)
    {  /* element previously removed */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot answer this closed transaction\n"));
      return -1;
    }

  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }
  
  if (jd==NULL)
    i = _eXosip_build_response_default(answer, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(answer, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      return -1;
    }

  /* request that estabish a dialog: */
  /* 12.1.1 UAS Behavior */
  {
    i = complete_answer_that_establish_a_dialog(*answer, tr->orig_request);
    if (i!=0) goto g2atii_error_1;; /* ?? */
  }

  return 0;

 g2atii_error_1:
  osip_message_free(*answer);
  return -1;
}

int
_eXosip2_answer_invite_3456xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code, osip_message_t **answer)
{
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }
  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }

  i = _eXosip_build_response_default(answer, jd->d_dialog, code, tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      return -1;
    }

  if (300<=code<=399)
    {
      /* Should add contact fields */
      /* ... */
    }

  osip_message_set_content_length(*answer, "0");
  /*  send message to transaction layer */

  return 0;
}

int
eXosip_answer_invite_1xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }
  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }

  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return -2;
    }

  osip_message_set_content_length(response, "0");
  /*  send message to transaction layer */

  if (code>100)
    {
      /* request that estabish a dialog: */
      /* 12.1.1 UAS Behavior */
      i = complete_answer_that_establish_a_dialog(response, tr->orig_request);

      if (jd==NULL)
	{
	  i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
	  if (i!=0)
	    {
         OSIP_TRACE (osip_trace
		     (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot create dialog!\n"));
	    }
	  ADD_ELEMENT(jc->c_dialogs, jd);
	}
    }

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  
  return 0;
}

int
eXosip_answer_invite_2xx_with_body(eXosip_call_t *jc, eXosip_dialog_t *jd, int code,const char*bodytype, const char*body)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  char *size;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);

  if (tr==NULL || tr->orig_request==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer\n"));
      return -1;
    }

  if (jd!=NULL && jd->d_dialog==NULL)
    {  /* element previously removed */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot answer this closed transaction\n"));
      return -1;
    }

  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }
  
  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      code = 500; /* ? which code to use? */
      return -1;
    }

  if (code==488)
    {
      osip_message_set_content_length(response, "0");
      /*  TODO: send message to transaction layer */
      evt_answer = osip_new_outgoing_sipmessage(response);
      evt_answer->transactionid = tr->transactionid;
      osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
      return 0;
    }

  i = osip_message_set_body(response, body, strlen(body));
  if (i!=0) {
    goto g2atii_error_1;
  }
  size = (char *) osip_malloc(6*sizeof(char));
  sprintf(size,"%i",strlen(body));
  i = osip_message_set_content_length(response, size);
  osip_free(size);
  if (i!=0) goto g2atii_error_1;
  i = osip_message_set_content_type(response, bodytype);
  if (i!=0) goto g2atii_error_1;

  /* request that estabish a dialog: */
  /* 12.1.1 UAS Behavior */
  {
    i = complete_answer_that_establish_a_dialog(response, tr->orig_request);
    if (i!=0) goto g2atii_error_1;; /* ?? */
  }
  /* THIS RESPONSE MUST BE SENT RELIABILY until the final ACK is received !! */
  /* this response must be stored at the upper layer!!! (it will be destroyed*/
  /* right after being sent! */

  if (jd==NULL)
    {
      i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
      if (i!=0)
	{
     OSIP_TRACE (osip_trace
		 (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: cannot create dialog!\n"));
	  return -1;
	}
      ADD_ELEMENT(jc->c_dialogs, jd);
    }

  eXosip_dialog_set_200ok(jd, response);
  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);

  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
  __eXosip_wakeup();
  return 0;

 g2atii_error_1:
  osip_message_free(response);
  return -1;
}

int
eXosip_answer_invite_2xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code, char *local_sdp_port)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  char *size;
  char *body = NULL;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);

  if (tr==NULL || tr->orig_request==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer\n"));
      return -1;
    }

  if (jd!=NULL && jd->d_dialog==NULL)
    {  /* element previously removed */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot answer this closed transaction\n"));
      return -1;
    }

  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }

  /* WE SHOULD LOOK FOR A SDP PACKET!! */
  if(NULL != osip_list_get(tr->orig_request->bodies,0))
    {
      body = generating_sdp_answer(tr->orig_request, jc->c_ctx);
      if (body==NULL)
	code = 488; /* bad sdp */
    }
  else
    {
      if(local_sdp_port==NULL)
	code = 488; /* session description in the request is not acceptable. */
      else
	/* body is NULL (contains no SDP), generate a response to INVITE w/ no SDP */
	body = generating_no_sdp_answer(jc, jd, tr->orig_request, local_sdp_port);
    }
  
  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      code = 500; /* ? which code to use? */
      osip_free(body); /* not used */
      return -1;
    }

  if (code==488)
    {
      osip_message_set_content_length(response, "0");
      /*  TODO: send message to transaction layer */
      osip_free(body);
      evt_answer = osip_new_outgoing_sipmessage(response);
      evt_answer->transactionid = tr->transactionid;
      osip_transaction_add_event(tr, evt_answer);
	  __eXosip_wakeup();
      return 0;
    }

  i = osip_message_set_body(response, body, strlen(body));
  if (i!=0) {
    goto g2atii_error_1;
  }
  size = (char *) osip_malloc(6*sizeof(char));
#ifdef __APPLE_CC__
  sprintf(size,"%li",strlen(body));
#else
  sprintf(size,"%i",strlen(body));
#endif
  i = osip_message_set_content_length(response, size);
  osip_free(size);
  if (i!=0) goto g2atii_error_1;
  i = osip_message_set_content_type(response, "application/sdp");
  if (i!=0) goto g2atii_error_1;

  /* request that estabish a dialog: */
  /* 12.1.1 UAS Behavior */
  {
    i = complete_answer_that_establish_a_dialog(response, tr->orig_request);
    if (i!=0) goto g2atii_error_1;; /* ?? */
  }

  osip_free(body);
  /* THIS RESPONSE MUST BE SENT RELIABILY until the final ACK is received !! */
  /* this response must be stored at the upper layer!!! (it will be destroyed*/
  /* right after being sent! */

  if (jd==NULL)
    {
      i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
      if (i!=0)
	{
     OSIP_TRACE (osip_trace
		 (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: cannot create dialog!\n"));
	  return -1;
	}
      ADD_ELEMENT(jc->c_dialogs, jd);
    }

  eXosip_dialog_set_200ok(jd, response);
  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);

  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
  __eXosip_wakeup();
  return 0;

 g2atii_error_1:
  osip_free(body);
  osip_message_free(response);
  return -1;
}

int
eXosip_answer_invite_3456xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return -1;
    }
  /* is the transaction already answered? */
  if (tr->state==IST_COMPLETED
      || tr->state==IST_CONFIRMED
      || tr->state==IST_TERMINATED)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: transaction already answered\n"));
      return -1;
    }

  i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      return -1;
    }

  if (300<=code<=399)
    {
      /* Should add contact fields */
      /* ... */
      osip_message_set_contact(response, jc->c_redirection);
    }

  osip_message_set_content_length(response, "0");
  /*  send message to transaction layer */

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  return 0;
}


void
eXosip_notify_answer_subscribe_1xx(eXosip_notify_t *jn, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_subscribe(jn, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return;
    }

  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for subscribe\n"));
      return;
    }

  if (code>100)
    {
      /* request that estabish a dialog: */
      /* 12.1.1 UAS Behavior */
      i = complete_answer_that_establish_a_dialog(response, tr->orig_request);

      if (jd==NULL)
	{
	  i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
	  if (i!=0)
	    {
         OSIP_TRACE (osip_trace
	    	  (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot create dialog!\n"));
	    }
	  ADD_ELEMENT(jn->n_dialogs, jd);
	}
    }

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  return ;
}

void
eXosip_notify_answer_subscribe_2xx(eXosip_notify_t *jn, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_subscribe(jn, jd);

  if (tr==NULL || tr->orig_request==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer\n"));
      return;
    }

  if (jd!=NULL && jd->d_dialog==NULL)
    {  /* element previously removed, this is a no hop! */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot answer this closed transaction\n"));
      return ;
    }

  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for subscribe\n"));
      code = 500; /* ? which code to use? */
      return;
    }

  /* request that estabish a dialog: */
  /* 12.1.1 UAS Behavior */
  {
    i = complete_answer_that_establish_a_dialog(response, tr->orig_request);
    if (i!=0) goto g2atii_error_1;; /* ?? */
  }

  /* THIS RESPONSE MUST BE SENT RELIABILY until the final ACK is received !! */
  /* this response must be stored at the upper layer!!! (it will be destroyed*/
  /* right after being sent! */

  if (jd==NULL)
    {
      i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
      if (i!=0)
	{
     OSIP_TRACE (osip_trace
       (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: cannot create dialog!\n"));
	  return;
	}
      ADD_ELEMENT(jn->n_dialogs, jd);
    }

  eXosip_dialog_set_200ok(jd, response);
  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();

  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
  return ;

 g2atii_error_1:
  osip_message_free(response);
  return ;
}

void
eXosip_notify_answer_subscribe_3456xx(eXosip_notify_t *jn, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_subscribe(jn, jd);
  if (tr==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot find transaction to answer"));
      return;
    }
  i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for subscribe\n"));
      return;
    }

  if (300<=code<=399)
    {
      /* Should add contact fields */
      /* ... */
    }

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  return ;
}
