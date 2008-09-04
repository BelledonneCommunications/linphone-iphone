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
#include <eXosip_cfg.h>

#include <osip2/osip_mt.h>
#include <osip2/osip_condv.h>

#ifndef  WIN32
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

/* Private functions */
static int eXosip_create_transaction(eXosip_call_t *jc, eXosip_dialog_t *jd,
				     osip_message_t *request);
static int eXosip_create_cancel_transaction(eXosip_call_t *jc,
					    eXosip_dialog_t *jd,
					    osip_message_t *request);
static jauthinfo_t *eXosip_find_authentication_info(const char *username,
						    const char *realm);
static int eXosip_add_authentication_information(osip_message_t *req,
						 osip_message_t *last_response);
static void *eXosip_thread(void *arg);
static int eXosip_execute(void);
static osip_message_t *eXosip_prepare_request_for_auth(osip_message_t *msg);

static int eXosip_update_top_via(osip_message_t *sip);


eXosip_t eXosip;

void eXosip_set_firewallip(const char *firewall_address)
{
	if (firewall_address==NULL) return;
	snprintf(eXosip.j_firewall_ip,50, "%s", firewall_address);
}

void eXosip_set_nattype(const char *nat_type)
{
    osip_strncpy(eXosip.nat_type, (nat_type ? nat_type : ""), sizeof(eXosip.nat_type)-1);
}

void eXosip_force_proxy(const char *proxyurl)
{
    osip_strncpy(eXosip.forced_proxy, (proxyurl ? proxyurl : ""),  sizeof(eXosip.forced_proxy)-1);
}

int eXosip_guess_localip(int family, char *address, int size)
{
  return eXosip_guess_ip_for_via(family, address, size);
}

void eXosip_get_localip(char *ip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_ERROR, NULL,
	       "eXosip_get_localip IS DEPRECATED. Use eXosip_guess_localip!\n"));
  eXosip_guess_ip_for_via(AF_INET, ip, 15);
}

int 
eXosip_is_public_address(const char *c_address)
{
    return (0!=strncmp(c_address, "192.168",7)
	    && 0!=strncmp(c_address, "10.",3)
	    && 0!=strncmp(c_address, "172.16.",7)
	    && 0!=strncmp(c_address, "172.17.",7)
	    && 0!=strncmp(c_address, "172.18.",7)
	    && 0!=strncmp(c_address, "172.19.",7)
	    && 0!=strncmp(c_address, "172.20.",7)
	    && 0!=strncmp(c_address, "172.21.",7)
	    && 0!=strncmp(c_address, "172.22.",7)
	    && 0!=strncmp(c_address, "172.23.",7)
	    && 0!=strncmp(c_address, "172.24.",7)
	    && 0!=strncmp(c_address, "172.25.",7)
	    && 0!=strncmp(c_address, "172.26.",7)
	    && 0!=strncmp(c_address, "172.27.",7)
	    && 0!=strncmp(c_address, "172.28.",7)
	    && 0!=strncmp(c_address, "172.29.",7)
	    && 0!=strncmp(c_address, "172.30.",7)
	    && 0!=strncmp(c_address, "172.31.",7)
	    && 0!=strncmp(c_address, "169.254",7));
}


void __eXosip_wakeup(void)
{
  jpipe_write(eXosip.j_socketctl, "w", 1);
}

void __eXosip_wakeup_event(void)
{
  jpipe_write(eXosip.j_socketctl_event, "w", 1);
}


int
eXosip_lock(void)
{
  return osip_mutex_lock((struct osip_mutex*)eXosip.j_mutexlock);
}

int
eXosip_unlock(void)
{
  return osip_mutex_unlock((struct osip_mutex*)eXosip.j_mutexlock);
}


jfriend_t *jfriend_get(void)
{
  return eXosip.j_friends;
}

void jfriend_remove(jfriend_t *fr)
{
  REMOVE_ELEMENT(eXosip.j_friends, fr);
}

jsubscriber_t *jsubscriber_get(void)
{
  return eXosip.j_subscribers;
}

jidentity_t *jidentity_get(void)
{
  return eXosip.j_identitys;
}

void
eXosip_kill_transaction (osip_list_t * transactions)
{
  osip_transaction_t *transaction;

  if (!osip_list_eol (transactions, 0))
    {
      /* some transaction are still used by osip,
         transaction should be released by modules! */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: _osip_kill_transaction transaction should be released by modules!\n"));
    }

  while (!osip_list_eol (transactions, 0))
    {
      transaction = osip_list_get (transactions, 0);

      __eXosip_delete_jinfo(transaction);
      osip_transaction_free (transaction);
    }
}

void eXosip_quit(void)
{
  jauthinfo_t   *jauthinfo;
  eXosip_call_t *jc;
  eXosip_notify_t *jn;
  eXosip_subscribe_t *js;
  eXosip_reg_t  *jreg;
  eXosip_pub_t  *jpub;
  int i;

  eXosip.j_stop_ua = 1; /* ask to quit the application */
  __eXosip_wakeup();
  __eXosip_wakeup_event();

  i = osip_thread_join((struct osip_thread*)eXosip.j_thread);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: can't terminate thread!"));
    }
  osip_free((struct osip_thread*)eXosip.j_thread);

  jpipe_close(eXosip.j_socketctl);
  jpipe_close(eXosip.j_socketctl_event);

  osip_free(eXosip.localip);
  osip_free(eXosip.localport);
  osip_free(eXosip.user_agent);

  eXosip.j_input = 0;
  eXosip.j_output = 0;

  for (jc = eXosip.j_calls; jc!=NULL;jc = eXosip.j_calls)
    {
      REMOVE_ELEMENT(eXosip.j_calls, jc);
      eXosip_call_free(jc);
    }

  for (js = eXosip.j_subscribes; js!=NULL;js = eXosip.j_subscribes)
    {
      REMOVE_ELEMENT(eXosip.j_subscribes, js);
      eXosip_subscribe_free(js);
    }

  for (jn = eXosip.j_notifies; jn!=NULL;jn = eXosip.j_notifies)
    {
      REMOVE_ELEMENT(eXosip.j_notifies, jn);
      eXosip_notify_free(jn);
    }
  
  osip_mutex_destroy((struct osip_mutex*)eXosip.j_mutexlock);
  osip_cond_destroy((struct osip_cond*)eXosip.j_cond);

  eXosip_sdp_negotiation_free(eXosip.osip_negotiation);

  if (eXosip.j_input)
    fclose(eXosip.j_input);
  if (eXosip.j_output)
    osip_free(eXosip.j_output);
  if (eXosip.j_socket)
    close(eXosip.j_socket);

  for (jreg = eXosip.j_reg; jreg!=NULL; jreg = eXosip.j_reg)
    {
      REMOVE_ELEMENT(eXosip.j_reg, jreg);
      eXosip_reg_free(jreg);
    }

  for (jpub = eXosip.j_pub; jpub!=NULL; jpub = eXosip.j_pub)
    {
      REMOVE_ELEMENT(eXosip.j_pub, jpub);
      _eXosip_pub_free(jpub);
    }

  /* should be moved to method with an argument */
  jfriend_unload();
  jidentity_unload();
  jsubscriber_unload();

  /*
  for (jid = eXosip.j_identitys; jid!=NULL; jid = eXosip.j_identitys)
    {
      REMOVE_ELEMENT(eXosip.j_identitys, jid);
      eXosip_friend_free(jid);
    }

  for (jfr = eXosip.j_friends; jfr!=NULL; jfr = eXosip.j_friends)
    {
      REMOVE_ELEMENT(eXosip.j_friends, jfr);
      eXosip_reg_free(jfr);
    }
  */

  while (!osip_list_eol(eXosip.j_transactions, 0))
    {
      osip_transaction_t *tr = (osip_transaction_t*) osip_list_get(eXosip.j_transactions, 0);
      if (tr->state==IST_TERMINATED || tr->state==ICT_TERMINATED
	  || tr->state== NICT_TERMINATED || tr->state==NIST_TERMINATED)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		      "Release a terminated transaction\n"));
	  osip_list_remove(eXosip.j_transactions, 0);
	  __eXosip_delete_jinfo(tr);
	  osip_transaction_free(tr);
	}
      else
	{
	  osip_list_remove(eXosip.j_transactions, 0);
	  __eXosip_delete_jinfo(tr);
	  osip_transaction_free(tr);
	}
    }

  osip_free(eXosip.j_transactions);

  eXosip_kill_transaction (eXosip.j_osip->osip_ict_transactions);
  eXosip_kill_transaction (eXosip.j_osip->osip_nict_transactions);
  eXosip_kill_transaction (eXosip.j_osip->osip_ist_transactions);
  eXosip_kill_transaction (eXosip.j_osip->osip_nist_transactions);
  osip_release (eXosip.j_osip);

  if (eXosip.j_events!=NULL)
    {
      eXosip_event_t *ev;
      for(ev=osip_fifo_tryget(eXosip.j_events);ev!=NULL;
	  ev=osip_fifo_tryget(eXosip.j_events))
	eXosip_event_free(ev);
    }

  osip_fifo_free(eXosip.j_events);

  for (jauthinfo = eXosip.authinfos; jauthinfo!=NULL;
       jauthinfo = eXosip.authinfos)
    {
      REMOVE_ELEMENT(eXosip.authinfos, jauthinfo);
      osip_free(jauthinfo);
    }

  return ;
}

