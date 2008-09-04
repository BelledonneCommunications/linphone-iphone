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

#include <stdlib.h>

#ifdef WIN32
#include <windowsx.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#else 
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <eXosip.h>
#include "eXosip2.h"
#include <eXosip_cfg.h>

extern eXosip_t eXosip;

#ifdef TEST_AUDIO
static pid_t pid = 0;
#endif


/* Private functions */
static void rcvregister_failure(int type, osip_transaction_t *tr,osip_message_t *sip);
int cb_udp_snd_message(osip_transaction_t *tr, osip_message_t *sip,
			      char *host, int port, int out_socket);
static void cb_ict_kill_transaction(int type, osip_transaction_t *tr);
static void cb_ist_kill_transaction(int type, osip_transaction_t *tr);
static void cb_nict_kill_transaction(int type, osip_transaction_t *tr);
static void cb_nist_kill_transaction(int type, osip_transaction_t *tr);
static void cb_rcvinvite  (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvack     (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvack2    (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvregister(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvbye     (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvcancel  (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvinfo    (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvoptions (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvnotify  (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvsubscribe (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvunkrequest(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndinvite  (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndack     (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndregister(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndbye     (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndcancel  (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndinfo    (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndoptions (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndnotify  (int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndsubscribe(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_sndunkrequest(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv1xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv2xx_4invite(osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv2xx_4subscribe(osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv2xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv3xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv4xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv5xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcv6xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_snd1xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_snd2xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_snd3xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_snd4xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_snd5xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_snd6xx(int type, osip_transaction_t *tr,osip_message_t *sip);
static void cb_rcvresp_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip);
static void cb_sndreq_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip);
static void cb_sndresp_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip);
static void cb_rcvreq_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip);
static void cb_transport_error(int type, osip_transaction_t *tr, int error);
static void report_call_event_with_status(int evt, eXosip_call_t *jc, eXosip_dialog_t *jd, osip_message_t *sip);
static void report_event_with_status(eXosip_event_t *je, osip_message_t *sip);

int cb_udp_snd_message(osip_transaction_t *tr, osip_message_t *sip, char *host,
		       int port, int out_socket)
{
  int len = 0;
  size_t length = 0;
  static int num = 0;
  struct addrinfo *addrinfo;
  struct __eXosip_sockaddr addr;
  char *message;
  int i;

  if (eXosip.j_socket==0) return -1;

  if (host==NULL)
    {
      host = sip->req_uri->host;
      if (sip->req_uri->port!=NULL)
	port = osip_atoi(sip->req_uri->port);
      else
	port = 5060;
    }

  i = eXosip_get_addrinfo(&addrinfo, host, port);
  if (i!=0)
    {
      return -1;
    }
  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  len = addrinfo->ai_addrlen;

  freeaddrinfo (addrinfo);

  i = osip_message_to_str(sip, &message, &length);

  if (i!=0 || length<=0) {
    return -1;
  }

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			"Message sent: \n%s (len=%i sizeof(addr)=%i %i)\n",
			message, len, sizeof(addr), sizeof(struct sockaddr_in6)));
  if (0  > sendto (eXosip.j_socket, (const void*) message, length, 0,
		   (struct sockaddr *) &addr, len /* sizeof(addr) */ )) 
    {
#ifdef WIN32
      if (WSAECONNREFUSED==WSAGetLastError())
#else
	if (ECONNREFUSED==errno)
#endif
	  {
	    /* This can be considered as an error, but for the moment,
	       I prefer that the application continue to try sending
	       message again and again... so we are not in a error case.
	       Nevertheless, this error should be announced!
	       ALSO, UAS may not have any other options than retry always
	       on the same port.
	    */
	    osip_free(message);
	    return 1;
	  }
	else
	  {
	    /* SIP_NETWORK_ERROR; */
	    osip_free(message);
	    return -1;
	  }
    }
  if (strncmp(message, "INVITE", 7)==0)
    {
      num++;
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,"number of message sent: %i\n", num));
    }

  osip_free(message);
  return 0;
  
}

static void cb_ict_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_ict_kill_transaction (id=%i)\r\n", tr->transactionid));

  i = osip_remove_transaction(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_ict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

static void cb_ist_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_ist_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_transaction(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_ist_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

static void cb_nict_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  eXosip_dialog_t    *jd;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t    *jn;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_nict_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_transaction(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_nict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }

  if (MSG_IS_REGISTER(tr->orig_request)
      && type==OSIP_NICT_KILL_TRANSACTION
      && tr->last_response==NULL)
    {
      eXosip_event_t *je;
      eXosip_reg_t *jreg=NULL;
      /* find matching j_reg */
      _eXosip_reg_find(&jreg, tr);
      if (jreg!=NULL)
	{
	  je = eXosip_event_init_for_reg(EXOSIP_REGISTRATION_FAILURE, jreg);
	  report_event_with_status(je, NULL);
	}
      return;
    }

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  jn = jinfo->jn;
  js = jinfo->js;

  if (jn==NULL && js==NULL)
    return;

  /* no answer to a NOTIFY request! */
  if (MSG_IS_NOTIFY(tr->orig_request)
      && type==OSIP_NICT_KILL_TRANSACTION
      && tr->last_response==NULL)
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_notifies, jn);
      eXosip_notify_free(jn);
      return;
    }

  if (MSG_IS_NOTIFY(tr->orig_request)
      && type==OSIP_NICT_KILL_TRANSACTION
      && tr->last_response!=NULL
      && tr->last_response->status_code > 299)
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_notifies, jn);
      eXosip_notify_free(jn);
      return;
    }

  if (MSG_IS_NOTIFY(tr->orig_request)
      && type==OSIP_NICT_KILL_TRANSACTION
      && tr->last_response!=NULL
      && tr->last_response->status_code > 199
      && tr->last_response->status_code < 300)
    {
      if (jn->n_ss_status==EXOSIP_SUBCRSTATE_TERMINATED)
	{
	  /* delete the dialog! */
	  REMOVE_ELEMENT(eXosip.j_notifies, jn);
	  eXosip_notify_free(jn);
	  return;
	}
    }

  /* no answer to a SUBSCRIBE request! */
  if (MSG_IS_SUBSCRIBE(tr->orig_request)
      && type==OSIP_NICT_KILL_TRANSACTION
      && tr->last_response==NULL)
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_subscribes, js);
      eXosip_subscribe_free(js);
      return;
    }

  /* detect SUBSCRIBE request that close the dialogs! */
  /* expires=0 with MSN */
  if (MSG_IS_SUBSCRIBE(tr->orig_request)
      && type==OSIP_NICT_KILL_TRANSACTION)
    {
      osip_header_t *expires;
      osip_message_get_expires(tr->orig_request, 0, &expires);
      if (expires==NULL || expires->hvalue==NULL)
	{
	}
      else if (0==strcmp(expires->hvalue, "0"))
	{
	  /* delete the dialog! */
	  REMOVE_ELEMENT(eXosip.j_subscribes, js);
	  eXosip_subscribe_free(js);
	  return;
	}
    }
}

static void cb_nist_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_nist_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_transaction(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_nist_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }

}
  
static void cb_rcvinvite  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvinvite (id=%i)\n", tr->transactionid));
}

static void cb_rcvack     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvack (id=%i)\n", tr->transactionid));
}

static void cb_rcvack2    (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvack2 (id=%i)\r\n", tr->transactionid));
}
  
static void cb_rcvregister(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvregister (id=%i)\r\n", tr->transactionid));
}

static void cb_rcvbye     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvbye (id=%i)\r\n", tr->transactionid));
#ifdef TEST_AUDIO
  if (pid!=0)
    {
      int i = kill(pid, SIGINT);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"audio command kill return %i %i\n", i, pid));
      pid = 0;
    }
#endif
}

static void cb_rcvcancel  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvcancel (id=%i)\r\n", tr->transactionid));
}

static void cb_rcvinfo    (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_event_t     *je;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvinfo (id=%i)\r\n", tr->transactionid));
  
  if (jinfo==NULL)
    return;
  if (jinfo->jc==NULL)
    return;

  je = eXosip_event_init_for_call(EXOSIP_INFO_NEW, jinfo->jc, jinfo->jd);
  if (je!=NULL)
    {
      char *tmp;
      osip_uri_to_str(sip->req_uri, &tmp);
      if (tmp!=NULL)
 	{
 	  snprintf(je->req_uri, 255, "%s", tmp);
 	  osip_free(tmp);
 	}
      
      if (sip!=NULL)
	{
	  int pos;
	  /* get content-type info */
	  osip_content_type_clone(osip_message_get_content_type(sip), &(je->i_ctt));
	  /* get list of bodies */
	  je->i_bodies = (osip_list_t*) osip_malloc(sizeof(osip_list_t));
	  osip_list_init(je->i_bodies);
	  for (pos=0;!osip_list_eol(sip->bodies, pos);pos++)
	    {
	      osip_body_t *body;
	      osip_body_t *_body;
	      body = (osip_body_t *)osip_list_get(sip->bodies, pos);
	      osip_body_clone(body, &_body);
	      osip_list_add(je->i_bodies, _body, -1);
	    }
	}
    }

  report_event_with_status(je, NULL);
}

static void cb_rcvoptions (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_event_t     *je;
  eXosip_dialog_t    *jd;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t    *jn;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvoptions (id=%i)\r\n", tr->transactionid));

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  jn = jinfo->jn;
  js = jinfo->js;
  if (jinfo->jc==NULL)
    return;
  
  je = eXosip_event_init_for_call(EXOSIP_OPTIONS_NEW, jc, jd);
  if (je!=NULL)
    {
      char *tmp;
      osip_uri_to_str(sip->req_uri, &tmp);
      if (tmp!=NULL)
	{
	  snprintf(je->req_uri, 255, "%s", tmp);
	  osip_free(tmp);
	}
    }
  report_event_with_status(je, NULL);

}

static void cb_rcvnotify  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvnotify (id=%i)\r\n", tr->transactionid));
}

static void cb_rcvsubscribe (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_event_t     *je;
  eXosip_dialog_t    *jd;
  eXosip_notify_t    *jn;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvsubscribe (id=%i)\r\n", tr->transactionid));

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jn = jinfo->jn;
  if (jinfo->jn==NULL)
    return;
  
  je = eXosip_event_init_for_notify(EXOSIP_IN_SUBSCRIPTION_NEW, jn, jd);
  if (je!=NULL)
    {
      char *tmp;
      osip_uri_to_str(sip->req_uri, &tmp);
      if (tmp!=NULL)
	{
	  snprintf(je->req_uri, 255, "%s", tmp);
	  osip_free(tmp);
	}
    }
  report_event_with_status(je, NULL);
}

static void cb_rcvunkrequest(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t    *jd;
  eXosip_call_t      *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvunkrequest (id=%i)\r\n", tr->transactionid));

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jinfo->jc==NULL)
    return;

  
  if (MSG_IS_REFER(sip))
    {
      eXosip_event_t *je;

      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvrefer (id=%i)\r\n", tr->transactionid));

      je = eXosip_event_init_for_call(EXOSIP_CALL_REFERED, jc, jd);
      if (je!=NULL)
	{
	  report_event_with_status(je, NULL);
	}
    }

}

static void cb_sndinvite  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndinvite (id=%i)\r\n", tr->transactionid));
}

