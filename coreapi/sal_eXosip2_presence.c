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


#include "sal_eXosip2.h"


SalOp * sal_find_out_subscribe(Sal *sal, int sid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->out_subscribes;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->sid==sid) return op;
	}
	return NULL;
}

static void sal_add_out_subscribe(Sal *sal, SalOp *op){
	sal->out_subscribes=ms_list_append(sal->out_subscribes,op);
}

void sal_remove_out_subscribe(Sal *sal, SalOp *op){
	sal->out_subscribes=ms_list_remove(sal->out_subscribes,op);
}

static SalOp * sal_find_in_subscribe(Sal *sal, int nid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->in_subscribes;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->nid==nid) return op;
	}
	return NULL;
}

static SalOp * sal_find_in_subscribe_by_call_id(Sal *sal, osip_call_id_t *call_id){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->in_subscribes;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->call_id && osip_call_id_match(op->call_id,call_id)==0)
			return op;
	}
	return NULL;
}

static void sal_add_in_subscribe(Sal *sal, SalOp *op, osip_message_t *subs){
	osip_call_id_clone(subs->call_id,&op->call_id);
	sal->in_subscribes=ms_list_append(sal->in_subscribes,op);
}

void sal_remove_in_subscribe(Sal *sal, SalOp *op){
	sal->in_subscribes=ms_list_remove(sal->in_subscribes,op);
}

int sal_text_send(SalOp *op, const char *from, const char *to, const char *msg){
	osip_message_t *sip=NULL;

	if(op->cid == -1)
	{
		/* we are not currently in communication with the destination */
		if (from)
			sal_op_set_from(op,from);
		if (to)
			sal_op_set_to(op,to);

		eXosip_lock();
		eXosip_message_build_request(&sip,"MESSAGE",sal_op_get_to(op),
			sal_op_get_from(op),sal_op_get_route(op));
		if (sip!=NULL){
			osip_message_set_content_type(sip,"text/plain");
			osip_message_set_body(sip,msg,strlen(msg));
			sal_add_other(op->base.root,op,sip);
			eXosip_message_send_request(sip);
		}else{
			ms_error("Could not build MESSAGE request !");
		}
		eXosip_unlock();
	}
	else
	{
		/* we are currently in communication with the destination */
		eXosip_lock();
		//First we generate an INFO message to get the current call_id and a good cseq
		eXosip_call_build_request(op->did,"MESSAGE",&sip);
		if(sip == NULL)
		{
			ms_warning("could not get a build info to send MESSAGE, maybe no previous call established ?");
			eXosip_unlock();
			return -1;
		}
		osip_message_set_content_type(sip,"text/plain");
		osip_message_set_body(sip,msg,strlen(msg));
		eXosip_call_send_request(op->did,sip);
		eXosip_unlock();
	}
	return 0;
}

/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to){
	osip_message_t *msg;
	if (from)
		sal_op_set_from(op,from);
	if (to)
		sal_op_set_to(op,to);
	sal_exosip_fix_route(op);
	eXosip_lock();
	eXosip_subscribe_build_initial_request(&msg,sal_op_get_to(op),sal_op_get_from(op),
	    	sal_op_get_route(op),"presence",600);
	if (op->base.contact){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,op->base.contact);
	}
	op->sid=eXosip_subscribe_send_initial_request(msg);
	eXosip_unlock();
	if (op->sid==-1){
		osip_message_free(msg);
		return -1;
	}
	sal_add_out_subscribe(op->base.root,op);
	return 0;
}

int sal_unsubscribe(SalOp *op){
	osip_message_t *msg=NULL;
	if (op->did==-1){
		ms_error("cannot unsubscribe, no dialog !");
		return -1;
	}
	eXosip_lock();
	eXosip_subscribe_build_refresh_request(op->did,&msg);
	if (msg){
		osip_message_set_expires(msg,"0");
		eXosip_subscribe_send_refresh_request(op->did,msg);
	}else ms_error("Could not build subscribe refresh request ! op->sid=%i, op->did=%i",
	    	op->sid,op->did);
	eXosip_unlock();
	return 0;
}

int sal_subscribe_accept(SalOp *op){
	osip_message_t *msg;
	eXosip_lock();
	eXosip_insubscription_build_answer(op->tid,202,&msg);
	if (op->base.contact){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,op->base.contact);
	}
	eXosip_insubscription_send_answer(op->tid,202,msg);
	eXosip_unlock();
	return 0;
}

