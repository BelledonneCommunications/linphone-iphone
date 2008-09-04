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

extern eXosip_t eXosip;

osip_list_t *supported_codec = NULL;

int eXosip_sdp_accept_audio_codec(osip_negotiation_ctx_t *context,
				  char *port, char *number_of_port,
				  int audio_qty, char *payload);
int eXosip_sdp_accept_video_codec(osip_negotiation_ctx_t *context,
				  char *port, char *number_of_port,
				  int video_qty, char *payload);
int eXosip_sdp_accept_other_codec(osip_negotiation_ctx_t *context,
				  char *type, char *port,
				  char *number_of_port, char *payload);

char *eXosip_sdp_get_audio_port(osip_negotiation_ctx_t *context, int pos_media);

int eXosip_sdp_accept_audio_codec(osip_negotiation_ctx_t *context,
				  char *port, char *number_of_port,
				  int audio_qty, char *payload)
{
  int pos;
  for (pos=0;!osip_list_eol(supported_codec, pos);pos++)
    {
      char *_payload;
      _payload = osip_list_get(supported_codec, pos);
      if (0==strcmp(payload,_payload))
	{
	  /* 
	     We have to look at the rtpmap attributes in context->remote
	     to check if we support this stuff.
	  */

	  return 0;
	}
    }
  return -1;
}
  
int eXosip_sdp_accept_video_codec(osip_negotiation_ctx_t *context,
				  char *port, char *number_of_port,
				  int video_qty, char *payload)
{
  return -1;
}

int eXosip_sdp_accept_other_codec(osip_negotiation_ctx_t *context,
				  char *type, char *port,
				  char *number_of_port, char *payload)
{
  /* ... */
  return -1;
}

char *eXosip_sdp_get_audio_port(osip_negotiation_ctx_t *context, int pos_media)
{
  eXosip_call_t *jc = (eXosip_call_t*)osip_negotiation_ctx_get_mycontext(context);
  if (jc==NULL)
    return osip_strdup("10500");
  else if (jc->c_sdp_port[0]=='\0')
    return osip_strdup("10500");
  else return osip_strdup(jc->c_sdp_port);
}

int eXosip_sdp_negotiation_replace(osip_negotiation_t *sn)
{
  if (eXosip.osip_negotiation!=NULL)
    osip_negotiation_free(eXosip.osip_negotiation);
  eXosip.osip_negotiation = sn;
  return 0;
}

void
eXosip_sdp_negotiation_ctx_set_mycontext(struct eXosip_call_t *jc, void *arg)
{
	osip_negotiation_ctx_set_mycontext(jc->c_ctx, arg);
}

void eXosip_sdp_negotiation_remove_audio_payloads()
{
  if (supported_codec==NULL)
    return;
  for (;!osip_list_eol(supported_codec, 0);)
    {
      char *p;
      p = (char *) osip_list_get(supported_codec, 0);
      osip_free(p);
      osip_list_remove(supported_codec, 0);
    }
  osip_negotiation_remove_audio_payloads(eXosip.osip_negotiation);
}

void eXosip_sdp_negotiation_add_codec(char *payload, char *number_of_port,
				      char *proto, char *c_nettype,
				      char *c_addrtype, char *c_addr,
				      char *c_addr_multicast_ttl,
				      char *c_addr_multicast_int,
				      char *a_rtpmap)
{
  osip_negotiation_add_support_for_audio_codec(eXosip.osip_negotiation,
					       payload,
					       number_of_port,
					       proto,
					       c_nettype,
					       c_addrtype,
					       c_addr,
					       c_addr_multicast_ttl,
					       c_addr_multicast_int,
					       a_rtpmap);
  osip_list_add(supported_codec, osip_strdup(payload), -1);
}

int eXosip_sdp_negotiation_init(osip_negotiation_t **sn)
{
  int i = osip_negotiation_init(sn);
  if (i!=0) {
    return -1;
  }
  if (supported_codec==NULL)
    {
      supported_codec = (osip_list_t*) osip_malloc(sizeof(osip_list_t));
      osip_list_init(supported_codec);
    }
  osip_negotiation_set_o_username(*sn, osip_strdup("userX"));
  osip_negotiation_set_o_session_id(*sn, osip_strdup("20000001"));
  osip_negotiation_set_o_session_version(*sn, osip_strdup("20000001"));
  osip_negotiation_set_o_nettype(*sn, osip_strdup("IN"));
  osip_negotiation_set_o_addrtype(*sn, osip_strdup("IP4"));
  osip_negotiation_set_o_addr(*sn, osip_strdup(eXosip.localip));
  
  osip_negotiation_set_c_nettype(*sn, osip_strdup("IN"));
  osip_negotiation_set_c_addrtype(*sn, osip_strdup("IP4"));
  osip_negotiation_set_c_addr(*sn, osip_strdup(eXosip.localip));
  
  /* ALL CODEC MUST SHARE THE SAME "C=" line and proto as the media 
     will appear on the same "m" line... */

  osip_negotiation_set_fcn_accept_audio_codec(*sn, &eXosip_sdp_accept_audio_codec);
  osip_negotiation_set_fcn_accept_video_codec(*sn, &eXosip_sdp_accept_video_codec);
  
  osip_negotiation_set_fcn_accept_other_codec(*sn, &eXosip_sdp_accept_other_codec);
  osip_negotiation_set_fcn_get_audio_port(*sn, &eXosip_sdp_get_audio_port);

  return 0;
}

