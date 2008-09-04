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

#ifndef __EXOSIP_H__
#define __EXOSIP_H__

#include <osipparser2/osip_parser.h>
#include <osip2/osip_negotiation.h>
#include <time.h>

/**
 * @file eXosip.h
 * @brief eXosip API
 *
 * eXosip is a high layer library for rfc3261: the SIP protocol.
 * It has limited API and functionnalities to make it easy to use.
 * If you need complexity in your SIP applications, you may prefer
 * to use directly osip.
 *
 * eXosip capabilities:
 *    REGISTER                 to handle mobility.
 *    INVITE/BYE/OPTIONS       to start/stop VoIP sessions.
 *    re-INVITE                to modify VoIP sessions (On-hold/Off-hold)
 *    MESSAGE                  to send Instant Message.
 *    SUBSCRIBE/NOTIFY         to handle presence capabilities.
 *
 * <P>
 */

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @defgroup eXosip eXosip Management
 * @ingroup eXosip
 * @{
 */

/**
 * Use IPv6 instead of IPv4.
 * 
 * @param ipv6_enable  This paramter should be set to 1 to enable IPv6.
 */
void eXosip_enable_ipv6(int ipv6_enable);

/**
 * Initiate the eXtented oSIP library.
 * 
 * @param input    command input (RESERVED FOR FUTUR USE).
 * @param output   resulting output (RESERVED FOR FUTUR USE).
 * @param port     The SIP port to use. (5060 is the default)
 */
int   eXosip_init(FILE *input, FILE *output, int port);

	
/**
 * Force eXosip to use a specific local ip address in all its SIP message.
 *
 * @param localip 	the ip address.
 *
 * If set to NULL, then the local ip address will be guessed 
 * automatically (returns to default mode).
 */
int eXosip_force_localip(const char *localip); 

/**
 * Release ressource used by the eXtented oSIP library.
 * 
 */
void  eXosip_quit(void);

/**
 * Automatic refresh of subscription and register
 *   send subscribe refresh when subscription is close to expiration.
 *   send register refresh when registration is close to expiration.
 * 
 */
void eXosip_automatic_refresh(void);

/**
 * Wake Up the eXosip_event_wait method.
 * 
 */
void  __eXosip_wakeup_event(void);

/**
 * This method is used to modify the sdp connection address
 * When you are behind a NAT. (EXPERIMENTAL)
 * 
 */
void eXosip_set_firewallip(const char *firewall_address);

/**
 * This method is used to modify NAT firewall type
 * When you are behind a NAT. (EXPERIMENTAL)
 * @param nat type ("", "fcone", "rcone", "prcone", "sym")
 */
void eXosip_set_nattype(const char *nat_type);

/**
 * This method is used to force ALL SIP packets to go through given proxy
 * @param  proxyurl - when empty disable forced proxy behaviour
 */
void eXosip_force_proxy(const char *proxyurl);

/**
 * This method is used to set contact header on answers which establishes dialogs
 * @param  contacturl - the url to use for conatct SIP header,
 *                      when empty - the contact header will be generated from 'to'
 *                     header of the incoming request
 */
void eXosip_set_answer_contact(const char *contacturl);

/**
 * This method is used to build a contact header for SIP packets
 * @param  url         url from wich username part will be extracted
 * @param  strbuf      buffer where the result will be stored
 * @param  bufsize     size of the 'strbuf'
 * @param  public_net  when TRUE use firewall ip (if defined) as hostname part
 *                     else use local host ip
 *                    
 */
void eXosip_guess_contact_uri(const char *url, char *strbuf, int bufsize, int public_net);

/**
 * Find the current localip (interface with default route).
 * ****DEPRECATED**** Use eXosip_guess_localip
 * 
 * @param ip    a string containing the local IP address.
 */
void eXosip_get_localip(char *ip);

/**
 * Find the current localip (interface with default route).
 * 
 * @param family    AF_INET or AF_INET6
 * @param address   a string containing the local IP address.
 * @param size      The size of the string
 */
int eXosip_guess_localip(int family, char *address, int size);

/**
 * Find the interface to be used to reach the specified host.
 * 
 * @param ip    a string containing the local IP address.
 * @param localip	the local ip address to be used to reach host.
 *
 * You usually don't need this function at all.
 */
void eXosip_get_localip_for(char *host, char **localip);

/**
 * Add authentication credentials. These are used when an outgoing
 * request comes back with an authorization required response.
 *
 * @param username	username
 * @param userid	not sure what this is for
 * @param passwd	password
 * @param ha1		currently ignored
 * @param realm		realm within which credentials apply, or NULL
 *			to apply credentials to unrecognized realms
 */
int eXosip_add_authentication_info(const char *username, const char *userid,
				   const char *passwd, const char *ha1,
				   const char *realm);
				   
/**
 * Clear all authentication credentials stored in eXosip.
 *
 */
int eXosip_clear_authentication_info(void);

/**
 * Remove an audio payload.
 * 
 */
void eXosip_sdp_negotiation_remove_audio_payloads(void);

/**
 * Remove an audio payload.
 * 
 * @param payload         The payload
 * @param number_of_port  The number of port
 * @param proto           The protocol
 * @param c_nettype       The connection type of network
 * @param c_addrtype      The connection type of address
 * @param c_addr          The connection address
 * @param c_addr_multicast_ttl  The multicast TTL value
 * @param c_addr_multicast_int  ?
 * @param a_rtpmap        The rtpmap of element.
 */
void eXosip_sdp_negotiation_add_codec(char *payload, char *number_of_port,
				      char *proto, char *c_nettype,
				      char *c_addrtype, char *c_addr,
				      char *c_addr_multicast_ttl,
				      char *c_addr_multicast_int,
				      char *a_rtpmap);

/**
 * Replace the internal SDP negociator facility.
 * 
 * @param sn The new negociator context
 */
int eXosip_sdp_negotiation_replace(osip_negotiation_t *sn);

struct eXosip_call_t;

/**
 * Set the reference element to be used in callback for the negociator.
 * This must be done only if eXosip_sdp_negotiation_replace has been
 * used and must not be done if this method has not been used.
 * 
 * @param jc The related call.
 * @param sn The new negociator context.
 */
void eXosip_sdp_negotiation_ctx_set_mycontext(struct eXosip_call_t *jc, void *arg);

#define DEFAULT_MODE  2
#define CALLBACK_MODE 1
#define EVENT_MODE    2
/**
 * Set the functionnal mode of eXosip.
 * The default mode is the EVENT_MODE.
 * 
 * @param mode    mode to use.
 */
void eXosip_set_mode(int mode);

/**
 * Set the SIP User-Agent: header string.
 */
void eXosip_set_user_agent(const char *user_agent);

/**
 * Lock the eXtented oSIP library.
 * 
 */
int   eXosip_lock(void);

/**
 * UnLock the eXtented oSIP library.
 * 
 */
int   eXosip_unlock(void);

/** @} */

/**
 * @defgroup eXosip_call eXosip Call Management
 * @ingroup eXosip
 * @{
 */

/**
 * Build a default INVITE message for a new call.
 * 
 * @param invite    Pointer for the SIP element to hold.
 * @param to        SIP url for callee.
 * @param from      SIP url for caller.
 * @param route     Route header for INVITE. (optionnal)
 * @param subject   Subject for the call.
 */
int   eXosip_build_initial_invite(osip_message_t **invite, char *to,
				  char *from, char *route, char *subject);
/**
 * Initiate a call.
 * 
 * @param invite          SIP INVITE message to send.
 * @param reference       Application context. (optionnal)
 * @param sdp_reference_context  Sdp Application context. (optionnal)
 * @param local_sdp_port  The port to be used for this call.
 */
int   eXosip_initiate_call    (osip_message_t *invite, void *reference,
			       void *sdp_context_reference, char *local_sdp_port);

/**
 * Initiate a call with the specified body-type and body
 * 
 * @param invite          SIP INVITE message to send.
 * @param body_type		  body type (ex: "application/sdp")
 * @param body 			  The body.
 */
int   eXosip_initiate_call_with_body    (osip_message_t *invite, const char *body_type, const char *body, void *reference);


/**
 * When a call failed to initiate due to authorization reasons (407), retry the call using the authentication information
 * previously stored in eXosip using eXosip_add_authentication_info()
 * 
 * @param cid The eXosip call id as returned by eXosip_initiate_call*()
 */
int   eXosip_retry_call  (int cid);

/**
 * Initiate a blind transfer outside of a call.
 * (probably not supported by many softphone...)
 * 
 * @param refer_to  SIP url for transfer.
 * @param from      SIP url for caller.
 * @param to        SIP url for callee.
 * @param route     Route header for REFER. (optionnal)
 */
  int eXosip_transfer_call_out_of_dialog(char *refer_to, char *from, char *to, char *route);

/**
 * Set redirection url before a call to eXosip_answer_call.
 * 
 * @param jid          dialog id of call.
 * @param contact      contact address.
 */
  int eXosip_set_redirection_address (int jid, char *contact);

/**
 * Answer call.
 * 
 * @param jid          dialog id of call.
 * @param status       Status code to use.
 */
int   eXosip_answer_call   (int jid, int status, char *local_sdp_port);

/**
 * Answer call.
 * 
 * @param jid          dialog id of call.
 * @param status       Status code to use.
 * @param answer       The resulting sip answer to send.
 */
int   eXosip2_answer   (int jid, int status, osip_message_t **answer);

/**
 * Answer call.
 * 
 * @param jid          dialog id of call.
 * @param answer       The sip answer to send.
 */
int eXosip2_answer_send(int jid, osip_message_t *answer);

/**
 * Answer call with the specified body.
 * 
 * @param jid          dialog id of call.
 * @param status       Status code to use.
 * @param body_type	   body type (ex: "application/sdp")
 * @param body 		   The body.
 */
int   eXosip_answer_call_with_body   (int jid, int status,const char *body_type, const char*body);


/**
 * Retreive payload negotiated for this dialog
 * 
 * @param jid          dialog id of call.
 * @param payload      will hold the negotited payload code
 * @param payload_name will hold negotioated payload mime string
 * @param pnsize       size of the payload_name buffer
 */
int   eXosip_retrieve_negotiated_payload(int jid, int *payload, char *payload_name, int pnsize);

/**
 * Set a new application context for an existing call
 *
 * @param jc        Pointer to the call to modify
 * @param reference New application context.
 */
int  eXosip_set_call_reference(int jid, void *reference);

/**
 * Put a call on hold.
 * 
 * @param jid          dialog id of call.
 */
int   eXosip_on_hold_call  (int jid);
int   eXosip_off_hold_call (int jid, char *rtp_ip, int port);

/**
 * Tranfer a call.
 * 
 * @param jid          dialog id of call.
 * @param refer_to     SIP Url for transfer.
 */
int   eXosip_transfer_call(int jid, char *refer_to);

/**
 * Tranfer a call.
 * 
 * @param jid          dialog id of call.
 * @param refer_to     SIP Url for transfer.
 */
int   eXosip_transfer_send_notify(int jid, int subscription_status, char *body);

/**
 * Terminate a call.
 * 
 * @param cid          call id of call.
 * @param jid          dialog id of call.
 */
int   eXosip_terminate_call(int cid, int jid);

/**
 * Send an OPTIONS message within a call.
 * 
 * @param cid          call id of call.
 * @param jid          dialog id of call.
 */
int   eXosip_options_call  (int jid);

/**
 * Send an INFO message within a call.
 * 
 * @param jid           dialog id of call.
 * @param content_type  content-type of body.
 * @param body          body to attach.
 */
int eXosip_info_call(int jid, char *content_type, char *body);

/**
 * Answer an OPTIONS message.
 * 
 * @param cid          call id of call.
 * @param jid          dialog id of call.
 */
int   eXosip_answer_options(int cid, int jid, int status);

/** @} */

/**
 * @defgroup eXosip_registration eXosip Registration Management
 * @ingroup eXosip
 * @{
 */

/**
 * Send initial registration to a server. (REGISTER request)
 * 
 * @param from      SIP url for caller.
 * @param proxy     Proxy used for registration.
 * @param contact   Contact address. (optionnal)
 * @param route     Outbound proxy address. (optionnal)
 */
int   eXosip_register_init (char *from, char *proxy, char *contact, char* route);

/**
 * Update the registration. (REGISTER request)
 * 
 * @param rid .
 * @param expires The expires value for registration.
 */
int   eXosip_register      (int rid, int expires);

/** @} */

/**
 * @defgroup eXosip_impp eXosip Instant Messaging and Presence Management
 * @ingroup eXosip
 * @{
 */

/**
 * Send an Instant Message (MESSAGE request).
 * 
 * @param rid .
 */
int  eXosip_message    (char *to, char *from, char *route, char *buff);

/**
 * build publication for a user. (PUBLISH request)
 * 
 * @param message   returned published request.
 * @param to        SIP url for callee.
 * @param from      SIP url for caller.
 * @param route     Route used for publication.
 * @param event     SIP Event header.
 * @param expires   SIP Expires header.
 * @param ctype     Content-Type of body.
 * @param body     body for publication.
 */
int eXosip_build_publish(osip_message_t **message,
			 char *to,
			 char *from,
			 char *route,
			 const char *event,
			 const char *expires,
			 const char *ctype,
			 const char *body);

/**
 * Send an Publication Message (PUBLISH request).
 * 
 * @param message is a ready to be sent publish message .
 * @param sip_if_match is the SIP-If-Match header. (NULL for initial publication)
 */
int eXosip_publish (osip_message_t *message, const char *sip_if_match);

/**
 * Send initial subscription to a user. (SUBSCRIBE request)
 * 
 * @param to        SIP url for callee.
 * @param from      SIP url for caller.
 * @param route     Route used for registration.
 */
int  eXosip_subscribe(char *to, char *from, char *route);

/**
 * Update a subscription (SUBSCRIBE request).
 * 
 * @param sid       id of subscription.
 * @param expires   Time between updates.
 */
int  eXosip_subscribe_refresh(int sid, char *expires);

/**
 * Close a subscription (SUBSCRIBE request).
 * 
 * @param sid       id of subscription.
 */
int  eXosip_subscribe_close(int sid);

typedef enum eXosip_ss {
  EXOSIP_SUBCRSTATE_UNKNOWN,
  EXOSIP_SUBCRSTATE_PENDING,
  EXOSIP_SUBCRSTATE_ACTIVE,
  EXOSIP_SUBCRSTATE_TERMINATED
} eXosip_ss_t;

typedef enum eXosip_ss_reason {
  DEACTIVATED,
  PROBATION,
  REJECTED,
  TIMEOUT,
  GIVEUP,
  NORESOURCE
} eXosip_ss_reason_t;

  /* typo error */
#define NORESSOURCE NORESOURCE

typedef enum eXosip_ss_status {
  EXOSIP_NOTIFY_UNKNOWN,
  EXOSIP_NOTIFY_PENDING, /* subscription not yet accepted */
  EXOSIP_NOTIFY_ONLINE,
  EXOSIP_NOTIFY_BUSY,
  EXOSIP_NOTIFY_BERIGHTBACK,
  EXOSIP_NOTIFY_AWAY,
  EXOSIP_NOTIFY_ONTHEPHONE,
  EXOSIP_NOTIFY_OUTTOLUNCH,
  EXOSIP_NOTIFY_CLOSED
} eXosip_ss_status_t;

/**
 * Accept a subscription and notify state (NOTIFY request).
 * 
 * @param nid            call id of call.
 * @param code           code for answer to subscribe.
 * @param subscription_status  subscription status.
 * @param online_status  online status to send.
 */
int  eXosip_notify_accept_subscribe   (int nid, int code, int subscription_status, int online_status);

/**
 * Update a subscription and notify state (NOTIFY request).
 * 
 * @param nid            call id of call.
 * @param subscription_status  subscription status.
 * @param online_status  online status to send.
 */
int  eXosip_notify(int nid, int subscription_status, int online_status);

/** @} */


int eXosip_send_ack(int did);

/**
 * @defgroup eXosip_event eXosip event Management
 * @ingroup eXosip
 * @{
 */
typedef enum eXosip_event_type_t {

  /* Registration Info */
  EXOSIP_REGISTRATION_NEW,              /* announce new registration.       */
  EXOSIP_REGISTRATION_SUCCESS,          /* user is successfully registred.  */
  EXOSIP_REGISTRATION_FAILURE,          /* user is not registred.           */
  EXOSIP_REGISTRATION_REFRESHED,        /* registration has been refreshed. */
  EXOSIP_REGISTRATION_TERMINATED,       /* UA is not registred any more.    */

  /* for UAC events */
  EXOSIP_CALL_NOANSWER,        /* announce no answer within the timeout */
  EXOSIP_CALL_PROCEEDING,      /* announce processing by a remote app   */
  EXOSIP_CALL_RINGING,         /* announce ringback                     */
  EXOSIP_CALL_ANSWERED,        /* announce start of call                */
  EXOSIP_CALL_REDIRECTED,      /* announce a redirection                */
  EXOSIP_CALL_REQUESTFAILURE,  /* announce a request failure            */
  EXOSIP_CALL_SERVERFAILURE,   /* announce a server failure             */
  EXOSIP_CALL_GLOBALFAILURE,   /* announce a global failure             */

  /* for UAS events */
  EXOSIP_CALL_NEW,             /* announce a new call                   */
  EXOSIP_CALL_ACK,             /* ACK received for 200ok to INVITE      */
  EXOSIP_CALL_CANCELLED,       /* announce that call has been cancelled */
  EXOSIP_CALL_TIMEOUT,         /* announce that call has failed         */
  EXOSIP_CALL_HOLD,            /* audio must be stopped                 */
  EXOSIP_CALL_OFFHOLD,         /* audio must be restarted               */
  EXOSIP_CALL_CLOSED,          /* a BYE was received for this call      */

  /* for both UAS & UAC events */
  EXOSIP_CALL_STARTAUDIO,         /* audio must be established           */
  EXOSIP_CALL_RELEASED,           /* call context is cleared.            */

  /* for UAC events */
  EXOSIP_OPTIONS_NOANSWER,        /* announce no answer within the timeout */
  EXOSIP_OPTIONS_PROCEEDING,      /* announce processing by a remote app   */
  EXOSIP_OPTIONS_ANSWERED,        /* announce a 200ok                      */
  EXOSIP_OPTIONS_REDIRECTED,      /* announce a redirection                */
  EXOSIP_OPTIONS_REQUESTFAILURE,  /* announce a request failure            */
  EXOSIP_OPTIONS_SERVERFAILURE,   /* announce a server failure             */
  EXOSIP_OPTIONS_GLOBALFAILURE,   /* announce a global failure             */

  EXOSIP_INFO_NOANSWER,        /* announce no answer within the timeout */
  EXOSIP_INFO_PROCEEDING,      /* announce processing by a remote app   */
  EXOSIP_INFO_ANSWERED,        /* announce a 200ok                      */
  EXOSIP_INFO_REDIRECTED,      /* announce a redirection                */
  EXOSIP_INFO_REQUESTFAILURE,  /* announce a request failure            */
  EXOSIP_INFO_SERVERFAILURE,   /* announce a server failure             */
  EXOSIP_INFO_GLOBALFAILURE,   /* announce a global failure             */

  /* for UAS events */
  EXOSIP_OPTIONS_NEW,             /* announce a new options method         */
  EXOSIP_INFO_NEW,               /* new info request received           */

  EXOSIP_MESSAGE_NEW,            /* announce new incoming MESSAGE. */
  EXOSIP_MESSAGE_SUCCESS,        /* announce a 200ok to a previous sent */
  EXOSIP_MESSAGE_FAILURE,        /* announce a failure. */

  /* Presence and Instant Messaging */
  EXOSIP_SUBSCRIPTION_NEW,          /* announce new incoming SUBSCRIBE.  */
  EXOSIP_SUBSCRIPTION_UPDATE,       /* announce incoming SUBSCRIBE.      */
  EXOSIP_SUBSCRIPTION_CLOSED,       /* announce end of subscription.     */

  EXOSIP_SUBSCRIPTION_NOANSWER,        /* announce no answer              */
  EXOSIP_SUBSCRIPTION_PROCEEDING,      /* announce a 1xx                  */
  EXOSIP_SUBSCRIPTION_ANSWERED,        /* announce a 200ok                */
  EXOSIP_SUBSCRIPTION_REDIRECTED,      /* announce a redirection          */
  EXOSIP_SUBSCRIPTION_REQUESTFAILURE,  /* announce a request failure      */
  EXOSIP_SUBSCRIPTION_SERVERFAILURE,   /* announce a server failure       */
  EXOSIP_SUBSCRIPTION_GLOBALFAILURE,   /* announce a global failure       */
  EXOSIP_SUBSCRIPTION_NOTIFY,          /* announce new NOTIFY request     */

  EXOSIP_SUBSCRIPTION_RELEASED,        /* call context is cleared.        */

  EXOSIP_IN_SUBSCRIPTION_NEW,          /* announce new incoming SUBSCRIBE.*/
  EXOSIP_IN_SUBSCRIPTION_RELEASED,     /* announce end of subscription.   */

  EXOSIP_CALL_REFERED,              /* announce incoming REFER           */
  EXOSIP_CALL_REFER_STATUS,         /* announce incoming NOTIFY          */

  EXOSIP_CALLBACK_COUNT
} eXosip_event_type_t;

typedef struct eXosip_event {
  eXosip_event_type_t type;
  void               *external_reference;
  char                reason_phrase[50];
  int                 status_code;

  char                textinfo[256];
  char                req_uri[256];
  char                local_uri[256];
  char                remote_uri[256];
  char                remote_contact[256];
  char                refer_to[256];
  char                subject[256];
  osip_content_type_t *i_ctt;
  osip_list_t         *i_bodies;

  char                remote_sdp_audio_ip[50];
  int                 remote_sdp_audio_port;
  int                 local_sdp_audio_port;
  int                 payload;
  char                payload_name[50];
  char                sdp_body[1000]; /* The whole SDP body */
  int                 online_status;
  int                 ss_status;
  int                 ss_reason;

  /* For a high level usage of the eXosip stack? (API is enough?) */
  int did;
  int cid;
  int rid;
  int sid;
  int nid;

  /* For a low level usage of the eXosip stack */
  struct eXosip_dialog_t     *jd;
  struct eXosip_call_t       *jc;
  struct eXosip_reg_t        *jr;
  struct eXosip_subscribe_t  *js;
  struct eXosip_notify_t     *jn;

} eXosip_event_t;

/**
 * Free ressource in an eXosip event.
 * 
 * @param je    event to work on.
 */
void eXosip_event_free(eXosip_event_t *je);

/**
 * Wait for an eXosip event. (FUTURE USE)
 * 
 * @param tv_s      timeout value (seconds).
 * @param tv_ms     timeout value (mseconds).
 */
eXosip_event_t *eXosip_event_wait(int tv_s, int tv_ms);


/**
 * Wait for next eXosip event.
 * 
 */
eXosip_event_t *eXosip_event_get(void);

#ifdef __cplusplus
}
#endif

#endif