static int eXosip_execute ( void )
{
  struct timeval lower_tv;
  int i;

  osip_timers_gettimeout(eXosip.j_osip, &lower_tv);
  if (lower_tv.tv_sec>15)
    {
      lower_tv.tv_sec = 15;
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "eXosip: Reseting timer to 15s before waking up!\n"));
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "eXosip: timer sec:%i usec:%i!\n",
		   lower_tv.tv_sec, lower_tv.tv_usec));
    }
  i = eXosip_read_message(1, lower_tv.tv_sec, lower_tv.tv_usec);

  if (i==-2)
    {
      return -2;
    }

  eXosip_lock();
  osip_timers_ict_execute(eXosip.j_osip);
  osip_timers_nict_execute(eXosip.j_osip);
  osip_timers_ist_execute(eXosip.j_osip);
  osip_timers_nist_execute(eXosip.j_osip);
  
  osip_ict_execute(eXosip.j_osip);
  osip_nict_execute(eXosip.j_osip);
  osip_ist_execute(eXosip.j_osip);
  osip_nist_execute(eXosip.j_osip);
  
  /* free all Calls that are in the TERMINATED STATE? */
  eXosip_release_terminated_calls();

  eXosip_unlock();

  return 0;
}

void *eXosip_thread        ( void *arg )
{
  int i;
  while (eXosip.j_stop_ua==0)
    {
      i = eXosip_execute();
      if (i==-2)
	osip_thread_exit();
    }
  osip_thread_exit();
  return NULL;
}

static int ipv6_enable = 0;

void eXosip_enable_ipv6(int _ipv6_enable)
{
  ipv6_enable = _ipv6_enable;
}

int eXosip_init(FILE *input, FILE *output, int port)
{
  osip_t *osip;
  int i;
  if (port<=0)
    {
     OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: port must be higher than 0!\n"));
      return -1;
    }
  memset(&eXosip, 0, sizeof(eXosip));
  eXosip.localip = (char *) osip_malloc(50);
  memset(eXosip.localip, '\0', 50);

  if (ipv6_enable == 0)
    eXosip.ip_family = AF_INET;
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "IPv6 is enabled. Pls report bugs\n"));
      eXosip.ip_family = AF_INET6;
    }

  eXosip_guess_localip(eXosip.ip_family, eXosip.localip, 49);
  if (eXosip.localip[0]=='\0')
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No ethernet interface found!\n"));
      OSIP_TRACE (osip_trace
        (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: using 127.0.0.1 (debug mode)!\n"));
      strcpy(eXosip.localip, "127.0.0.1");
		/* we should always fallback on something. The linphone user will surely
		start linphone BEFORE setting its dial up connection.*/
    }
  eXosip.user_agent = osip_strdup("eXosip/" EXOSIP_VERSION);

  eXosip_set_mode(EVENT_MODE);
  eXosip.j_input = input;
  eXosip.j_output = output;
  eXosip.j_calls = NULL;
  eXosip.j_stop_ua = 0;
  eXosip.j_thread = NULL;
  eXosip.j_transactions = (osip_list_t*) osip_malloc(sizeof(osip_list_t));
  osip_list_init(eXosip.j_transactions);
  eXosip.j_reg = NULL;

  eXosip.j_cond      = (struct osip_cond*)osip_cond_init();

  eXosip.j_mutexlock = (struct osip_mutex*)osip_mutex_init();

  if (-1==osip_init(&osip))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: Cannot initialize osip!\n"));
      return -1;
    }

  eXosip_sdp_negotiation_init(&(eXosip.osip_negotiation));

  eXosip_sdp_negotiation_add_codec(osip_strdup("0"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("0 PCMU/8000"));

  eXosip_sdp_negotiation_add_codec(osip_strdup("8"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("8 PCMA/8000"));

  osip_set_application_context(osip, &eXosip);
  
  eXosip_set_callbacks(osip);
  
  eXosip.j_osip = osip;

#ifdef WIN32
   /* Initializing windows socket library */
   {
     WORD wVersionRequested;
     WSADATA wsaData;

     wVersionRequested = MAKEWORD(1,1);
     if(i = WSAStartup(wVersionRequested,  &wsaData))
       {
         OSIP_TRACE (osip_trace
		     (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: Unable to initialize WINSOCK, reason: %d\n",i));
	 /* return -1; It might be already initilized?? */
       }
   }
#endif

  /* open a TCP socket to wake up the application when needed. */
  eXosip.j_socketctl = jpipe();
  if (eXosip.j_socketctl==NULL)
    return -1;

  eXosip.j_socketctl_event = jpipe();
  if (eXosip.j_socketctl_event==NULL)
    return -1;
  if (ipv6_enable == 0)
    {
      eXosip.j_socket = (int)socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (eXosip.j_socket==-1)
	return -1;
      
      {
	struct sockaddr_in  raddr;
	raddr.sin_addr.s_addr = htons(INADDR_ANY);
	raddr.sin_port = htons((short)port);
	raddr.sin_family = AF_INET;
	
	i = bind(eXosip.j_socket, (struct sockaddr *)&raddr, sizeof(raddr));
	if (i < 0)
	  {
       OSIP_TRACE (osip_trace
		   (__FILE__, __LINE__, OSIP_ERROR, NULL,
	       "eXosip: Cannot bind on port: %i!\n", i));
	    return -1;
	  }
      }
    }
  else
    {
      eXosip.j_socket = (int)socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      if (eXosip.j_socket==-1)
	return -1;
      
      {
	struct sockaddr_in6  raddr;
	memset(&raddr, 0, sizeof(raddr));
	raddr.sin6_port = htons((short)port);
	raddr.sin6_family = AF_INET6;
	
	i = bind(eXosip.j_socket, (struct sockaddr *)&raddr, sizeof(raddr));
	if (i < 0)
	  {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
	      "eXosip: Cannot bind on port: %i!\n", i));
	    return -1;
	  }
      }
    }


  eXosip.localport = (char*)osip_malloc(10);
  sprintf(eXosip.localport, "%i", port);

  eXosip.j_thread = (void*) osip_thread_create(20000,eXosip_thread, NULL);
  if (eXosip.j_thread==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: Cannot start thread!\n"));
      return -1;
    }

  /* To be changed in osip! */
  eXosip.j_events = (osip_fifo_t*) osip_malloc(sizeof(osip_fifo_t));
  osip_fifo_init(eXosip.j_events);

  jfriend_load();
  jidentity_load();
  jsubscriber_load();
  return 0;
}

int eXosip_force_localip(const char *localip){
	if (localip!=NULL){
		strcpy(eXosip.localip,localip);
		eXosip.forced_localip=1;
	}else {
		eXosip_guess_ip_for_via(eXosip.ip_family, eXosip.localip, 49);
		eXosip.forced_localip=0;
	}
	return 0;
}

void
eXosip_set_mode(int mode)
{
  eXosip.j_runtime_mode = mode;
}

void
eXosip_set_user_agent(const char *user_agent)
{
  osip_free(eXosip.user_agent);
  eXosip.user_agent = osip_strdup(user_agent);
}

void
eXosip_automatic_refresh()
{
#if 0
  eXosip_call_t      *jc;
  eXosip_notify_t    *jn;
#endif
  eXosip_subscribe_t *js;
  eXosip_dialog_t    *jd;
  eXosip_reg_t       *jr;
  int now;
  
  now = time(NULL);
  
#if 0
  for (jc=eXosip.j_calls; jc!=NULL; jc=jc->next)
    {
      if (jc->c_id<1)
	{
	}
      for (jd=jc->c_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      /* sending update for keep-alive mechanism could be done here! */
	    }
	}
    }
#endif
  
  for (js=eXosip.j_subscribes; js!=NULL; js=js->next)
    {
      for (jd=js->s_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id>=1)
		{
		  if (eXosip_subscribe_need_refresh(js, now)==0)
		    {
		      int i;
#ifdef LOW_EXPIRE
		      i = eXosip_subscribe_send_subscribe(js, jd, "120");
#else
		      i = eXosip_subscribe_send_subscribe(js, jd, "3600");
#endif
		    }
		}
	    }
	}
    }
  
#if 0
  for (jn=eXosip.j_notifies; jn!=NULL; jn=jn->next)
    {
      if (jn->n_id<1)
	{
	  jn->n_id = static_id;
	  static_id++;
	}
      for (jd=jn->n_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id>=1)
		{
		  /* keep-alive of dialog should be done by subscribe */
		  /* may be we can decide to close the subscribption
		     if it is expired */
		}
	    }
	}
    }
#endif

    for (jr = eXosip.j_reg; jr != NULL; jr = jr->next)
      {
	if (jr->r_id >=1 || jr->r_last_tr!=NULL)
	  {
	    if (jr->r_reg_period==0)
	      {
		/* skip refresh! */
	      }
	    else if (now-jr->r_last_tr->birth_time>300)
	      {
		/* automatic refresh */
		eXosip_register(jr->r_id, jr->r_reg_period);
	      }
	    else if (now-jr->r_last_tr->birth_time>jr->r_reg_period-60)
	      {
		/* automatic refresh */
		eXosip_register(jr->r_id, jr->r_reg_period);
	      }
	    else if (now-jr->r_last_tr->birth_time>120 &&
		     (jr->r_last_tr->last_response==NULL
		      || (!MSG_IS_STATUS_2XX(jr->r_last_tr->last_response))))
	      {
		/* automatic refresh */
		eXosip_register(jr->r_id, jr->r_reg_period);
	      }
	  }
      }
}