static void cb_sndack     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndack (id=%i)\r\n", tr->transactionid));
}
  
static void cb_sndregister(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndregister (id=%i)\r\n", tr->transactionid));
}

static void cb_sndbye     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndbye (id=%i)\r\n", tr->transactionid));
#ifdef TEST_AUDIO
  if (pid!=0)
    {
      int i = kill(pid, SIGINT);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"audio command kill return %i %i\n", i, pid));
      pid = 0;
    }
#endif

}

static void cb_sndcancel  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndcancel (id=%i)\r\n", tr->transactionid));
}

static void cb_sndinfo    (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndinfo (id=%i)\r\n", tr->transactionid));
}

static void cb_sndoptions (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndoptions (id=%i)\r\n", tr->transactionid));
}

static void cb_sndnotify  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndnotify (id=%i)\r\n", tr->transactionid));
}

static void cb_sndsubscribe(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndsubscibe (id=%i)\r\n", tr->transactionid));
}

static void cb_sndunkrequest(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndunkrequest (id=%i)\r\n", tr->transactionid));
}

void __eXosip_delete_jinfo(osip_transaction_t *transaction)
{
  jinfo_t *ji;
  if (transaction==NULL)
    return;
  ji = osip_transaction_get_your_instance(transaction);
  osip_free(ji);
  osip_transaction_set_your_instance(transaction, NULL);
}