void eXosip_sdp_negotiation_free(osip_negotiation_t *sn)
{
  if (sn==NULL)
    return;
  osip_list_ofchar_free(supported_codec);
  supported_codec = NULL;
  osip_negotiation_free(sn);
}

sdp_message_t *
eXosip_get_local_sdp_info(osip_transaction_t *invite_tr)
{
  osip_content_type_t *ctt;
  osip_mime_version_t *mv;
  osip_message_t *message;
  sdp_message_t *sdp;
  osip_body_t *oldbody;
  int pos;

  if (invite_tr->ctx_type == IST)
    message = invite_tr->last_response;
  else if (invite_tr->ctx_type == ICT)
    message = invite_tr->orig_request;
  else return NULL; /* BUG -> NOT AN INVITE TRANSACTION!! */

  if (message==NULL) return NULL;

  /* get content-type info */
  ctt = osip_message_get_content_type(message);
  mv  = osip_message_get_mime_version(message);
  if (mv==NULL && ctt==NULL)
    return NULL; /* previous message was not correct or empty */
  if (mv!=NULL)
    {
      /* look for the SDP body */
      /* ... */
    }
  else if (ctt!=NULL)
    {
      if (ctt->type==NULL || ctt->subtype==NULL)
	/* it can be application/sdp or mime... */
	return NULL;
      if (osip_strcasecmp(ctt->type, "application")!=0 ||
	  osip_strcasecmp(ctt->subtype, "sdp")!=0 )
	{ return NULL; }
    }
  
  pos=0;
  while (!osip_list_eol(message->bodies, pos))
    {
      int i;
      oldbody = (osip_body_t *) osip_list_get(message->bodies, pos);
      pos++;
      sdp_message_init(&sdp);
      i = sdp_message_parse(sdp,oldbody->body);
      if (i==0) return sdp;
      sdp_message_free(sdp);
      sdp = NULL;
    }
  return NULL;
}

sdp_message_t *
eXosip_get_remote_sdp_info(osip_transaction_t *invite_tr)
{
  osip_content_type_t *ctt;
  osip_mime_version_t *mv;
  osip_message_t *message;
  sdp_message_t *sdp;
  osip_body_t *oldbody;
  int pos;

  if (invite_tr->ctx_type == IST)
    message = invite_tr->orig_request;
  else if (invite_tr->ctx_type == ICT)
    message = invite_tr->last_response;
  else return NULL; /* BUG -> NOT AN INVITE TRANSACTION!! */

  if (message==NULL) return NULL;

  /* get content-type info */
  ctt = osip_message_get_content_type(message);
  mv  = osip_message_get_mime_version(message);
  if (mv==NULL && ctt==NULL)
    return NULL; /* previous message was not correct or empty */
  if (mv!=NULL)
    {
      /* look for the SDP body */
      /* ... */
    }
  else if (ctt!=NULL)
    {
      if (ctt->type==NULL || ctt->subtype==NULL)
	/* it can be application/sdp or mime... */
	return NULL;
      if (osip_strcasecmp(ctt->type, "application")!=0 ||
	  osip_strcasecmp(ctt->subtype, "sdp")!=0 )
	{ return NULL; }
    }
  
  pos=0;
  while (!osip_list_eol(message->bodies, pos))
    {
      int i;
      oldbody = (osip_body_t *) osip_list_get(message->bodies, pos);
      pos++;
      sdp_message_init(&sdp);
      i = sdp_message_parse(sdp,oldbody->body);
      if (i==0) return sdp;
      sdp_message_free(sdp);
      sdp = NULL;
    }
  return NULL;
}

int eXosip_retrieve_sdp_negotiation_result(osip_negotiation_ctx_t *ctx, char *payload_name,  int pnsize)
{
  sdp_message_t *local_sdp = 0;
  int payload_result = -1;
  
  if (!ctx)
    return payload_result;

  local_sdp = osip_negotiation_ctx_get_local_sdp(ctx);

  if (local_sdp != NULL)
    {
      sdp_media_t *med = (sdp_media_t*) osip_list_get(local_sdp->m_medias, 0);
      char *payload = (char *) osip_list_get (med->m_payloads, 0);
      int pos_attr;

      if (payload!=NULL)
	{
	  payload_result = osip_atoi(payload);

	  /* copy payload name! */
	  for (pos_attr=0;
	       !osip_list_eol(med->a_attributes, pos_attr);
	       pos_attr++)
	    {
	      sdp_attribute_t *attr;
	      attr = (sdp_attribute_t *)osip_list_get(med->a_attributes, pos_attr);
	      if (0==osip_strncasecmp(attr->a_att_field, "rtpmap", 6))
		{
		  if ((payload_result <10 && 
		       0==osip_strncasecmp(attr->a_att_value, payload, 1))
		      ||(payload_result>9 && payload_result<100 && 
			 0==osip_strncasecmp(attr->a_att_value, payload, 2))
		      ||(payload_result >100 && payload_result<128 &&
			 0==osip_strncasecmp(attr->a_att_value, payload, 3)))
		    {
		      snprintf(payload_name, pnsize, "%s", attr->a_att_value);
		      return payload_result;
		    }
		}
	    }
	}
    }

  return payload_result;

}