void
eXosip_update()
{
  static int static_id  = 1;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t *jn;
  eXosip_dialog_t    *jd;
  int now;

  if (static_id>100000)
    static_id = 1; /* loop */

  now = time(NULL);
  for (jc=eXosip.j_calls; jc!=NULL; jc=jc->next)
    {
      if (jc->c_id<1)
	{
	  jc->c_id = static_id;
	  static_id++;
	}
      for (jd=jc->c_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id<1)
		{
		  jd->d_id = static_id;
		  static_id++;
		}
	    }
	  else jd->d_id = -1;
	}
    }

  for (js=eXosip.j_subscribes; js!=NULL; js=js->next)
    {
      if (js->s_id<1)
	{
	  js->s_id = static_id;
	  static_id++;
	}
      for (jd=js->s_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id<1)
		{
		  jd->d_id = static_id;
		  static_id++;
		}
	    }
	  else
	    jd->d_id = -1;
	}
    }

  for (jn=eXosip.j_notifies; jn!=NULL; jn=jn->next)
    {
      if (jn->n_id<1)
	{
	  jn->n_id = static_id;
	  static_id++;
	}
      for (jd=jn->n_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id<1)
		{
		  jd->d_id = static_id;
		  static_id++;
		}
	    }
	  else jd->d_id = -1;
	}
    }
}

int eXosip_message    (char *to, char *from, char *route, char *buff)
{
  /* eXosip_call_t *jc;
     osip_header_t *subject; */
  osip_message_t *message;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;

  i = generating_message(&message, to, from, route, buff);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot send message (cannot build MESSAGE)! "));
      return -1;
    }

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       message);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(message);
      return -1;
    }
  
  osip_list_add(eXosip.j_transactions, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(message);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_info_call(int jid, char *content_type, char *body)
{
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *info;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  int i;
  
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (jd==NULL || jd->d_dialog==NULL)
    {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No established dialog!"));
      return -1;
    }

  transaction = eXosip_find_last_options(jc, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
      transaction=NULL;
    }
 
  i = generating_info_within_dialog(&info, jd->d_dialog);
  if (i!=0)
    {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot send info message! "));
      return -2;
    }
  
  osip_message_set_content_type(info, content_type);
  osip_message_set_body(info, body, strlen(body));
  
  i = osip_transaction_init(&transaction,
			    NICT,
			    eXosip.j_osip,
			    info);
  if (i!=0)
    {
      osip_message_free(info);
      return -1;
    }

  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(info);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_initiate_call_with_body(osip_message_t *invite,const char *bodytype, const char*body, void *reference){
	eXosip_call_t *jc;
  osip_header_t *subject;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
  char *size;
  
  if (body!=NULL){
    size= (char *)osip_malloc(7*sizeof(char));
	sprintf(size,"%i",strlen(body));
	osip_message_set_content_length(invite, size);
	osip_free(size);
	osip_message_set_body(invite, body, strlen(body));
	osip_message_set_content_type(invite,bodytype);
  }
  else osip_message_set_content_length(invite, "0");

  eXosip_call_init(&jc);
  i = osip_message_get_subject(invite, 0, &subject);
  if (subject!=NULL && subject->hvalue!=NULL && subject->hvalue[0]!='\0')
    snprintf(jc->c_subject, 99, "%s", subject->hvalue);

  jc->c_ack_sdp = 0;

  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      eXosip_call_free(jc);
      osip_message_free(invite);
      return -1;
    }
  
  jc->c_out_tr = transaction;
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);

  jc->external_reference = reference;
  ADD_ELEMENT(eXosip.j_calls, jc);

  eXosip_update(); /* fixed? */
  __eXosip_wakeup();
  return jc->c_id;
}

osip_message_t *eXosip_prepare_request_for_auth(osip_message_t *msg)
{
#ifdef SM
  char *locip;
#else
  char locip[50];
#endif
  osip_message_t *newmsg;
  int  cseq;
  char tmp[90];
  osip_via_t *via;
  
  osip_message_clone(msg,&newmsg);
  if (newmsg==NULL){
    eXosip_trace(OSIP_INFO1,("eXosip_prepare_request_for_auth: could not clone msg."));
    return NULL;
  }
  via = (osip_via_t *) osip_list_get (newmsg->vias, 0);
  if (via==NULL || newmsg->cseq==NULL || newmsg->cseq->number==NULL) {
    osip_message_free(newmsg);
    eXosip_trace(OSIP_INFO1,("eXosip_prepare_request_for_auth: Bad headers in previous request."));
    return NULL;
  }
  /* increment cseq */
  cseq=atoi(newmsg->cseq->number);
  osip_free(newmsg->cseq->number);
  newmsg->cseq->number=strdup_printf("%i",cseq+1);

  osip_list_remove(newmsg->vias, 0);
	locip=osip_strdup(osip_via_get_host(via));
  osip_via_free(via);
#ifdef SM
	
#else
  eXosip_guess_ip_for_via(eXosip.ip_family, locip, 49);
#endif
  if (eXosip.ip_family==AF_INET6)
    sprintf(tmp, "SIP/2.0/UDP [%s]:%s;branch=z9hG4bK%u",
	    locip,
	    eXosip.localport,
	    via_branch_new_random());
  else
    sprintf(tmp, "SIP/2.0/UDP %s:%s;rport;branch=z9hG4bK%u",
	    locip,
	    eXosip.localport,
	    via_branch_new_random());
  
#ifdef SM
  osip_free(locip);
#endif
  osip_via_init(&via);
  osip_via_parse(via, tmp);
  osip_list_add(newmsg->vias, via, 0);

  return newmsg;
}

int eXosip_retry_call(int cid)
{
  eXosip_call_t *jc=NULL;
  osip_transaction_t *tr,*newtr=NULL;
  osip_message_t *inv=NULL;
  int i;
  osip_event_t *sipevent;
  eXosip_call_find(cid,&jc);
  if (jc==NULL) {
    eXosip_trace(OSIP_INFO1,("eXosip_retry_last_invite: No such call."));
    return -1;
  }
  tr=eXosip_find_last_out_invite(jc,NULL);
  if (tr==NULL){
    eXosip_trace(OSIP_INFO1,("eXosip_retry_last_invite: No such transaction."));
    return -1;
  }
  if (tr->last_response==NULL){
    eXosip_trace(OSIP_INFO1,("eXosip_retry_last_invite: transaction has not been answered."));
    return -1;
  }
  inv=eXosip_prepare_request_for_auth(tr->orig_request);
  if (inv==NULL) return -1;
  eXosip_add_authentication_information(inv,tr->last_response);
  if (-1 == eXosip_update_top_via(inv))
    {
      osip_message_free(inv);
      return -1;
    }


  i = osip_transaction_init(&newtr,
		       ICT,
		       eXosip.j_osip,
		       inv);
  if (i!=0)
    {
      osip_message_free(inv);
      return -1;
    }
  jc->c_out_tr = newtr;
  
  sipevent = osip_new_outgoing_sipmessage(inv);
  
  osip_transaction_set_your_instance(newtr, __eXosip_new_jinfo(jc, NULL, NULL, NULL));
  osip_transaction_add_event(newtr, sipevent);

  eXosip_update(); /* fixed? */
  __eXosip_wakeup();
  return jc->c_id;
}

extern int cb_udp_snd_message(osip_transaction_t *tr, osip_message_t *sip,
			      char *host, int port, int out_socket);

int eXosip_send_ack(int did){
	eXosip_call_t *call;
	eXosip_dialog_t *dialog;
	eXosip_call_dialog_find(did,&call,&dialog);
	if (dialog!=NULL){
		if (dialog->d_ack!=NULL){
			osip_route_t *route;
			int port;
			char *host;
			osip_message_t *ack=dialog->d_ack;
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
		}
	}
	return 0;
}

extern osip_list_t *supported_codec;