jinfo_t *__eXosip_new_jinfo(eXosip_call_t *jc, eXosip_dialog_t *jd,
			    eXosip_subscribe_t *js, eXosip_notify_t *jn)
{
  jinfo_t *ji = (jinfo_t *) osip_malloc(sizeof(jinfo_t));
  if (ji==NULL) return NULL;
  ji->jd = jd;
  ji->jc = jc;
  ji->js = js;
  ji->jn = jn;
  return ji;
}

static void cb_rcv1xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t    *jd;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t    *jn;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv1xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  jn = jinfo->jn;
  js = jinfo->js;

  if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS"))
    {
      if (jc==NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv1xx (id=%i) Error: no call or transaction info for OPTIONS transaction\r\n", tr->transactionid));
	  return;
	}
      else if (jc->c_out_options_tr==NULL)
	{
	  /* options is within a call */
	}
      report_call_event_with_status(EXOSIP_OPTIONS_PROCEEDING, jc, jd, sip);
      return;
    }

  if ((MSG_IS_RESPONSE_FOR(sip, "INVITE")
       || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
      && !MSG_TEST_CODE(sip, 100))
    {
      int i;
      /* for SUBSCRIBE, test if the dialog has been already created
	 with a previous NOTIFY */
      if (jd==NULL && js!=NULL && js->s_dialogs!=NULL && MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
	{
	  /* find if existing dialog match the to tag */
	  osip_generic_param_t *tag;
	  int i;
	  i = osip_to_get_tag (sip->to, &tag);
	  if (i==0 && tag!=NULL && tag->gvalue!=NULL )
	    {
	      for (jd = js->s_dialogs; jd!= NULL ; jd=jd->next)
		{
		  if (0==strcmp(jd->d_dialog->remote_tag, tag->gvalue))
		    {
		      OSIP_TRACE (osip_trace
				  (__FILE__, __LINE__, OSIP_INFO2, NULL,
				   "eXosip: found established early dialog for this subscribe\n"));
		      jinfo->jd = jd;
		      break;
		    }
		}
	    }
	}

      if (jd == NULL) /* This transaction initiate a dialog in the case of
			 INVITE (else it would be attached to a "jd" element. */
	{
	  /* allocate a jd */

	  i = eXosip_dialog_init_as_uac(&jd, sip);
	  if (i!=0)
	    {
	      OSIP_TRACE (osip_trace
		     (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot establish a dialog\n"));
	      return;
	    }
	  if (jc!=NULL)
	    {
	      ADD_ELEMENT(jc->c_dialogs, jd);
	      jinfo->jd = jd;
	      eXosip_update();
	    }
	  else if (js!=NULL)
	    {
	      ADD_ELEMENT(js->s_dialogs, jd);
	      jinfo->jd = jd;
	      eXosip_update();
	    }
	  else if (jn!=NULL)
	    {
	      ADD_ELEMENT(jn->n_dialogs, jd);
	      jinfo->jd = jd;
	      eXosip_update();
	    }
	  else
	    {
#ifndef WIN32
	      assert(0==0);
#else
		  exit(0);
#endif
	    }
	  osip_transaction_set_your_instance(tr, jinfo);
	}
      else
	{
	  osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
	}

      if ( jd!=NULL)
	jd->d_STATE = JD_TRYING;
      if ( jd!=NULL && MSG_IS_RESPONSE_FOR(sip, "INVITE")
	   && sip->status_code < 180)
	{
	  eXosip_event_t *je;
	  je = eXosip_event_init_for_call(EXOSIP_CALL_PROCEEDING, jc, jd);
	  if (je!=NULL)
	    {
	      if (sip->status_code>100)
		eXosip_event_add_sdp_info(je, sip);
	      report_event_with_status(je, sip);
	    }
	}
      else if ( jd!=NULL && MSG_IS_RESPONSE_FOR(sip, "INVITE")
		&& sip->status_code >= 180)
	{
	  eXosip_event_t *je;
	  je = eXosip_event_init_for_call(EXOSIP_CALL_RINGING, jc, jd);
	  if (je!=NULL)
	    {
	      eXosip_event_add_sdp_info(je, sip);
	      report_event_with_status(je, sip);
	    }
	}
      else if ( jd!=NULL && MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
	{
	  eXosip_event_t *je;
	  je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_PROCEEDING, js, jd);
	  if (je!=NULL)
	    report_event_with_status(je, sip);
	}
      if (MSG_TEST_CODE(sip, 180) && jd!=NULL)
	{
	  jd->d_STATE = JD_RINGING;
	}
      else if (MSG_TEST_CODE(sip, 183) && jd!=NULL)
	{
	  jd->d_STATE = JD_QUEUED;
	}

    }
}

sdp_message_t *eXosip_get_remote_sdp(osip_transaction_t *transaction)
{
  osip_message_t *message;
  osip_body_t *body;
  sdp_message_t *sdp;
  int pos = 0;
  int i;
  if (transaction->ist_context!=NULL)
    /* remote sdp is in INVITE (or ACK!) */
    message = transaction->orig_request;
  else
    /* remote sdp is in response */
    message = transaction->last_response;

  if (message==NULL)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"No remote sdp body found\r\n"));
      return NULL;
    }
  sdp=NULL;
  body = (osip_body_t *)osip_list_get(message->bodies,0);
  while (body!=NULL)
    {
      i = sdp_message_init(&sdp);
      if (i!=0)
	{ sdp = NULL; break; }
      i = sdp_message_parse(sdp,body->body);
      if (i==0)
	return sdp;
      sdp_message_free(sdp);
      sdp = NULL;
      pos++;
      body = (osip_body_t *)osip_list_get(message->bodies,pos);
    }
  return NULL;
}