int sal_subscribe_decline(SalOp *op){
	eXosip_lock();
	eXosip_insubscription_send_answer(op->tid,401,NULL);
	eXosip_unlock();
	return 0;
}

static void add_presence_body(osip_message_t *notify, SalPresenceStatus online_status)
{
	char buf[1000];
#ifdef SUPPORT_MSN
	int atom_id = 1000;
#endif
	char *contact_info;

	osip_from_t *from=NULL;
	from=osip_message_get_from(notify);
	osip_uri_to_str(from->url,&contact_info);

#ifdef SUPPORT_MSN

  if (online_status==SalPresenceOnline)
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"online\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

    }
  else if (online_status==SalPresenceBusy)
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"busy\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

    }
  else if (online_status==SalPresenceBerightback)
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"berightback\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

    }
  else if (online_status==SalPresenceAway)
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

    }
  else if (online_status==SalPresenceOnthephone)
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"onthephone\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

    }
  else if (online_status==SalPresenceOuttolunch)
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"outtolunch\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

    }
  else
    {
      sprintf(buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);
    }

  osip_message_set_body(notify, buf, strlen(buf));
  osip_message_set_content_type(notify, "application/xpidf+xml");
#else

  if (online_status==SalPresenceOnline)
    {
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>online</note>\n\
</tuple>\n\
</presence>",
	      contact_info, contact_info);
    }
  else if (online_status==SalPresenceBusy)
    {
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>busy</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>busy</note>\n\
</tuple>\n\
</presence>",
	      contact_info, contact_info);
    }
  else if (online_status==SalPresenceBerightback)
    {
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>in-transit</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>be right back</note>\n\
</tuple>\n\
</presence>",
	      contact_info, contact_info);
    }
  else if (online_status==SalPresenceAway)
    {
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>away</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>away</note>\n\
</tuple>\n\
</presence>",
	      contact_info, contact_info);
    }
  else if (online_status==SalPresenceOnthephone)
    {
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>on-the-phone</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>on the phone</note>\n\
</tuple>\n\
</presence>",
	      contact_info, contact_info);
    }
  else if (online_status==SalPresenceOuttolunch)
    {
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>meal</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>out to lunch</note>\n\
</tuple>\n\
</presence>",
	      contact_info, contact_info);
    }
  else
    {
      /* */
      sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
entity=\"%s\">\n%s",
	      contact_info,
"<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>closed</basic>\n\
<es:activities>\n\
  <es:activity>permanent-absence</es:activity>\n\
</es:activities>\n\
</status>\n\
</tuple>\n\
\n</presence>\n");
    }
  osip_message_set_body(notify, buf, strlen(buf));
  osip_message_set_content_type(notify, "application/pidf+xml");

#endif
	osip_free(contact_info);
}


int sal_notify_presence(SalOp *op, SalPresenceStatus status, const char *status_message){
	osip_message_t *msg;
	eXosip_ss_t ss=EXOSIP_SUBCRSTATE_ACTIVE;
	if (op->nid==-1){
		ms_warning("Cannot notify, subscription was closed.");
		return -1;
	}
	
	eXosip_lock();
	eXosip_insubscription_build_notify(op->did,ss,DEACTIVATED,&msg);
	if (msg!=NULL){
		const char *identity=sal_op_get_contact(op);
		if (identity==NULL) identity=sal_op_get_to(op);
		osip_message_set_contact(msg,identity);
		add_presence_body(msg,status);
		eXosip_insubscription_send_request(op->did,msg);
	}else ms_error("could not create notify for incoming subscription.");
	eXosip_unlock();
	return 0;
}

int sal_notify_close(SalOp *op){
	osip_message_t *msg=NULL;
	eXosip_lock();
	eXosip_insubscription_build_notify(op->did,EXOSIP_SUBCRSTATE_TERMINATED,DEACTIVATED,&msg);
	if (msg!=NULL){
		const char *identity=sal_op_get_contact(op);
		if (identity==NULL) identity=sal_op_get_to(op);
		osip_message_set_contact(msg,identity);
		add_presence_body(msg,SalPresenceOffline);
		eXosip_insubscription_send_request(op->did,msg);
	}else ms_error("sal_notify_close(): could not create notify for incoming subscription"
	    " did=%i, nid=%i",op->did,op->nid);
	eXosip_unlock();
	return 0;
}