int eXosip_initiate_call(osip_message_t *invite, void *reference,
			 void *sdp_context_reference,
			 char *local_sdp_port)
{
  eXosip_call_t *jc;
  osip_header_t *subject;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
  sdp_message_t *sdp;
  char *body;
  char *size;
  
  if (invite==NULL || invite->req_uri==NULL || invite->req_uri->host==NULL  ) return -1;
  
  if (local_sdp_port!=NULL)
    {
      osip_negotiation_sdp_build_offer(eXosip.osip_negotiation, NULL, &sdp, local_sdp_port, NULL);
      
      /*
	if speex codec is supported, add bandwith attribute:
	b=AS:110 20
	b=AS:111 20
      */
      if (sdp!=NULL)
	{
	  int pos=0;
	  while (!sdp_message_endof_media (sdp, pos))
	    {
	      int k = 0;
	      char *tmp = sdp_message_m_media_get (sdp, pos);
	      if (0 == osip_strncasecmp (tmp, "audio", 5))
		{
		  char *payload = NULL;
		  do {
		    payload = sdp_message_m_payload_get (sdp, pos, k);
		    if (payload == NULL)
		      {
		      }
		    else if (0==strcmp("110",payload))
		      {
			sdp_message_a_attribute_add (sdp,
						     pos,
						     osip_strdup ("AS"),
						     osip_strdup ("110 20"));
		      }
		    else if (0==strcmp("111",payload))
		      {
			sdp_message_a_attribute_add (sdp,
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

	  if (eXosip.j_firewall_ip[0]!='\0')
	  {
		  char *c_address = invite->req_uri->host;
		  int pos=0;
		  struct addrinfo *addrinfo;
		  struct __eXosip_sockaddr addr;
		  i = eXosip_get_addrinfo(&addrinfo, invite->req_uri->host, 5060);
		  if (i==0)
			{
			  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
			  freeaddrinfo (addrinfo);
			  c_address = inet_ntoa (((struct sockaddr_in *) &addr)->sin_addr);
			  OSIP_TRACE (osip_trace
				  (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "eXosip: here is the resolved destination host=%s\n", c_address));
			}

		  /* If remote message contains a Public IP, we have to replace the SDP
			connection address */
		  if (eXosip_is_public_address(c_address))
		  {
			  /* replace the IP with our firewall ip */
			  sdp_connection_t *conn = sdp_message_connection_get(sdp, -1, 0);
			  if (conn!=NULL && conn->c_addr!=NULL )
			  {
				  osip_free(conn->c_addr);
				  conn->c_addr = osip_strdup(eXosip.j_firewall_ip);
			  }
			  pos=0;
			  conn = sdp_message_connection_get(sdp, pos, 0);
			  while (conn!=NULL)
			  {
				  if (conn!=NULL && conn->c_addr!=NULL )
				  {
					  osip_free(conn->c_addr);
					  conn->c_addr = osip_strdup(eXosip.j_firewall_ip);
				  }
				  pos++;
				  conn = sdp_message_connection_get(sdp, pos, 0);
			  }
		  }

	  }

      i = sdp_message_to_str(sdp, &body);
      if (body!=NULL)
	{
	  size= (char *)osip_malloc(7*sizeof(char));
#ifdef __APPLE_CC__
	  sprintf(size,"%li",strlen(body));
#else
	  sprintf(size,"%i",strlen(body));
#endif
	  osip_message_set_content_length(invite, size);
	  osip_free(size);
	  
	  osip_message_set_body(invite, body, strlen(body));
	  osip_free(body);
	  osip_message_set_content_type(invite, "application/sdp");
	}
      else
	osip_message_set_content_length(invite, "0");
    }

  eXosip_call_init(&jc);
  if (local_sdp_port!=NULL)
    snprintf(jc->c_sdp_port,9, "%s", local_sdp_port);
  i = osip_message_get_subject(invite, 0, &subject);
  if (subject!=NULL && subject->hvalue!=NULL && subject->hvalue[0]!='\0')
	  snprintf(jc->c_subject, 99, "%s", subject->hvalue);
  
  if (sdp_context_reference==NULL)
    osip_negotiation_ctx_set_mycontext(jc->c_ctx, jc);
  else
    osip_negotiation_ctx_set_mycontext(jc->c_ctx, sdp_context_reference);

  if (local_sdp_port!=NULL)
    {
      osip_negotiation_ctx_set_local_sdp(jc->c_ctx, sdp);
      jc->c_ack_sdp = 0;
    }
  else
    jc->c_ack_sdp = 1;

  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      eXosip_call_free(jc);
      osip_message_free(invite);
      return -1;
    }
  
  jc->c_out_tr = transaction;
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);

  jc->external_reference = reference;
  ADD_ELEMENT(eXosip.j_calls, jc);

  eXosip_update(); /* fixed? */
  __eXosip_wakeup();
  return jc->c_id;
}


int eXosip2_answer(int jid, int status, osip_message_t **answer){
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (status>100 && status<200)
    {
      i = _eXosip2_answer_invite_1xx(jc, jd, status, answer);
    }
  else if (status>199 && status<300)
    {
      i = _eXosip2_answer_invite_2xx(jc, jd, status, answer);
    }
  else if (status>300 && status<699)
    {
      i = _eXosip2_answer_invite_3456xx(jc, jd, status, answer);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: wrong status code (101<status<699)\n"));
      return -1;
    }
  if (i!=0)
    return -1;
  return 0;
}

int eXosip2_answer_send(int jid, osip_message_t *answer){
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  osip_transaction_t *tr;
  osip_event_t *evt_answer;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }

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

  if (MSG_IS_STATUS_1XX(answer))
    {
      if (jd==NULL)
	{
	  i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, answer);
	  if (i!=0)
	    {
         OSIP_TRACE (osip_trace
		     (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot create dialog!\n"));
	      i = 0;
	    }
	  else
	    {
	      ADD_ELEMENT(jc->c_dialogs, jd);
	    }
	}
    }
  else if (MSG_IS_STATUS_2XX(answer))
    {
      if (jd==NULL)
	{
	  i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, answer);
	  if (i!=0)
	    {
         OSIP_TRACE (osip_trace
           (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot create dialog!\n"));
	      return -1;
	    }
	  ADD_ELEMENT(jc->c_dialogs, jd);
	}
      else i = 0;
      
      eXosip_dialog_set_200ok(jd, answer);
      osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
    }
  else if (answer->status_code>=300 && answer->status_code<=699)
    {
      i = 0;
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: wrong status code (101<status<699)\n"));
      return -1;
    }
  if (i!=0)
    return -1;

  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = tr->transactionid;
  
  osip_transaction_add_event(tr, evt_answer);
  __eXosip_wakeup();
  return 0;
}

int eXosip_answer_call_with_body(int jid, int status, const char *bodytype, const char *body){
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (status>100 && status<200)
    {
      i = eXosip_answer_invite_1xx(jc, jd, status);
    }
  else if (status>199 && status<300)
    {
      i = eXosip_answer_invite_2xx_with_body(jc, jd, status,bodytype,body);
    }
  else if (status>300 && status<699)
    {
      i = eXosip_answer_invite_3456xx(jc, jd, status);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: wrong status code (101<status<699)\n"));
      return -1;
    }
  if (i!=0)
    return -1;
  return 0;
}

int eXosip_set_redirection_address (int jid, char *contact)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (contact==NULL)
    memset(jc->c_redirection, '\0', 1024);
  else
    snprintf(jc->c_redirection, 1024, "%s", contact);
  return 0;
}

int eXosip_answer_call   (int jid, int status, char *local_sdp_port)
{
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (status>100 && status<200)
    {
      i = eXosip_answer_invite_1xx(jc, jd, status);
    }
  else if (status>199 && status<300)
    {
#if 0 /* this seems to be useless?? */
      if (jc->c_ctx!=NULL)
	osip_negotiation_ctx_set_mycontext(jc->c_ctx, jc);
      else
	osip_negotiation_ctx_set_mycontext(jc->c_ctx, sdp_context_reference);
#endif
      if (local_sdp_port!=NULL)
	{
	  osip_negotiation_ctx_set_mycontext(jc->c_ctx, jc);
	  snprintf(jc->c_sdp_port,9, "%s", local_sdp_port);
	}

      i = eXosip_answer_invite_2xx(jc, jd, status, local_sdp_port);
    }
  else if (status>300 && status<699)
    {
      i = eXosip_answer_invite_3456xx(jc, jd, status);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: wrong status code (101<status<699)\n"));
      return -1;
    }
  if (i!=0)
    return -1;
  return 0;
}

int
eXosip_retrieve_negotiated_payload(int jid, int *payload, char *payload_name, int pnsize)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  int pl;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }

  pl = eXosip_retrieve_sdp_negotiation_result(jc->c_ctx, payload_name, pnsize);

  if (pl >= 0)
    {
    *payload = pl;
    return 0;
    }

  return -1;
}

int eXosip_options_call  (int jid)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *options;
  int i;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }

  transaction = eXosip_find_last_options(jc, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
      transaction=NULL;
    }

  i = _eXosip_build_request_within_dialog(&options, "OPTIONS", jd->d_dialog, "UDP");
  if (i!=0)
    return -2;
  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       options);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(options);
      return -2;
    }
  
  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(options);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_answer_options   (int cid, int jid, int status)
{
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
      if (jd==NULL)
	{
     OSIP_TRACE (osip_trace
	    (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: No dialog here?\n"));
	  return -1;
	}
    }
  else
    {
      eXosip_call_find(cid, &jc);
      if (jc==NULL)
   {
     OSIP_TRACE (osip_trace
	    (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: No call here?\n"));
	  return -1;
	}
    }
  if (status>100 && status<200)
    {
      i = eXosip_answer_options_1xx(jc, jd, status);
    }
  else if (status>199 && status<300)
    {
      i = eXosip_answer_options_2xx(jc, jd, status);
    }
  else if (status>300 && status<699)
    {
      i = eXosip_answer_options_3456xx(jc, jd, status);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: wrong status code (101<status<699)\n"));
      return -1;
    }
  if (i!=0)
    return -1;
  return 0;
}

int eXosip_set_call_reference(int jid, void *reference)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jc==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
    jc->external_reference = reference;
    return 0;
}

int eXosip_on_hold_call  (int jid)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *invite;
  int i;
  sdp_message_t *sdp;
  char *body;
  char *size;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }

  transaction = eXosip_find_last_invite(jc, jd);
  if (transaction==NULL) return -1;
  if (transaction->state!=ICT_TERMINATED &&
      transaction->state!=IST_TERMINATED)
    return -1;

  sdp = eXosip_get_local_sdp_info(transaction);
  if (sdp==NULL)
    return -1;
  i = osip_negotiation_sdp_message_put_on_hold(sdp);
  if (i!=0)
    {
      sdp_message_free(sdp);
      return -2;
    }

  i = _eXosip_build_request_within_dialog(&invite, "INVITE", jd->d_dialog, "UDP");
  if (i!=0) {
    sdp_message_free(sdp);
    return -2;
  }

  i = sdp_message_to_str(sdp, &body);
  if (body!=NULL)
    {
      size= (char *)osip_malloc(7*sizeof(char));
#ifdef __APPLE_CC__
      sprintf(size,"%li",strlen(body));
#else
      sprintf(size,"%i",strlen(body));
#endif
      osip_message_set_content_length(invite, size);
      osip_free(size);
      
      osip_message_set_body(invite, body, strlen(body));
      osip_free(body);
      osip_message_set_content_type(invite, "application/sdp");
    }
  else
    osip_message_set_content_length(invite, "0");

  if (jc->c_subject==NULL || jc->c_subject[0]=='\0')
  {
#if 0
	  osip_message_set_subject(invite, "New Call");
#endif
  }
  else
	  osip_message_set_subject(invite, jc->c_subject);

  transaction=NULL;
  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(invite);
      return -2;
    }
  
  {
    sdp_message_t *old_sdp = osip_negotiation_ctx_get_local_sdp(jc->c_ctx);
    sdp_message_free(old_sdp);
    osip_negotiation_ctx_set_local_sdp(jc->c_ctx, sdp);
  }

  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_off_hold_call (int jid, char *rtp_ip, int port)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *invite;
  int i;
  sdp_message_t *sdp;
  char *body;
  char *size;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }

  transaction = eXosip_find_last_invite(jc, jd);
  if (transaction==NULL) return -1;
  if (transaction->state!=ICT_TERMINATED &&
      transaction->state!=IST_TERMINATED)
    return -1;

  sdp = eXosip_get_local_sdp_info(transaction);
  if (sdp==NULL)
    return -1;
  i = osip_negotiation_sdp_message_put_off_hold(sdp);
  if (i!=0)
    {
      sdp_message_free(sdp);
      return -2;
    }

  i = _eXosip_build_request_within_dialog(&invite, "INVITE", jd->d_dialog, "UDP");
  if (i!=0) {
    sdp_message_free(sdp);
    return -2;
  }

  if (rtp_ip!=NULL)
    {
      /* modify the connection address of host */
      sdp_connection_t *conn;
      sdp_media_t *med;
      int pos_media = 0;
      conn = sdp_message_connection_get(sdp, -1, 0);
      if (conn!=NULL && conn->c_addr!=NULL)
	{
	  osip_free(conn->c_addr);
	  conn->c_addr = osip_strdup(rtp_ip);
	}
      med = (sdp_media_t *) osip_list_get (sdp->m_medias, pos_media);
      while (med != NULL)
	{
	  if (med->m_media!=NULL && 0==osip_strcasecmp(med->m_media, "audio"))
	    {
	      osip_free(med->m_port);
	      med->m_port=(char *)osip_malloc(15);
	      snprintf(med->m_port, 14, "%i", port);
	      break;
	    }
	  pos_media++;
	  med = (sdp_media_t *) osip_list_get (sdp->m_medias, pos_media);
	}
    }

  i = sdp_message_to_str(sdp, &body);
  if (body!=NULL)
    {
      size= (char *)osip_malloc(7*sizeof(char));
#ifdef __APPLE_CC__
      sprintf(size,"%li",strlen(body));
#else
      sprintf(size,"%i",strlen(body));
#endif
      osip_message_set_content_length(invite, size);
      osip_free(size);

      osip_message_set_body(invite, body, strlen(body));
      osip_free(body);
      osip_message_set_content_type(invite, "application/sdp");
    }
  else
    osip_message_set_content_length(invite, "0");

  if (jc->c_subject==NULL || jc->c_subject[0]=='\0')
  {
#if 0
	  osip_message_set_subject(invite, "New Call");
#endif
  }
  else
	  osip_message_set_subject(invite, jc->c_subject);

  transaction=NULL;
  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(invite);
      return -2;
    }

  {
    sdp_message_t *old_sdp = osip_negotiation_ctx_get_local_sdp(jc->c_ctx);
    sdp_message_free(old_sdp);
    osip_negotiation_ctx_set_local_sdp(jc->c_ctx, sdp);
  }

  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