sdp_message_t *eXosip_get_local_sdp(osip_transaction_t *transaction)
{
  osip_message_t *message;
  osip_body_t *body;
  sdp_message_t *sdp;
  int i;
  int pos = 0;
  if (transaction->ict_context!=NULL)
    /* local sdp is in INVITE (or ACK!) */
    message = transaction->orig_request;
  else
    /* local sdp is in response */
    message = transaction->last_response;

  sdp=NULL;
  body = (osip_body_t *)osip_list_get(message->bodies,0);
  while (body!=NULL)
    {
      i = sdp_message_init(&sdp);
      if (i!=0)
	{ sdp = NULL; break; }
      i = sdp_message_parse(sdp,body->body);
      if (i==0)
	return sdp;
      sdp_message_free(sdp);
      sdp = NULL;
      pos++;
      body = (osip_body_t *)osip_list_get(message->bodies,pos);
    }
  return NULL;
}


static
void report_call_event_with_status(int evt, eXosip_call_t *jc, eXosip_dialog_t *jd, osip_message_t *sip)
{
  eXosip_event_t *je;
  je = eXosip_event_init_for_call(evt, jc, jd);
  if (je!=NULL)
    {
      if (sip != NULL)
	eXosip_event_add_status(je, sip);
      if (eXosip.j_call_callbacks[evt]!=NULL)
	eXosip.j_call_callbacks[evt](evt, je);
      else if (eXosip.j_runtime_mode==EVENT_MODE)
	eXosip_event_add(je);
    }

}

static
void report_event_with_status(eXosip_event_t *je, osip_message_t *sip)
{
  if (je!=NULL)
    {
      int evt = je->type;

      if (sip != NULL)
	eXosip_event_add_status(je, sip);
      if (eXosip.j_call_callbacks[evt]!=NULL)
	eXosip.j_call_callbacks[evt](evt, je);
      else if (eXosip.j_runtime_mode==EVENT_MODE)
	eXosip_event_add(je);
    }
}


#if 0
void eXosip_update_audio_session(osip_transaction_t *transaction)
{
  char *remaddr;
  sdp_message_t *remote_sdp;
  sdp_message_t *local_sdp;
  char *remote_port;
  char *local_port;
  char *payload;
  char *media_type;
  int pos;
  /* look for the SDP informations */
  
  remote_sdp = eXosip_get_remote_sdp(transaction);
  if (remote_sdp==NULL)
    return ;
  local_sdp = eXosip_get_local_sdp(transaction);
  if (local_sdp==NULL)
    {
      sdp_message_free(remote_sdp);
      return ;
    }
  remaddr=sdp_message_c_addr_get(remote_sdp,-1,0);
  if (remaddr==NULL){
    remaddr=sdp_message_c_addr_get(remote_sdp,0,0);
  }

  pos=0;
  local_port=sdp_message_m_port_get(local_sdp,pos);
  media_type = sdp_message_m_media_get(local_sdp,pos);
  while (local_port!=NULL && media_type!=NULL)
    { /* If we have refused some media lines, the port is set to 0 */
      if (0!=strncmp(local_port,"0", 1)&&0==osip_strcasecmp(media_type,"audio"))
	break;
      pos++;
      media_type = sdp_message_m_media_get(local_sdp,pos);
      local_port=sdp_message_m_port_get(local_sdp,pos);
    }

  if (media_type!=NULL && local_port!=NULL)
    {
      remote_port = sdp_message_m_port_get(remote_sdp,pos);
      payload = sdp_message_m_payload_get(local_sdp,pos,0);
    }
  else
    {
      remote_port = NULL;
      payload = NULL;
    }
  if (remote_port!=NULL && media_type!=NULL) /* if codec has been found */
    {
      char tmp[256];
      sprintf(tmp, "mediastream --local %s --remote %s:%s --payload %s > debug_rtp 2>&1" , local_port, remaddr, remote_port, payload);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"audio command %s\n", tmp));

#ifdef TEST_AUDIO
      if (pid!=0)
	{
	  int i = kill(pid, SIGINT);
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"audio command kill return %i %i\n", i, pid));
	  pid = 0;
	}

      pid = fork();
      if (pid==0)
	{
	  int ret;
#ifndef USE_EXECL
	  ret = system(tmp);
	  if (WIFSIGNALED(ret) &&
	      (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
	    {
	      exit(-1);
	    }
	  if (ret==0)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"Could not start audio\n", tmp));
	    }
	  exit(0);
#else
	  char _remoteipport[100];
	  snprintf(_remoteipport, 100, "%s:%s", remaddr, remote_port);
	  ret = execl("mediastream","--local", local_port,
			  "--remote", _remoteipport, "--payload", payload);
#endif
	}
#endif

    }
  else
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"Could not create audio session.\r\n"));
    }
  sdp_message_free(local_sdp);
  sdp_message_free(remote_sdp);
}
#endif