int sal_publish(SalOp *op, const char *from, const char *to, SalPresenceStatus presence_mode){
	osip_message_t *pub;
	int i;
	char buf[1024];

	if (presence_mode==SalPresenceOnline)
	{
	  snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
		  entity=\"%s\">\n\
	<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>open</basic>\n\
	</status>\n\
	<contact priority=\"0.8\">%s</contact>\n\
	<note>online</note>\n\
	</tuple>\n\
	</presence>",
		   from, from);
	}
	else if (presence_mode==SalPresenceBusy
	   ||presence_mode==SalPresenceDonotdisturb)
	{
	  snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
		  xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
		  entity=\"%s\">\n\
	<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>open</basic>\n\
	<es:activities>\n\
	<es:activity>busy</es:activity>\n\
	</es:activities>\n\
	</status>\n\
	<contact priority=\"0.8\">%s</contact>\n\
	<note>busy</note>\n\
	</tuple>\n\
	</presence>",
		  from, from);
	}
	else if (presence_mode==SalPresenceBerightback)
	{
		snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
		  xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
		  entity=\"%s\">\n\
	<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>open</basic>\n\
	<es:activities>\n\
	<es:activity>in-transit</es:activity>\n\
	</es:activities>\n\
	</status>\n\
	<contact priority=\"0.8\">%s</contact>\n\
	<note>be right back</note>\n\
	</tuple>\n\
	</presence>",
		  from,from);
	}
	else if (presence_mode==SalPresenceAway
	   ||presence_mode==SalPresenceMoved)
	{
		snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
		  xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
		  entity=\"%s\">\n\
	<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>open</basic>\n\
	<es:activities>\n\
	<es:activity>away</es:activity>\n\
	</es:activities>\n\
	</status>\n\
	<contact priority=\"0.8\">%s</contact>\n\
	<note>away</note>\n\
	</tuple>\n\
	</presence>",
		  from, from);
	}
	else if (presence_mode==SalPresenceOnthephone)
	{
	  snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
		  xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
		  entity=\"%s\">\n\
	<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>open</basic>\n\
	<es:activities>\n\
	<es:activity>on-the-phone</es:activity>\n\
	</es:activities>\n\
	</status>\n\
	<contact priority=\"0.8\">%s</contact>\n\
	<note>on the phone</note>\n\
	</tuple>\n\
	</presence>",
		  from, from);
	}
	else if (presence_mode==SalPresenceOuttolunch)
	{
	  snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
		  xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
		  entity=\"%s\">\n\
	<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>open</basic>\n\
	<es:activities>\n\
	<es:activity>meal</es:activity>\n\
	</es:activities>\n\
	</status>\n\
	<contact priority=\"0.8\">%s</contact>\n\
	<note>out to lunch</note>\n\
	</tuple>\n\
	</presence>",
		  from, from);
	}
	else{ 
	  /* offline */
	  snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
	xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
	entity=\"%s\">\n%s",
		  from,
	"<tuple id=\"sg89ae\">\n\
	<status>\n\
	<basic>closed</basic>\n\
	<es:activities>\n\
	<es:activity>permanent-absence</e:activity>\n\
	</es:activities>\n\
	</status>\n\
	</tuple>\n\
	\n</presence>\n");
	}

	i = eXosip_build_publish(&pub,from, to, NULL, "presence", "1800", "application/pidf+xml", buf);
	if (i<0){
		ms_warning("Failed to build publish request.");
		return -1;
	}

	eXosip_lock();
	i = eXosip_publish(pub, to); /* should update the sip-if-match parameter
				    from sip-etag  from last 200ok of PUBLISH */
	eXosip_unlock();
	if (i<0){
	  ms_message("Failed to send publish request.");
	  return -1;
	}
	sal_add_other(sal_op_get_sal(op),op,pub);
	return 0;
}

static void _sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev){	
	SalOp *op=sal_op_new(sal);
	char *tmp;
	op->did=ev->did;
	op->tid=ev->tid;
	op->nid=ev->nid;
	osip_from_to_str(ev->request->from,&tmp);
	sal_op_set_from(op,tmp);
	ms_free(tmp);
	osip_from_to_str(ev->request->to,&tmp);
	sal_op_set_to(op,tmp);
	ms_free(tmp);
	sal_add_in_subscribe(sal,op,ev->request);
	sal->callbacks.subscribe_received(op,sal_op_get_from(op));
}

void sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev){	
	/*workaround a bug in eXosip: incoming SUBSCRIBES within dialog with expires: 0 are
	 recognized as new incoming subscribes*/
	SalOp *op=sal_find_in_subscribe_by_call_id(sal,ev->request->call_id);
	if (op){
		osip_header_t *h;
		osip_message_header_get_byname(ev->request,"expires",0,&h);
		if (h && h->hvalue && atoi(h->hvalue)==0){
			ms_warning("This susbscribe is not a new one but terminates an old one.");
			ev->did=op->did;
			ev->nid=op->nid;
			sal_exosip_subscription_closed(sal,ev);
		}else {
			osip_message_t *msg=NULL;
			ms_warning("Probably a refresh subscribe");
			eXosip_lock();
			eXosip_insubscription_build_answer(ev->tid,202,&msg);
			eXosip_insubscription_send_answer(ev->tid,202,msg);
			eXosip_unlock();
		}
	}else _sal_exosip_subscription_recv(sal,ev);
}

void sal_exosip_notify_recv(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_find_out_subscribe(sal,ev->sid);
	char *tmp;
	osip_from_t *from=NULL;
	osip_body_t *body=NULL;
	SalPresenceStatus estatus=SalPresenceOffline;
	
	ms_message("Receiving notify with sid=%i,nid=%i",ev->sid,ev->nid);

	if (op==NULL){
		ms_error("No operation related to this notify !");
		return;
	}
	if (ev->request==NULL) return;

	from=ev->request->from;
	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		ms_error("No body in NOTIFY");
		return;
	}
	osip_from_to_str(from,&tmp);
	if (strstr(body->body,"pending")!=NULL){
		estatus=SalPresenceOffline;
	}else if (strstr(body->body,"busy")!=NULL){
		estatus=SalPresenceBusy;
	}else if (strstr(body->body,"berightback")!=NULL
			|| strstr(body->body,"in-transit")!=NULL ){
		estatus=SalPresenceBerightback;
	}else if (strstr(body->body,"away")!=NULL){
		estatus=SalPresenceAway;
	}else if (strstr(body->body,"onthephone")!=NULL
		|| strstr(body->body,"on-the-phone")!=NULL){
		estatus=SalPresenceOnthephone;
	}else if (strstr(body->body,"outtolunch")!=NULL
			|| strstr(body->body,"meal")!=NULL){
		estatus=SalPresenceOuttolunch;
	}else if (strstr(body->body,"closed")!=NULL){
		estatus=SalPresenceOffline;
	}else if ((strstr(body->body,"online")!=NULL) || (strstr(body->body,"open")!=NULL)) {
		estatus=SalPresenceOnline;
	}else{
		estatus=SalPresenceOffline;
	}
	ms_message("We are notified that %s has online status %i",tmp,estatus);
	if (ev->ss_status==EXOSIP_SUBCRSTATE_TERMINATED) {
		sal_remove_out_subscribe(sal,op);
		op->sid=-1;
		op->did=-1;
		ms_message("And outgoing subscription terminated by remote.");
	}
	sal->callbacks.notify_presence(op,op->sid!=-1 ? SalSubscribeActive : SalSubscribeTerminated, estatus,NULL);
	osip_free(tmp);
}

void sal_exosip_subscription_answered(Sal *sal,eXosip_event_t *ev){
	SalOp *op=sal_find_out_subscribe(sal,ev->sid);
	if (op==NULL){
		ms_error("Subscription answered but no associated op !");
		return;
	}
	op->did=ev->did;
}

void sal_exosip_in_subscription_closed(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_find_in_subscribe(sal,ev->nid);
	char *tmp;
	if (op==NULL){
		ms_error("Incoming subscription closed but no associated op !");
		return;
	}
	
	
	sal_remove_in_subscribe(sal,op);
	op->nid=-1;
	op->did=-1;
	if (ev->request){
		osip_from_to_str(ev->request->from,&tmp);
		sal->callbacks.subscribe_closed(op,tmp);
		osip_free(tmp);
	}
}

void sal_exosip_subscription_closed(Sal *sal,eXosip_event_t *ev){
	SalOp *op=sal_find_out_subscribe(sal,ev->sid);
	if (op==NULL){
		ms_error("Subscription closed but no associated op !");
		return;
	}
	sal_remove_out_subscribe(sal,op);
	op->sid=-1;
	op->did=-1;
	sal->callbacks.notify_presence(op,SalSubscribeTerminated, SalPresenceOffline,NULL);
}