static int eXosip_create_transaction(eXosip_call_t *jc,
				     eXosip_dialog_t *jd,
				     osip_message_t *request)
{
  osip_event_t *sipevent;
  osip_transaction_t *tr;
  int i;
  i = osip_transaction_init(&tr,
		       NICT,
		       eXosip.j_osip,
		       request);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(request);
      return -1;
    }
  
  if (jd!=NULL)
    osip_list_add(jd->d_out_trs, tr, 0);
  
  sipevent = osip_new_outgoing_sipmessage(request);
  sipevent->transactionid =  tr->transactionid;
  
  osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(tr, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_transfer_call_out_of_dialog(char *refer_to, char *from, char *to, char *proxy)
{
  osip_message_t *refer;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
  i = generating_refer_outside_dialog(&refer, refer_to, from, to, proxy);
  if (i!=0) return -1;

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       refer);
  if (i!=0)
    {
      osip_message_free(refer);
      return -1;
    }
  
  osip_list_add(eXosip.j_transactions, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(refer);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_transfer_call(int jid, char *refer_to)
{
  int i;
  osip_message_t *request;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid<=0)
    return -1;

  eXosip_call_dialog_find(jid, &jc, &jd);
  if (jd==NULL || jd->d_dialog==NULL || jd->d_dialog->state==DIALOG_EARLY)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No established call here!"));
      return -1;
    }

  i = generating_refer(&request, jd->d_dialog, refer_to);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot generate REFER for call!"));
      return -2;
    }

  i = eXosip_create_transaction(jc, jd, request);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot initiate SIP transfer transaction!"));
      return i;
    }
  return 0;
}

static int eXosip_create_cancel_transaction(eXosip_call_t *jc,
					    eXosip_dialog_t *jd,
					    osip_message_t *request)
{
  osip_event_t *sipevent;
  osip_transaction_t *tr;
  int i;
  i = osip_transaction_init(&tr,
		       NICT,
		       eXosip.j_osip,
		       request);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(request);
      return -2;
    }
  
  osip_list_add(eXosip.j_transactions, tr, 0);
  
  sipevent = osip_new_outgoing_sipmessage(request);
  sipevent->transactionid =  tr->transactionid;
  
  osip_transaction_add_event(tr, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_terminate_call(int cid, int jid)
{
  int i;
  osip_transaction_t *tr;
  osip_message_t *request;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
      if (jd==NULL)
	{
     OSIP_TRACE (osip_trace
	    (__FILE__, __LINE__, OSIP_ERROR, NULL,
	     "eXosip: No call here? "));
	  return -1;
	}
    }
  else
    {
      eXosip_call_find(cid, &jc);
    }

  if (jc==NULL)
    {
      return -1;
    }

  tr=eXosip_find_last_out_invite(jc, jd);
  if (tr!=NULL && tr->last_response!=NULL && MSG_IS_STATUS_1XX(tr->last_response))
	{
	  i = generating_cancel(&request, tr->orig_request);
	  if (i!=0)
	    {
         OSIP_TRACE (osip_trace
	        (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot terminate this call! "));
	      return -2;
	    }
	  i = eXosip_create_cancel_transaction(jc, jd, request);
	  if (i!=0)
	    {
         OSIP_TRACE (osip_trace
    		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
	         "eXosip: cannot initiate SIP transaction! "));
	      return i;
	    }
	  if (jd!=NULL)
	  {
		  osip_dialog_free(jd->d_dialog);
		  jd->d_dialog = NULL;
	  }
	  return 0;
	}

  if (jd==NULL || jd->d_dialog==NULL)
    {
	  /* Check if some dialog exists */
	  jd = jc->c_dialogs;
	  if (jd!=NULL && jd->d_dialog!=NULL)
	  {
		  i = generating_bye(&request, jd->d_dialog);
		  if (i!=0)
			{
			  OSIP_TRACE (osip_trace
				  (__FILE__, __LINE__, OSIP_ERROR, NULL,
				 "eXosip: cannot terminate this call! "));
			  return -2;
			}

		  i = eXosip_create_transaction(jc, jd, request);
		  if (i!=0)
			{
			  OSIP_TRACE (osip_trace
				  (__FILE__, __LINE__, OSIP_ERROR, NULL,
				 "eXosip: cannot initiate SIP transaction! "));
			  return -2;
			}

		  osip_dialog_free(jd->d_dialog);
		  jd->d_dialog = NULL;
		  return 0;
	  }

      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No established dialog!"));
      return -1;
    }

  if (tr==NULL)
	{
	  /*this may not be enough if it's a re-INVITE! */
	  tr = eXosip_find_last_inc_invite(jc, jd);
	  if (tr!=NULL && tr->last_response!=NULL &&
	      MSG_IS_STATUS_1XX(tr->last_response))
	    { /* answer with 603 */
	      i = eXosip_answer_call(jid, 603, 0);
	      return i;
	    }
	}


  i = generating_bye(&request, jd->d_dialog);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot terminate this call! "));
      return -2;
    }

  i = eXosip_create_transaction(jc, jd, request);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot initiate SIP transaction! "));
      return -2;
    }

  osip_dialog_free(jd->d_dialog);
  jd->d_dialog = NULL;
  return 0;
}