static void cb_rcv2xx_4invite(osip_transaction_t *tr,osip_message_t *sip)
{
  int i;
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd == NULL) /* This transaction initiate a dialog in the case of
		     INVITE (else it would be attached to a "jd" element. */
    {
      /* allocate a jd */
      i = eXosip_dialog_init_as_uac(&jd, sip);
      if (i!=0)
	{
     OSIP_TRACE (osip_trace
	    (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: cannot establish a dialog\n"));
	  return;
	}
      ADD_ELEMENT(jc->c_dialogs, jd);
      jinfo->jd = jd;
      eXosip_update();
      osip_transaction_set_your_instance(tr, jinfo);
    }
  else
    {
      /* Here is a special case:
	 We have initiated a dialog and we have received informationnal
	 answers from 2 or more remote SIP UA. Those answer can be
	 differentiated with the "To" header's tag.

	 We have used the first informationnal answer to create a
	 dialog, but we now want to be sure the 200ok received is
	 for the dialog this dialog.
	 
	 We have to check the To tag and if it does not match, we
	 just have to modify the existing dialog and replace it. */
      osip_generic_param_t *tag;
      int i;
      i = osip_to_get_tag (sip->to, &tag);
      i=1; /* default is the same dialog */

      if (jd->d_dialog==NULL || jd->d_dialog->remote_tag==NULL)
	{
	  /* There are real use-case where a BYE is received/processed before
	     the 200ok of the previous INVITE. In this case, jd->d_dialog is
	     empty and the transaction should be silently discarded. */
	  /* a ACK should still be sent... -but there is no dialog built- */
	  return;
	}

      if (jd->d_dialog->remote_tag==NULL && tag==NULL)
	{  } /* non compliant remote UA -> assume it is the same dialog */
      else if (jd->d_dialog->remote_tag!=NULL && tag==NULL)
	{ i=0; } /* different dialog! */
      else if (jd->d_dialog->remote_tag==NULL && tag!=NULL)
	{ i=0; } /* different dialog! */
      else if (jd->d_dialog->remote_tag!=NULL && tag!=NULL && tag->gvalue!=NULL
	       && 0!=strcmp(jd->d_dialog->remote_tag, tag->gvalue))
	{ i=0; } /* different dialog! */
      
      if (i==1) /* just update the dialog */
	{
	  osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
	  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
	}
      else
	{
	  /* the best thing is to update the repace the current dialog
	     information... Much easier than creating a useless dialog! */
	  osip_dialog_free(jd->d_dialog);
	  i = osip_dialog_init_as_uac(&(jd->d_dialog), sip);
	  if (i!=0)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"Cannot replace the dialog.\r\n"));
	    }
	  else
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_WARNING,NULL,"The dialog has been replaced with the new one fro 200ok.\r\n"));
	    }
	}
    }

  jd->d_STATE = JD_ESTABLISHED;

  eXosip_dialog_set_200ok(jd, sip);

  {
    osip_route_t *route;
    char *host;
    int port;
    osip_message_t *ack;
    i = _eXosip_build_request_within_dialog(&ack, "ACK", jd->d_dialog, "UDP");
    if (i!=0) {
      jd->d_STATE = JD_ESTABLISHED;
      return ;
    }

    if(jc->c_ack_sdp) /* need to build sdp answer */
      {
	char *body;
	char *size;
	
	body = generating_sdp_answer(tr->last_response, jc->c_ctx);
	if (body==NULL)
	  {
	    return;
	  }

	i = osip_message_set_body(ack, body, strlen(body));
	if (i!=0)
	  {
	    return;
	  }
	
	size = (char *) osip_malloc(6*sizeof(char));
#ifdef __APPLE_CC__
	sprintf(size,"%li",strlen(body));
#else
	sprintf(size,"%i",strlen(body));
#endif
	osip_free(body);  
	i = osip_message_set_content_length(ack, size);
	osip_free(size);
	if (i!=0)
	  {
	    return;
	  }
	i = osip_message_set_content_type(ack, "application/sdp");
	if (i!=0)
	  {
	    return;
	  }
      }

	/* SM: do not send the ack now, just prepare it.
	The application will send it using eXosip_send_ack(int jid)
	*/
	/*
    osip_message_get_route(ack, 0, &route);
    if (route!=NULL)
      {
	port = 5060;
	if (route->url->port!=NULL)
	  port = osip_atoi(route->url->port);
	host = route->url->host;
      }
    else
      {
	port = 5060;
	if (ack->req_uri->port!=NULL)
	  port = osip_atoi(ack->req_uri->port);
	host = ack->req_uri->host;
      }

    cb_udp_snd_message(NULL, ack, host, port, eXosip.j_socket);
	*/

    jd->d_ack  = ack;

  }

  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_call(EXOSIP_CALL_ANSWERED, jc, jd);
    if (je!=NULL)
      {
	eXosip_event_add_sdp_info(je, sip);
	report_event_with_status(je, sip);
      }


    je = eXosip_event_init_for_call(EXOSIP_CALL_STARTAUDIO, jc, jd);
    if (je!=NULL)
      {
	eXosip_event_add_sdp_info(je, sip);
	report_event_with_status(je, sip);
      }

  }

  /* look for the SDP information and decide if this answer was for
     an initial INVITE, an HoldCall, or a RetreiveCall */

  /* don't handle hold/unhold by now... */
  /* eXosip_update_audio_session(tr); */

}

static void cb_rcv2xx_4subscribe(osip_transaction_t *tr,osip_message_t *sip)
{
  int i;
  eXosip_dialog_t    *jd;
  eXosip_subscribe_t *js;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  js = jinfo->js;
  _eXosip_subscribe_set_refresh_interval(js, sip);


  /* for SUBSCRIBE, test if the dialog has been already created
     with a previous NOTIFY */
  if (jd==NULL && js!=NULL && js->s_dialogs!=NULL && MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      /* find if existing dialog match the to tag */
      osip_generic_param_t *tag;
      int i;
      i = osip_to_get_tag (sip->to, &tag);
      if (i==0 && tag!=NULL && tag->gvalue!=NULL )
	{
	  for (jd = js->s_dialogs; jd!= NULL ; jd=jd->next)
	    {
	      if (0==strcmp(jd->d_dialog->remote_tag, tag->gvalue))
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_INFO2, NULL,
			       "eXosip: found established early dialog for this subscribe\n"));
		  jinfo->jd = jd;
		  break;
		}
	    }
	}
    }

  if (jd == NULL) /* This transaction initiate a dialog in the case of
		     SUBSCRIBE (else it would be attached to a "jd" element. */
    {
      /* allocate a jd */
      i = eXosip_dialog_init_as_uac(&jd, sip);
      if (i!=0)
	{
     OSIP_TRACE (osip_trace
	    (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: cannot establish a dialog\n"));
	  return;
	}
      ADD_ELEMENT(js->s_dialogs, jd);
      jinfo->jd = jd;
      eXosip_update();
      osip_transaction_set_your_instance(tr, jinfo);
    }
  else
    {
      osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
      osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
    }

  jd->d_STATE = JD_ESTABLISHED;
  /* look for the body information */

  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_ANSWERED, js, jd);
    if (je!=NULL)
      {
	report_event_with_status(je, sip);
      }
  }

}

static void cb_rcv2xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t    *jd;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t    *jn;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv2xx (id=%i)\r\n", tr->transactionid));

  if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH"))
    {
      eXosip_pub_t *pub;
      int i;
      i = _eXosip_pub_update(&pub, tr, sip);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"cb_rcv2xx (id=%i) No publication to update\r\n", tr->transactionid));
	}
      return;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER"))
    {
      eXosip_event_t *je;
      eXosip_reg_t *jreg=NULL;
      /* find matching j_reg */
      _eXosip_reg_find(&jreg, tr);
      if (jreg!=NULL)
	{
	  je = eXosip_event_init_for_reg(EXOSIP_REGISTRATION_SUCCESS, jreg);
	  if (je!=NULL)
	    {
	      report_event_with_status(je, sip);
	    }
	}
      return;
    }

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  jn = jinfo->jn;
  js = jinfo->js;

  if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS"))
    {
      if (jc==NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv2xx (id=%i) Error: no call or transaction info for OPTIONS transaction\r\n", tr->transactionid));
	  return;
	}
      else if (jc->c_out_options_tr==NULL)
	{
	  /* options is within a call */
	}
      report_call_event_with_status(EXOSIP_OPTIONS_ANSWERED, jc, jd, sip);
      return;
    }

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
    {
      cb_rcv2xx_4invite(tr, sip);
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      cb_rcv2xx_4subscribe(tr, sip);
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "BYE"))
    {
      if (jd!=NULL)
	jd->d_STATE = JD_TERMINATED;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "MESSAGE"))
    {
      eXosip_event_t *je;
      je = eXosip_event_init_for_message(EXOSIP_MESSAGE_SUCCESS, tr, sip);
      if (je!=NULL)
	report_event_with_status(je, sip);
      return;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "NOTIFY"))
    {
#ifdef SUPPORT_MSN
      osip_header_t  *expires;
      osip_message_header_get_byname(tr->orig_request, "expires",
				     0, &expires);
      if (expires==NULL || expires->hvalue==NULL)
	{
	  /* UNCOMPLIANT UA without a subscription-state header */
	}
      else if (0==osip_strcasecmp(expires->hvalue, "0"))
	{
	  /* delete the dialog! */
	  if (jn!=NULL)
	    {
	      REMOVE_ELEMENT(eXosip.j_notifies, jn);
	      eXosip_notify_free(jn);
	    }
	}
#else
      osip_header_t  *sub_state;
      osip_message_header_get_byname(tr->orig_request, "subscription-state",
				     0, &sub_state);
      if (sub_state==NULL || sub_state->hvalue==NULL)
	{
	  /* UNCOMPLIANT UA without a subscription-state header */
	}
      else if (0==osip_strncasecmp(sub_state->hvalue, "terminated", 10))
	{
	  /* delete the dialog! */
	  if (jn!=NULL)
	    {
	      REMOVE_ELEMENT(eXosip.j_notifies, jn);
	      eXosip_notify_free(jn);
	    }
	}
#endif
    }
}

void eXosip_delete_early_dialog(eXosip_dialog_t *jd)
{
  if (jd == NULL) /* bug? */
      return;

  /* an early dialog was created, but the call is not established */
  if (jd->d_dialog!=NULL && jd->d_dialog->state==DIALOG_EARLY)
    {
      osip_dialog_free(jd->d_dialog);
      jd->d_dialog = NULL;
      eXosip_dialog_set_state(jd, JD_TERMINATED);
    }
}

static void
rcvregister_failure(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_event_t *je;
  eXosip_reg_t *jreg=NULL;
  /* find matching j_reg */
  _eXosip_reg_find(&jreg, tr);
  if (jreg!=NULL)
    {
      je = eXosip_event_init_for_reg(EXOSIP_REGISTRATION_FAILURE, jreg);
      report_event_with_status(je, sip);
    }
}

static void cb_rcv3xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv3xx (id=%i)\r\n", tr->transactionid));

  if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH"))
    {
      eXosip_pub_t *pub;
      int i;
      i = _eXosip_pub_update(&pub, tr, sip);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"cb_rcv3xx (id=%i) No publication to update\r\n", tr->transactionid));
	}
      return;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER"))
    {
      rcvregister_failure(type, tr, sip);
      return;
    }

  if (jinfo==NULL) return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  js = jinfo->js;

  if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS"))
    {
      if (jc==NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv3xx (id=%i) Error: no call or transaction info for INFO transaction\r\n", tr->transactionid));
	  return;
	}
      else if (jc->c_out_options_tr==NULL)
	{
	  /* options is within a call */
	}

      report_call_event_with_status(EXOSIP_OPTIONS_REDIRECTED, jc, jd, sip);
      return;
    }

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
    {
      report_call_event_with_status(EXOSIP_CALL_REDIRECTED, jc, jd, sip);
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "MESSAGE"))
    {      
      eXosip_event_t *je;
      je = eXosip_event_init_for_message(EXOSIP_MESSAGE_FAILURE, tr, sip);
      if (je)
	report_event_with_status(je, sip);
      return;
    }    
  else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_event_t *je;
      je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_REDIRECTED, js, jd);
      if (je)
	report_event_with_status(je, sip);
    }
  
  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
      if (jd->d_dialog==NULL)
	jd->d_STATE = JD_REDIRECTED;
    }

}

static void cb_rcv4xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv4xx (id=%i)\r\n", tr->transactionid));

  if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH"))
    {
      eXosip_pub_t *pub;
      int i;
      i = _eXosip_pub_update(&pub, tr, sip);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"cb_rcv4xx (id=%i) No publication to update\r\n", tr->transactionid));
	}
      return;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER"))
    {
      rcvregister_failure(type, tr, sip);
      return;
    }

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  js = jinfo->js;

  if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS"))
    {
      if (jc==NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv4xx (id=%i) Error: no call or transaction info for INFO transaction\r\n", tr->transactionid));
	  return;
	}
      else if (jc->c_out_options_tr==NULL)
	{
	  /* options is within a call */
	}

      report_call_event_with_status(EXOSIP_OPTIONS_REQUESTFAILURE, jc, jd, sip);      
      return;
    }

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
    report_call_event_with_status(EXOSIP_CALL_REQUESTFAILURE, jc, jd, sip);      
  }
  else if (MSG_IS_RESPONSE_FOR(sip, "MESSAGE"))
  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_message(EXOSIP_MESSAGE_FAILURE, tr, sip);
    if (je!=NULL)
      report_event_with_status(je, sip);
    return;
  }
  else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_REQUESTFAILURE, js, jd);
    if (je!=NULL)
      report_event_with_status(je, sip);
  }

  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
      if (MSG_TEST_CODE(sip, 401) || MSG_TEST_CODE(sip, 407))
	jd->d_STATE = JD_AUTH_REQUIRED;
      else
	jd->d_STATE = JD_CLIENTERROR;
    }

}