static jauthinfo_t *
eXosip_find_authentication_info(const char *username, const char *realm)
{
  jauthinfo_t *fallback = NULL;
  jauthinfo_t *authinfo;

  for (authinfo = eXosip.authinfos;
       authinfo!=NULL;
       authinfo = authinfo->next)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "INFO: authinfo: %s %s\n", realm, authinfo->realm));
      if (0==strcmp(authinfo->username, username))
	{
	  if (authinfo->realm == NULL || authinfo->realm[0] == '\0')
	    {
	      fallback = authinfo;
	    }
	  else if (strcmp(realm,authinfo->realm)==0 || 0==strncmp(realm+1, authinfo->realm, strlen(realm)-2))
	    {
	      return authinfo;
	    }
	}
    }
  return fallback;
}


int eXosip_clear_authentication_info(){
  jauthinfo_t *jauthinfo;
  for (jauthinfo = eXosip.authinfos; jauthinfo!=NULL;
       jauthinfo = eXosip.authinfos)
    {
      REMOVE_ELEMENT(eXosip.authinfos, jauthinfo);
      osip_free(jauthinfo);
    }
  return 0;
}

int
eXosip_add_authentication_info(const char *username, const char *userid,
			       const char *passwd, const char *ha1,
			       const char *realm)
{
  jauthinfo_t *authinfos;

  if (username==NULL || username[0]=='\0') return -1;
  if (userid==NULL || userid[0]=='\0')     return -1;

  if ( passwd!=NULL && passwd[0]!='\0')    {}
  else if (ha1!=NULL && ha1[0]!='\0')      {}
  else return -1;

  authinfos = (jauthinfo_t *) osip_malloc(sizeof(jauthinfo_t));
  if (authinfos==NULL)
    return -1;
  memset(authinfos, 0, sizeof(jauthinfo_t));

  snprintf(authinfos->username, 50, "%s", username);
  snprintf(authinfos->userid,   50, "%s", userid);
  if ( passwd!=NULL && passwd[0]!='\0')
    snprintf(authinfos->passwd,   50, "%s", passwd);
  else if (ha1!=NULL && ha1[0]!='\0')
    snprintf(authinfos->ha1,      50, "%s", ha1);
  if(realm!=NULL && realm[0]!='\0')
    snprintf(authinfos->realm,    50, "%s", realm);

  ADD_ELEMENT(eXosip.authinfos, authinfos);
  return 0;
}

static int
eXosip_add_authentication_information(osip_message_t *req,
				      osip_message_t *last_response)
{
  osip_authorization_t *aut = NULL;
  osip_www_authenticate_t *wwwauth = NULL;
  osip_proxy_authorization_t *proxy_aut = NULL;
  osip_proxy_authenticate_t *proxyauth = NULL;
  jauthinfo_t *authinfo = NULL;
  int pos;
  int i;

  if (req==NULL
      ||req->from==NULL
      ||req->from->url==NULL
      ||req->from->url->username==NULL)
    return -1;

  pos=0;
  osip_message_get_www_authenticate(last_response, pos, &wwwauth);
  osip_message_get_proxy_authenticate(last_response, pos, &proxyauth);
  if (wwwauth==NULL && proxyauth==NULL) return -1;

  while (wwwauth!=NULL)
    {
      char *uri;
      authinfo = eXosip_find_authentication_info(req->from->url->username,
						 wwwauth->realm);
      if (authinfo==NULL) return -1;
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "authinfo: %s\n", authinfo->username));
      i = osip_uri_to_str (req->req_uri, &uri);
      if (i!=0) return -1;
      
      i = __eXosip_create_authorization_header(last_response, uri,
					       authinfo->userid,
					       authinfo->passwd,
					       &aut);
      osip_free(uri);
      if (i!=0) return -1;

      if (aut != NULL)
	{
	  osip_list_add (req->authorizations, aut, -1);
	  osip_message_force_update(req);
	}

      pos++;
      osip_message_get_www_authenticate(last_response, pos, &wwwauth);
    }

  pos=0;
  while (proxyauth!=NULL)
    {
      char *uri;
      authinfo = eXosip_find_authentication_info(req->from->url->username,
						 proxyauth->realm);
      if (authinfo==NULL) return -1;
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "authinfo: %s\n", authinfo->username));
      i = osip_uri_to_str (req->req_uri, &uri);
      if (i!=0) return -1;
      
      i = __eXosip_create_proxy_authorization_header(last_response, uri,
						     authinfo->userid,
						     authinfo->passwd,
						     &proxy_aut);
      osip_free(uri);
      if (i!=0) return -1;

      if (proxy_aut != NULL)
	{
	  osip_list_add (req->proxy_authorizations, proxy_aut, -1);
	  osip_message_force_update(req);
	}

      pos++;
      osip_message_get_proxy_authenticate (last_response, pos, &proxyauth);
    }

  return 0;
}


static int 
eXosip_update_top_via(osip_message_t *sip)
{
#ifdef SM
  char *locip;
#else
  char locip[50];
#endif
  char *tmp    = (char *)osip_malloc(256*sizeof(char));
  osip_via_t *via   = (osip_via_t *) osip_list_get (sip->vias, 0);


  osip_list_remove(sip->vias, 0);
	locip=osip_strdup(osip_via_get_host(via));
  osip_via_free(via);
#ifdef SM
#else
  eXosip_guess_ip_for_via(eXosip.ip_family, locip, 49);
#endif
  if (eXosip.ip_family==AF_INET6)
    snprintf(tmp, 256, "SIP/2.0/UDP [%s]:%s;branch=z9hG4bK%u",
	    locip,
	    eXosip.localport,
	    via_branch_new_random());
  else
    snprintf(tmp, 256, "SIP/2.0/UDP %s:%s;rport;branch=z9hG4bK%u",
	    locip,
	    eXosip.localport,
	    via_branch_new_random());

  if (eXosip.nat_type[0])
    {
      strncat(tmp, ";xxx-nat-type=", 256);
      strncat(tmp, eXosip.nat_type, 256);
    }

	      
#ifdef SM
  osip_free(locip);
#endif
  osip_via_init(&via);
  osip_via_parse(via, tmp);
  osip_list_add(sip->vias, via, 0);
  osip_free(tmp);

  return 0;
}

static eXosip_reg_t *
eXosip_reg_find(int rid)
{
    eXosip_reg_t *jr;

    for (jr = eXosip.j_reg; jr != NULL; jr = jr->next)
      {
	if (jr->r_id == rid)
	  {
	    return jr;
	  }
      }
    return NULL;
}

int eXosip_register      (int rid, int registration_period)
{
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *reg;
  eXosip_reg_t *jr;
  int i;

  jr = eXosip_reg_find(rid);
  if (jr==NULL)
    {
      /* fprintf(stderr, "eXosip: no registration info saved!\n"); */
      return -1;
    }
  if (registration_period>=0)
    jr->r_reg_period = registration_period;
  if (jr->r_reg_period==0)
    {} /* unregistration */
  else if (jr->r_reg_period>3600)
    jr->r_reg_period = 3600;
  else if (jr->r_reg_period<200) /* too low */
    jr->r_reg_period = 200;

  reg = NULL;
  if (jr->r_last_tr!=NULL)
    {
      if (jr->r_last_tr->state!=NICT_TERMINATED
	  && jr->r_last_tr->state!=NICT_COMPLETED)
	{
	  /* fprintf(stderr, "eXosip: a registration is already pending!\n"); */
	  return -1;
	}
      else
	{
	  osip_message_t *last_response;

	  reg = jr->r_last_tr->orig_request;
	  last_response = jr->r_last_tr->last_response;

	  jr->r_last_tr->orig_request = NULL;
	  jr->r_last_tr->last_response = NULL;
	  __eXosip_delete_jinfo(jr->r_last_tr);
	  osip_transaction_free(jr->r_last_tr);
	  jr->r_last_tr = NULL;

	  /* modify the REGISTER request */
	  {
	    int osip_cseq_num = osip_atoi(reg->cseq->number);
	    int length   = strlen(reg->cseq->number);


	    osip_authorization_t *aut;
	    osip_proxy_authorization_t *proxy_aut;
	    
	    aut = (osip_authorization_t *)osip_list_get(reg->authorizations, 0);
	    while (aut!=NULL)
	      {
		osip_list_remove(reg->authorizations, 0);
		osip_authorization_free(aut);
		aut = (osip_authorization_t *)osip_list_get(reg->authorizations, 0);
	      }

	    proxy_aut = (osip_proxy_authorization_t*)osip_list_get(reg->proxy_authorizations, 0);
	    while (proxy_aut!=NULL)
	      {
		osip_list_remove(reg->proxy_authorizations, 0);
		osip_proxy_authorization_free(proxy_aut);
		proxy_aut = (osip_proxy_authorization_t*)osip_list_get(reg->proxy_authorizations, 0);
	      }


	    if (-1 == eXosip_update_top_via(reg))
	      {
		osip_message_free(reg);
		return -1;
	      }

	    osip_cseq_num++;
	    osip_free(reg->cseq->number);
	    reg->cseq->number = (char*)osip_malloc(length+2); /* +2 like for 9 to 10 */
	    sprintf(reg->cseq->number, "%i", osip_cseq_num);

	    {
	      osip_header_t *exp;
	      osip_message_header_get_byname(reg, "expires", 0, &exp);
	      osip_free(exp->hvalue);
	      exp->hvalue = (char*)osip_malloc(10);
	      snprintf(exp->hvalue, 9, "%i", jr->r_reg_period);
	    }

	    osip_message_force_update(reg);
	  }

	  if (last_response!=NULL)
	    {
	      if (MSG_IS_STATUS_4XX(last_response))
		{
		  eXosip_add_authentication_information(reg, last_response);
		}
	      osip_message_free(last_response);
	    }
	}
    }
  if (reg==NULL)
    {
      i = generating_register(&reg, jr->r_aor, jr->r_registrar, jr->r_contact, jr->r_reg_period);
      if (i!=0)
	{
	  /* fprintf(stderr, "eXosip: cannot register (cannot build REGISTER)! "); */
	  return -2;
	}
    }
       //!jinti! if exist outbound proxy setting ,register must use outboundproxy!
       if (jr->r_route!=NULL)
       {
	 osip_route_t *r=NULL;
	 osip_message_get_route(reg,0,&r);
	 if (r==NULL)
         	osip_message_set_route(reg, jr->r_route);
       }
	
  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       reg);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(reg);
      return -2;
    }

  jr->r_last_tr = transaction;

  /* send REGISTER */
  sipevent = osip_new_outgoing_sipmessage(reg);
  sipevent->transactionid =  transaction->transactionid;
  osip_message_force_update(reg);
  
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int
eXosip_register_init(char *from, char *proxy, char *contact, char *route)
{
  eXosip_reg_t *jr;
  int i;

  /* Avoid adding the same registration info twice to prevent mem leaks */
  for (jr = eXosip.j_reg; jr != NULL; jr = jr->next)
    {
      if (strcmp(jr->r_aor, from) == 0
	  && strcmp(jr->r_registrar, proxy) == 0)
	{
	  return jr->r_id;
	}
    }

  /* Add new registration info */
  i = eXosip_reg_init(&jr, from, proxy, contact, route);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot register! "));
      return i;
    }
  ADD_ELEMENT(eXosip.j_reg, jr);
  return jr->r_id;
}