static void cb_rcv5xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv5xx (id=%i)\r\n", tr->transactionid));

  if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH"))
    {
      eXosip_pub_t *pub;
      int i;
      i = _eXosip_pub_update(&pub, tr, sip);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"cb_rcv3xx (id=%i) No publication to update\r\n", tr->transactionid));
	}
      return;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER"))
    {
      rcvregister_failure(type, tr, sip);
      return;
    }

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  js = jinfo->js;

  if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS"))
    {
      if (jc==NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv5xx (id=%i) Error: no call or transaction info for INFO transaction\r\n", tr->transactionid));
	  return;
	}
      else if (jc->c_out_options_tr==NULL)
	{
	  /* options is within a call */
	}
    
      report_call_event_with_status(EXOSIP_OPTIONS_SERVERFAILURE, jc, jd, sip);      
      return;
    }

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
    report_call_event_with_status(EXOSIP_CALL_SERVERFAILURE, jc, jd, sip);      
  }
  else if (MSG_IS_RESPONSE_FOR(sip, "MESSAGE"))
  {
      eXosip_event_t *je;
      je = eXosip_event_init_for_message(EXOSIP_MESSAGE_FAILURE, tr, sip);
      if (je!=NULL)
	report_event_with_status(je, sip);
      return;
  }    
  else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_SERVERFAILURE, js, jd);
    if (je!=NULL)
      report_event_with_status(je, sip);
  }

  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
      jd->d_STATE = JD_SERVERERROR;
    }

}

static void cb_rcv6xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv6xx (id=%i)\r\n", tr->transactionid));

  if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH"))
    {
      eXosip_pub_t *pub;
      int i;
      i = _eXosip_pub_update(&pub, tr, sip);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"cb_rcv6xx (id=%i) No publication to update\r\n", tr->transactionid));
	}
      return;
    }
  else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER"))
    {
      rcvregister_failure(type, tr, sip);
      return;
    }

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  js = jinfo->js;

  if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS"))
    {
      if (jc==NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv6xx (id=%i) Error: no call or transaction info for INFO transaction\r\n", tr->transactionid));
	  return;
	}
      else if (jc->c_out_options_tr==NULL)
	{
	  /* options is within a call */
	}
      report_call_event_with_status(EXOSIP_OPTIONS_GLOBALFAILURE, jc, jd, sip);
      return;
    }

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
      report_call_event_with_status(EXOSIP_CALL_GLOBALFAILURE, jc, jd, sip);
  }
  else if (MSG_IS_RESPONSE_FOR(sip, "MESSAGE"))
  {
      eXosip_event_t *je;
      je = eXosip_event_init_for_message(EXOSIP_MESSAGE_FAILURE, tr, sip);
      if (je!=NULL)
	report_event_with_status(je, sip);
      return;
  }    
  else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_GLOBALFAILURE, js, jd);
    if (je!=NULL)
      report_event_with_status(je, sip);
  }

  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
      jd->d_STATE = JD_GLOBALFAILURE;
    }

}

static void cb_snd1xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd1xx (id=%i)\r\n", tr->transactionid));

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  jd->d_STATE = JD_TRYING;
}

static void cb_snd2xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd2xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      jd->d_STATE = JD_ESTABLISHED;
      return;
    }
  jd->d_STATE = JD_ESTABLISHED;
}

static void cb_snd3xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd3xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_REDIRECTED;

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
    report_call_event_with_status(EXOSIP_CALL_CLOSED, jc, jd, sip);
  }
}

static void cb_snd4xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd4xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_CLIENTERROR;

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
    report_call_event_with_status(EXOSIP_CALL_CLOSED, jc, jd, sip);
  }

}

static void cb_snd5xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd5xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_SERVERERROR;

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
    report_call_event_with_status(EXOSIP_CALL_CLOSED, jc, jd, sip);
  }

}

static void cb_snd6xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd6xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
      || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_GLOBALFAILURE;

  if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
  {
    report_call_event_with_status(EXOSIP_CALL_CLOSED, jc, jd, sip);
  }

}

static void cb_rcvresp_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvresp_retransmission (id=%i)\r\n", tr->transactionid));
}

static void cb_sndreq_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndreq_retransmission (id=%i)\r\n", tr->transactionid));
}

static void cb_sndresp_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndresp_retransmission (id=%i)\r\n", tr->transactionid));
}

static void cb_rcvreq_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvreq_retransmission (id=%i)\r\n", tr->transactionid));
}

static void cb_transport_error(int type, osip_transaction_t *tr, int error)
{
  eXosip_dialog_t    *jd;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t    *jn;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_transport_error (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  jn = jinfo->jn;
  js = jinfo->js;

  if (jn==NULL && js==NULL)
    return;

  if (MSG_IS_NOTIFY(tr->orig_request)
      && type==OSIP_NICT_TRANSPORT_ERROR)
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_notifies, jn);
      eXosip_notify_free(jn);
    }

  if (MSG_IS_SUBSCRIBE(tr->orig_request)
      && type==OSIP_NICT_TRANSPORT_ERROR)
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_subscribes, js);
      eXosip_subscribe_free(js);
    }

  if (MSG_IS_OPTIONS(tr->orig_request) && jc->c_dialogs==NULL
      && type==OSIP_NICT_TRANSPORT_ERROR)
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_calls, jc);
      eXosip_call_free(jc);
    }
}