int eXosip_build_publish(osip_message_t **message,
			 char *to,
			 char *from,
			 char *route,
			 const char *event,
			 const char *expires,
			 const char *ctype,
			 const char *body)
{
  int i;
  if (to==NULL || to[0]=='\0')
    return -1;
  if (from==NULL || from[0]=='\0')
    return -1;
  if (event==NULL || event[0]=='\0')
    return -1;
  if (ctype==NULL || ctype[0]=='\0')
    {
      if (body!=NULL && body[0]!='\0')
	return -1;
    }
  else
    {
      if (body==NULL || body[0]=='\0')
	return -1;
    }

  i = generating_publish(message, to, from, route);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot send message (cannot build PUBLISH)! "));
      return -1;
    }

  if (body!=NULL && body[0]!='\0' && ctype!=NULL && ctype[0]!='\0')
    {
      osip_message_set_content_type(*message, ctype);
      osip_message_set_body(*message, body, strlen(body));
      osip_message_set_header(*message, "Content-Disposition", "render;handling=required");
    }
  if (expires!=NULL && expires[0]!='\0')
    osip_message_set_expires(*message, expires);
  else
    osip_message_set_expires(*message, "3600");

  osip_message_set_header(*message, "Event", event);
  return 0;
}

int eXosip_publish (osip_message_t *message, const char *to)
{
  /* eXosip_call_t *jc;
     osip_header_t *subject; */
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
  eXosip_pub_t *pub=NULL;

  if (message==NULL)
    return -1;
  if (message->cseq==NULL||message->cseq->number==NULL)
    return -1;
  if (to==NULL)
    {
      osip_message_free(message);
      return -1;
    }

  i = _eXosip_pub_find_by_aor(&pub, to);
  if (i!=0 || pub==NULL)
    {
      osip_header_t *expires;
      osip_message_get_expires(message, 0, &expires);
      if (expires==NULL || expires->hvalue==NULL)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "eXosip: missing expires header in PUBLISH!"));
	  osip_message_free(message);
	  return -1;
	}
      else
	{
	  /* start a new publication context */
	  _eXosip_pub_init(&pub, to, expires->hvalue);
	  if (pub==NULL) return -1;
	  ADD_ELEMENT(eXosip.j_pub, pub);
	}
    }
  else
    {
      if (pub->p_sip_etag!=NULL && pub->p_sip_etag[0]!='\0')
	{
	  /* increase cseq */
	  osip_message_set_header(message, "SIP-If-Match", pub->p_sip_etag);
	}

      if (pub->p_last_tr!=NULL && pub->p_last_tr->cseq!=NULL
	  &&pub->p_last_tr->cseq->number!=NULL)
	{
	  int osip_cseq_num = osip_atoi(pub->p_last_tr->cseq->number);
	  int length        = strlen(pub->p_last_tr->cseq->number);
	  
	  osip_cseq_num++;
	  osip_free(message->cseq->number);
	  message->cseq->number = (char*)osip_malloc(length+2); /* +2 like for 9 to 10 */
	  sprintf(message->cseq->number, "%i", osip_cseq_num);
	}
    }

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       message);
  if (i!=0)
    {
      osip_message_free(message);
      return -1;
    }

  if (pub->p_last_tr!=NULL)
    osip_list_add(eXosip.j_transactions, pub->p_last_tr, 0);
  pub->p_last_tr = transaction;
  
  sipevent = osip_new_outgoing_sipmessage(message);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_subscribe    (char *to, char *from, char *route)
{
  eXosip_subscribe_t *js;
  osip_message_t *subscribe;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
    
  i = generating_initial_subscribe(&subscribe, to, from, route);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot subscribe (cannot build SUBSCRIBE)! "));
      return -1;
    }

  i = eXosip_subscribe_init(&js, to);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: cannot subscribe."));
      return -1;
    }
  
  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       subscribe);
  if (i!=0)
    {
      osip_message_free(subscribe);
      return -1;
    }

  _eXosip_subscribe_set_refresh_interval(js, subscribe);
  js->s_out_tr = transaction;
  
  sipevent = osip_new_outgoing_sipmessage(subscribe);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, NULL, js, NULL));
  osip_transaction_add_event(transaction, sipevent);

  ADD_ELEMENT(eXosip.j_subscribes, js);
  eXosip_update(); /* fixed? */
  __eXosip_wakeup();
  return 0;
}


int eXosip_subscribe_refresh  (int sid, char *expires)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  if (sid>0)
    {
      eXosip_subscribe_dialog_find(sid, &js, &jd);
    }
  if (js==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No subscribe here?\n"));
      return -1;
    }

  if (jd==NULL)
    {
      osip_transaction_t *tr;
      osip_transaction_t *newtr;
      osip_message_t *sub;
      osip_event_t *sipevent;
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No subscribe dialog here?\n"));
      
      tr=eXosip_find_last_out_subscribe(js,NULL);
      if (tr==NULL){
	eXosip_trace(OSIP_INFO1,("eXosip_retry_last_sub: No such transaction."));
	return -1;
      }
      if (tr->last_response==NULL){
	eXosip_trace(OSIP_INFO1,("eXosip_retry_last_sub: transaction has not been answered."));
	return -1;
      }
      sub=eXosip_prepare_request_for_auth(tr->orig_request);
      if (sub==NULL) return -1;
      eXosip_add_authentication_information(sub,tr->last_response);

      if (expires==NULL)
	osip_message_set_expires(sub, "3600");
      else
	osip_message_set_expires(sub, expires);

      osip_message_force_update(sub);
      i = osip_transaction_init(&newtr,
				NICT,
				eXosip.j_osip,
				sub);
      if (i!=0)
	{
	  osip_message_free(sub);
	  return -1;
	}

      if (jd!=NULL)
	osip_list_add(jd->d_out_trs, newtr, 0);
      else
	{
	  js->s_out_tr = newtr;
	  /* remove old transaction */
	  osip_list_add(eXosip.j_transactions, tr, 0);
	}
      
      sipevent = osip_new_outgoing_sipmessage(sub);
      
      osip_transaction_set_your_instance(newtr, tr->your_instance);
      osip_transaction_set_your_instance(tr, NULL);
      osip_transaction_add_event(newtr, sipevent);
      
      eXosip_update(); /* fixed? */
      __eXosip_wakeup();
      return -1;
    }

#ifdef LOW_EXPIRE
  if (expires==NULL)
    i = eXosip_subscribe_send_subscribe(js, jd, "60");
  else
    i = eXosip_subscribe_send_subscribe(js, jd, expires);
#else
  if (expires==NULL)
    i = eXosip_subscribe_send_subscribe(js, jd, "3600");
  else
    i = eXosip_subscribe_send_subscribe(js, jd, expires);
#endif
  return i;
}

int eXosip_subscribe_close(int sid)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  if (sid>0)
    {
      eXosip_subscribe_dialog_find(sid, &js, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No subscribe dialog here?\n"));
      return -1;
    }

  i = eXosip_subscribe_send_subscribe(js, jd, "0");
  return i;
}