int
eXosip_set_callbacks(osip_t *osip)
{
  /* register all callbacks */

  osip_set_cb_send_message(osip, &cb_udp_snd_message);
  
  osip_set_kill_transaction_callback(osip ,OSIP_ICT_KILL_TRANSACTION,
				 &cb_ict_kill_transaction);
  osip_set_kill_transaction_callback(osip ,OSIP_IST_KILL_TRANSACTION,
				 &cb_ist_kill_transaction);
  osip_set_kill_transaction_callback(osip ,OSIP_NICT_KILL_TRANSACTION,
				 &cb_nict_kill_transaction);
  osip_set_kill_transaction_callback(osip ,OSIP_NIST_KILL_TRANSACTION,
				 &cb_nist_kill_transaction);
          
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_message_callback(osip ,OSIP_ICT_INVITE_SENT_AGAIN,
			&cb_sndreq_retransmission);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_2XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_3456XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_message_callback(osip ,OSIP_IST_INVITE_RECEIVED_AGAIN,
			&cb_rcvreq_retransmission);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_message_callback(osip ,OSIP_NICT_REQUEST_SENT_AGAIN,
			&cb_sndreq_retransmission);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_2XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_3456XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_message_callback(osip ,OSIP_NIST_REQUEST_RECEIVED_AGAIN,
			&cb_rcvreq_retransmission);
          
  osip_set_transport_error_callback(osip ,OSIP_ICT_TRANSPORT_ERROR,
				    &cb_transport_error);
  osip_set_transport_error_callback(osip ,OSIP_IST_TRANSPORT_ERROR,
				    &cb_transport_error);
  osip_set_transport_error_callback(osip ,OSIP_NICT_TRANSPORT_ERROR,
				    &cb_transport_error);
  osip_set_transport_error_callback(osip ,OSIP_NIST_TRANSPORT_ERROR,
				    &cb_transport_error);
  
  osip_set_message_callback(osip ,OSIP_ICT_INVITE_SENT,     &cb_sndinvite);
  osip_set_message_callback(osip ,OSIP_ICT_ACK_SENT,        &cb_sndack);
  osip_set_message_callback(osip ,OSIP_NICT_REGISTER_SENT,  &cb_sndregister);
  osip_set_message_callback(osip ,OSIP_NICT_BYE_SENT,       &cb_sndbye);
  osip_set_message_callback(osip ,OSIP_NICT_CANCEL_SENT,    &cb_sndcancel);
  osip_set_message_callback(osip ,OSIP_NICT_INFO_SENT,      &cb_sndinfo);
  osip_set_message_callback(osip ,OSIP_NICT_OPTIONS_SENT,   &cb_sndoptions);
  osip_set_message_callback(osip ,OSIP_NICT_SUBSCRIBE_SENT, &cb_sndsubscribe);
  osip_set_message_callback(osip ,OSIP_NICT_NOTIFY_SENT,    &cb_sndnotify);
  /*  osip_set_cb_nict_sndprack   (osip,&cb_sndprack); */
  osip_set_message_callback(osip ,OSIP_NICT_UNKNOWN_REQUEST_SENT, &cb_sndunkrequest);

  osip_set_message_callback(osip ,OSIP_ICT_STATUS_1XX_RECEIVED, &cb_rcv1xx);
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_2XX_RECEIVED, &cb_rcv2xx);
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_3XX_RECEIVED, &cb_rcv3xx);
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_4XX_RECEIVED, &cb_rcv4xx);
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_5XX_RECEIVED, &cb_rcv5xx);
  osip_set_message_callback(osip ,OSIP_ICT_STATUS_6XX_RECEIVED, &cb_rcv6xx);
  
  osip_set_message_callback(osip ,OSIP_IST_STATUS_1XX_SENT, &cb_snd1xx);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_2XX_SENT, &cb_snd2xx);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_3XX_SENT, &cb_snd3xx);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_4XX_SENT, &cb_snd4xx);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_5XX_SENT, &cb_snd5xx);
  osip_set_message_callback(osip ,OSIP_IST_STATUS_6XX_SENT, &cb_snd6xx);
  
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_1XX_RECEIVED, &cb_rcv1xx);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_2XX_RECEIVED, &cb_rcv2xx);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_3XX_RECEIVED, &cb_rcv3xx);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_4XX_RECEIVED, &cb_rcv4xx);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_5XX_RECEIVED, &cb_rcv5xx);
  osip_set_message_callback(osip ,OSIP_NICT_STATUS_6XX_RECEIVED, &cb_rcv6xx);
      
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_1XX_SENT, &cb_snd1xx);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_2XX_SENT, &cb_snd2xx);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_3XX_SENT, &cb_snd3xx);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_4XX_SENT, &cb_snd4xx);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_5XX_SENT, &cb_snd5xx);
  osip_set_message_callback(osip ,OSIP_NIST_STATUS_6XX_SENT, &cb_snd6xx);
  
  osip_set_message_callback(osip ,OSIP_IST_INVITE_RECEIVED,     &cb_rcvinvite);
  osip_set_message_callback(osip ,OSIP_IST_ACK_RECEIVED,        &cb_rcvack);
  osip_set_message_callback(osip ,OSIP_IST_ACK_RECEIVED_AGAIN,  &cb_rcvack2);
  osip_set_message_callback(osip ,OSIP_NIST_REGISTER_RECEIVED,  &cb_rcvregister);
  osip_set_message_callback(osip ,OSIP_NIST_BYE_RECEIVED,       &cb_rcvbye);
  osip_set_message_callback(osip ,OSIP_NIST_CANCEL_RECEIVED,    &cb_rcvcancel);
  osip_set_message_callback(osip ,OSIP_NIST_INFO_RECEIVED,      &cb_rcvinfo);
  osip_set_message_callback(osip ,OSIP_NIST_OPTIONS_RECEIVED,   &cb_rcvoptions);
  osip_set_message_callback(osip ,OSIP_NIST_SUBSCRIBE_RECEIVED, &cb_rcvsubscribe);
  osip_set_message_callback(osip ,OSIP_NIST_NOTIFY_RECEIVED,    &cb_rcvnotify);
  osip_set_message_callback(osip ,OSIP_NIST_UNKNOWN_REQUEST_RECEIVED, &cb_rcvunkrequest);


  return 0;
}