int eXosip_transfer_send_notify(int jid, int subscription_status, char *body)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (jd==NULL || jd->d_dialog==NULL)
    {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No established dialog!"));
      return -1;
    }

  return _eXosip_transfer_send_notify(jc, jd, subscription_status, body);
}

int _eXosip_transfer_send_notify(eXosip_call_t *jc,
				 eXosip_dialog_t *jd,
				 int subscription_status,
				 char *body)
{
  osip_transaction_t *transaction;
  osip_message_t *notify;
  osip_event_t *sipevent;
  int   i;
  char  subscription_state[50];
  char *tmp;

  transaction = eXosip_find_last_inc_refer(jc, jd);
  if (transaction==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "eXosip: No pending transfer!\n"));
      return -1;
    }

  transaction = eXosip_find_last_out_notify_for_refer(jc, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
      transaction=NULL;
    }

  i = _eXosip_build_request_within_dialog(&notify, "NOTIFY", jd->d_dialog, "UDP");
  if (i!=0)
    return -2;

  if (subscription_status==EXOSIP_SUBCRSTATE_PENDING)
    osip_strncpy(subscription_state, "pending;expires=", 16);
  else if (subscription_status==EXOSIP_SUBCRSTATE_ACTIVE)
    osip_strncpy(subscription_state, "active;expires=", 15);
  else if (subscription_status==EXOSIP_SUBCRSTATE_TERMINATED)
    {
      int reason = NORESOURCE;
      if (reason==DEACTIVATED)
	osip_strncpy(subscription_state, "terminated;reason=deactivated", 29);
      else if (reason==PROBATION)
	osip_strncpy(subscription_state, "terminated;reason=probation", 27);
      else if (reason==REJECTED)
	osip_strncpy(subscription_state, "terminated;reason=rejected", 26);
      else if (reason==TIMEOUT)
	osip_strncpy(subscription_state, "terminated;reason=timeout", 25);
      else if (reason==GIVEUP)
	osip_strncpy(subscription_state, "terminated;reason=giveup", 24);
      else if (reason==NORESOURCE)
	osip_strncpy(subscription_state, "terminated;reason=noresource", 29);
    }
  tmp = subscription_state + strlen(subscription_state);
  if (subscription_status!=EXOSIP_SUBCRSTATE_TERMINATED)
    sprintf(tmp, "%i", 180);
  osip_message_set_header(notify, "Subscription-State",
			 subscription_state);

  /* add a body */
  if (body!=NULL)
    {
      osip_message_set_body(notify, body, strlen(body));
      osip_message_set_content_type(notify, "message/sipfrag");
    }

  osip_message_set_header(notify, "Event", "refer");

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       notify);
  if (i!=0)
    {
      osip_message_free(notify);
      return -1;
    }
  
  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(notify);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_notify_send_notify(eXosip_notify_t *jn,
			      eXosip_dialog_t *jd,
			      int subscription_status,
			      int online_status)
{
  osip_transaction_t *transaction;
  osip_message_t *notify;
  osip_event_t *sipevent;
  int   i;
  char  subscription_state[50];
  char *tmp;
  int   now = time(NULL);
  transaction = eXosip_find_last_out_notify(jn, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
      transaction=NULL;
    }

#ifndef SUPPORT_MSN

#else

  /* DO NOT SEND ANY NOTIFY when the status
     is not active (or terminated?) */
  if (subscription_status!=EXOSIP_SUBCRSTATE_ACTIVE
      && subscription_status!=EXOSIP_SUBCRSTATE_TERMINATED)
    {
      /* set the new state anyway! */
      jn->n_online_status = online_status;
      jn->n_ss_status = subscription_status;
      return -1;
    }

#endif

  i = _eXosip_build_request_within_dialog(&notify, "NOTIFY", jd->d_dialog, "UDP");
  if (i!=0)
    return -2;

  jn->n_online_status = online_status;
  jn->n_ss_status = subscription_status;

  /* add the notifications info */
  if (jn->n_ss_status==EXOSIP_SUBCRSTATE_UNKNOWN)
    jn->n_online_status=EXOSIP_SUBCRSTATE_PENDING;

#ifndef SUPPORT_MSN
  if (jn->n_ss_status==EXOSIP_SUBCRSTATE_PENDING)
    osip_strncpy(subscription_state, "pending;expires=", 16);
  else if (jn->n_ss_status==EXOSIP_SUBCRSTATE_ACTIVE)
    osip_strncpy(subscription_state, "active;expires=", 15);
  else if (jn->n_ss_status==EXOSIP_SUBCRSTATE_TERMINATED)
    {
      if (jn->n_ss_reason==DEACTIVATED)
	osip_strncpy(subscription_state, "terminated;reason=deactivated", 29);
      else if (jn->n_ss_reason==PROBATION)
	osip_strncpy(subscription_state, "terminated;reason=probation", 27);
      else if (jn->n_ss_reason==REJECTED)
	osip_strncpy(subscription_state, "terminated;reason=rejected", 26);
      else if (jn->n_ss_reason==TIMEOUT)
	osip_strncpy(subscription_state, "terminated;reason=timeout", 25);
      else if (jn->n_ss_reason==GIVEUP)
	osip_strncpy(subscription_state, "terminated;reason=giveup", 24);
      else if (jn->n_ss_reason==NORESOURCE)
	osip_strncpy(subscription_state, "terminated;reason=noresource", 29);
    }
  tmp = subscription_state + strlen(subscription_state);
  if (jn->n_ss_status!=EXOSIP_SUBCRSTATE_TERMINATED)
    sprintf(tmp, "%i", jn->n_ss_expires-now);
  osip_message_set_header(notify, "Subscription-State",
			 subscription_state);
#endif

  /* add a body */
  i = _eXosip_notify_add_body(jn, notify);
  if (i!=0)
    {

    }

#ifdef SUPPORT_MSN
#else
  osip_message_set_header(notify, "Event", "presence");
#endif

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       notify);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(notify);
      return -1;
    }
  
  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(notify);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, NULL, jn));
  osip_transaction_add_event(transaction, sipevent);
  __eXosip_wakeup();
  return 0;
}

int eXosip_notify  (int nid, int subscription_status, int online_status)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_notify_t *jn = NULL;

  if (nid>0)
    {
      eXosip_notify_dialog_find(nid, &jn, &jd);
    }
  if (jd==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No subscribe dialog here?\n"));
      return -1;
    }

  i = eXosip_notify_send_notify(jn, jd, subscription_status, online_status);
  return i;
}


int eXosip_notify_accept_subscribe(int nid, int code,
				   int subscription_status,
				   int online_status)
{
  int i = 0;
  eXosip_dialog_t *jd = NULL;
  eXosip_notify_t *jn = NULL;
  if (nid>0)
    {
      eXosip_notify_dialog_find(nid, &jn, &jd);
    }
  if (jd==NULL)
    {
       OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: No call here?\n"));
      return -1;
    }
  if (code>100 && code<200)
    {
      eXosip_notify_answer_subscribe_1xx(jn, jd, code);
    }
  else if (code>199 && code<300)
    {
      eXosip_notify_answer_subscribe_2xx(jn, jd, code);
      i = eXosip_notify(nid, subscription_status, online_status);
    }
  else if (code>300 && code<699)
    {
      eXosip_notify_answer_subscribe_3456xx(jn, jd, code);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
         "eXosip: wrong status code (101<code<699)\n"));
      return -1;
    }

  return i;
}

void eXosip_guess_contact_uri(const char *url, char *strbuf, int bufsize, int public_net)
{
  int i;
  osip_from_t *a_from;
  char *contact = strbuf;
  char locip[50];

  eXosip_guess_ip_for_via(eXosip.ip_family, locip, sizeof(locip) - 1);
  contact[0] = 0;

  i = osip_from_init(&a_from);
  if (i==0)
    i = osip_from_parse(a_from, url);
  
  if (i==0 && a_from!=NULL
      && a_from->url!=NULL && a_from->url->username!=NULL )
    {
      if (eXosip.j_firewall_ip[0]!='\0')
	{
	  if (public_net)
	    {
	      if (eXosip.localport==NULL)
		snprintf(contact, bufsize, "<sip:%s@%s>", a_from->url->username,
			 eXosip.j_firewall_ip);
	      else
		snprintf(contact, bufsize, "<sip:%s@%s:%s>", a_from->url->username,
			 eXosip.j_firewall_ip,
			 eXosip.localport);
		}
	  else
	    {
	      if (eXosip.localport==NULL)
		snprintf(contact, bufsize, "<sip:%s@%s>", a_from->url->username,
			locip);
	      else
		snprintf(contact, bufsize, "<sip:%s@%s:%s>", a_from->url->username,
			locip,
			eXosip.localport);
	    }
	}
      else
	{
	  if (eXosip.localport==NULL)
	    snprintf(contact, bufsize, "<sip:%s@%s>", a_from->url->username,
		    locip);
	  else
	    snprintf(contact, bufsize, "<sip:%s@%s:%s>", a_from->url->username,
		    locip,
		    eXosip.localport);
	}
	  
      osip_from_free(a_from);
    }
} 

void eXosip_set_answer_contact(const char *contacturl)
{
  osip_strncpy(eXosip.answer_contact, contacturl ? contacturl : "", sizeof(eXosip.answer_contact)-1);
}
