/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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
#include "sal_impl.h"

typedef enum {
	PIDF = 0,
	RFCxxxx = 1,
	MSOLDPRES = 2
} presence_type_t;

/*
 * REVISIT: this static variable forces every dialog to use the same presence description type depending
 * on what is received on a single dialog...
 */
static presence_type_t presence_style = PIDF;

static void mk_presence_body (const SalPresenceStatus online_status, const char *contact_info,
		char *buf, size_t buflen, presence_type_t ptype) {
  switch (ptype) {
    case RFCxxxx: {
	  /* definition from http://msdn.microsoft.com/en-us/library/cc246202%28PROT.10%29.aspx */
	  int atom_id = 1000;

	  if (online_status==SalPresenceOnline)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"online\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceBusy ||
			  online_status == SalPresenceDonotdisturb)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"busy\" />\n\
</address>\n\
</atom>\n</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceBerightback)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"berightback\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceAway ||
			  online_status == SalPresenceMoved)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOnthephone)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"onthephone\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOuttolunch)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"outtolunch\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"closed\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);
	  }
	  break;
    }
    case MSOLDPRES: {
	/* Couldn't find schema http://schemas.microsoft.com/2002/09/sip/presence
 	*  so messages format has been taken from Communigate that can send notify
 	*  requests with this schema
 	*/
	  int atom_id = 1000;

	  if (online_status==SalPresenceOnline)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"online\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceBusy ||
			  online_status == SalPresenceDonotdisturb)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"busy\" />\n\
</address>\n\
</atom>\n</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceBerightback)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"berightback\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceAway ||
			  online_status == SalPresenceMoved)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"idle\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOnthephone)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"onthephone\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOuttolunch)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"outtolunch\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"closed\" />\n\
<msnsubstatus substatus=\"offline\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);
	  }
	break;
	}
    default: { /* use pidf+xml as default format, rfc4479, rfc4480, rfc3863 */

	if (online_status==SalPresenceOnline)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status == SalPresenceBusy ||
			online_status == SalPresenceDonotdisturb)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:busy/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status==SalPresenceBerightback)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:in-transit/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status == SalPresenceAway ||
			online_status == SalPresenceMoved)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:away/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status==SalPresenceOnthephone)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:on-the-phone/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status==SalPresenceOuttolunch)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"7777\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"78787878\">\n\
<rpid:activities><rpid:meal/></rpid:activities>\n\
<rpid:note>Out to lunch</rpid:note> \n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>closed</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
</presence>\n", contact_info, contact_info);
	}
	break;
    }
 } // switch

}

static void add_presence_info(belle_sip_message_t *notify, SalPresenceStatus online_status) {
	char buf[1000];
	char *contact_info;
	size_t content_length;

	belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(notify,belle_sip_header_from_t);

	contact_info=belle_sip_uri_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from)));
	mk_presence_body (online_status, contact_info, buf, sizeof (buf), presence_style);



	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
								,BELLE_SIP_HEADER(belle_sip_header_content_type_create("application",presence_style?"xpidf+xml":"pidf+xml")));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
								,BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length=strlen(buf))));
	belle_sip_message_set_body(BELLE_SIP_MESSAGE(notify),buf,content_length);
	ms_free(contact_info);
}

static void presence_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("presence_process_io_error not implemented yet");
}
static void presence_process_dialog_terminated(void *op, const belle_sip_dialog_terminated_event_t *event) {
	if (((SalOp*)op)->dialog) ((SalOp*)op)->dialog=NULL;
}

static void presence_response_event(void *op_base, const belle_sip_response_event_t *event){
	SalOp* op = (SalOp*)op_base;
	belle_sip_dialog_state_t dialog_state;
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	belle_sip_response_t* response=belle_sip_response_event_get_response(event);
	belle_sip_request_t* request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	int code = belle_sip_response_get_status_code(response);
	char reason[256]={0};
	SalError error=SalErrorUnknown;
	SalReason sr=SalReasonUnknown;
	belle_sip_header_expires_t* expires;
	if (sal_compute_sal_errors(response,&error,&sr,reason, sizeof(reason))) {
		ms_error("subscription to [%s] rejected reason [%s]",sal_op_get_to(op),reason[0]!=0?reason:sal_reason_to_string(sr));
		op->base.root->callbacks.notify_presence(op,SalSubscribeTerminated, SalPresenceOffline,NULL);
		return;
	}
	set_or_update_dialog(op_base,belle_sip_response_event_get_dialog(event));
	if (!op->dialog) {
		ms_message("presence op [%p] receive out of dialog answer [%i]",op,code);
		return;
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);


		switch(dialog_state) {

		case BELLE_SIP_DIALOG_NULL:
		case BELLE_SIP_DIALOG_EARLY: {
			ms_error("presence op [%p] receive an unexpected answer [%i]",op,code);
			break;
		}
		case BELLE_SIP_DIALOG_CONFIRMED: {
			if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0) {
				expires=belle_sip_message_get_header_by_type(request,belle_sip_header_expires_t);
				if(op->refresher) {
					belle_sip_refresher_stop(op->refresher);
					belle_sip_object_unref(op->refresher);
				}
				if (expires>0){
					op->refresher=belle_sip_client_transaction_create_refresher(client_transaction);
				}
			}
			break;
		}
		case BELLE_SIP_DIALOG_TERMINATED:
			if (op->refresher) {
				belle_sip_refresher_stop(op->refresher);
				belle_sip_object_unref(op->refresher);
				op->refresher=NULL;
			}
		break;
		default: {
			ms_error("presence op [%p] receive answer [%i] not implemented",op,code);
		}
		/* no break */
	}


}
static void presence_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	ms_error("presence_process_timeout not implemented yet");
}
static void presence_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	ms_error("presence_process_timeout not implemented yet");
}
static void presence_process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_header_subscription_state_t* subscription_state_header=belle_sip_message_get_header_by_type(req,belle_sip_header_subscription_state_t);
	belle_sip_header_expires_t* expires = belle_sip_message_get_header_by_type(req,belle_sip_header_expires_t);;
	const char* body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
	SalPresenceStatus estatus=SalPresenceOffline;
	SalSubscribeStatus sub_state;
	belle_sip_response_t* resp;
	belle_sip_object_ref(server_transaction);
	if (op->pending_server_trans)  belle_sip_object_unref(op->pending_server_trans);
	op->pending_server_trans=server_transaction;

	if (!op->dialog) {
		op->dialog=belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(server_transaction));
		belle_sip_dialog_set_application_data(op->dialog,op);
		ms_message("new incoming subscription from [%s] to [%s]",sal_op_get_from(op),sal_op_get_to(op));
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);
	switch(dialog_state) {

	case BELLE_SIP_DIALOG_NULL: {
		op->base.root->callbacks.subscribe_received(op,sal_op_get_from(op));
		break;
	}
	case BELLE_SIP_DIALOG_EARLY:
		ms_error("unexpected method [%s] for dialog [%p] in state BELLE_SIP_DIALOG_EARLY ",belle_sip_request_get_method(req),op->dialog);
		break;

	case BELLE_SIP_DIALOG_CONFIRMED:
		if (strcmp("NOTIFY",belle_sip_request_get_method(req))==0) {
			if (body==NULL){
				ms_error("No body in NOTIFY received from [%s]",sal_op_get_from(op));
				return;
			}
			if (strstr(body,"pending")!=NULL){
				estatus=SalPresenceOffline;
			}else if (strstr(body,"busy")!=NULL){
				estatus=SalPresenceBusy;
			}else if (strstr(body,"berightback")!=NULL
					|| strstr(body,"in-transit")!=NULL ){
				estatus=SalPresenceBerightback;
			}else if (strstr(body,"away")!=NULL
					|| strstr(body,"idle")){
				estatus=SalPresenceAway;
			}else if (strstr(body,"onthephone")!=NULL
					|| strstr(body,"on-the-phone")!=NULL){
				estatus=SalPresenceOnthephone;
			}else if (strstr(body,"outtolunch")!=NULL
					|| strstr(body,"meal")!=NULL){
				estatus=SalPresenceOuttolunch;
			}else if (strstr(body,"closed")!=NULL){
				estatus=SalPresenceOffline;
			}else if ((strstr(body,"online")!=NULL) || (strstr(body,"open")!=NULL)) {
				estatus=SalPresenceOnline;
			}else{
				estatus=SalPresenceOffline;
			}
			ms_message("We are notified that [%s] has online status %i",sal_op_get_from(op),estatus);
			if (!subscription_state_header || strcasecmp(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,belle_sip_header_subscription_state_get_state(subscription_state_header)) ==0) {
				sub_state=SalSubscribeTerminated;
				ms_message("And outgoing subscription terminated by remote [%s]",sal_op_get_to(op));
			} else
				sub_state=SalSubscribeActive;

			op->base.root->callbacks.notify_presence(op,sub_state, estatus,NULL);
			resp=belle_sip_response_create_from_request(req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else if (strcmp("SUBSCRIBE",belle_sip_request_get_method(req))==0) {
			/*either a refresh of an unsubscribe*/
			if (expires && belle_sip_header_expires_get_expires(expires)>0) {
				op->base.root->callbacks.subscribe_received(op,sal_op_get_from(op));
			} else if(expires) {
				ms_message("Unsubscribe received from [%s]",sal_op_get_from(op));
				resp=belle_sip_response_create_from_request(req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
			}
		}
		break;
	default: {
		ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
	}
	/* no break */
	}


}
void sal_op_presence_fill_cbs(SalOp*op) {
	op->callbacks.process_io_error=presence_process_io_error;
	op->callbacks.process_response_event=presence_response_event;
	op->callbacks.process_timeout=presence_process_timeout;
	op->callbacks.process_transaction_terminated=presence_process_transaction_terminated;
	op->callbacks.process_request_event=presence_process_request_event;
	op->callbacks.process_dialog_terminated=presence_process_dialog_terminated;
}

/*presence publish */
int sal_publish(SalOp *op, const char *from, const char *to, SalPresenceStatus status){
	ms_error("sal_publish not implemented yet");
	return -1;
}
/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to){
	belle_sip_request_t *req=NULL;
	if (from)
		sal_op_set_from(op,from);
	if (to)
		sal_op_set_to(op,to);

	sal_op_presence_fill_cbs(op);

	/*???sal_exosip_fix_route(op); make sure to ha ;lr*/
	req=sal_op_build_request(op,"SUBSCRIBE");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("Event","Presence"));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_expires_create(600)));

	return sal_op_send_request_with_contact(op,req);
}
int sal_unsubscribe(SalOp *op){
	belle_sip_request_t* req=op->dialog?belle_sip_dialog_create_request(op->dialog,"SUBSCRIBE"):NULL; /*cannot create request if dialog not set yet*/
	if (!req) {
		ms_error("Cannot unsubscribe to [%s]",sal_op_get_to(op));
		return -1;
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("Event","Presence"));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_expires_create(0)));
	return sal_op_send_request_with_contact(op,req);
}
int sal_subscribe_accept(SalOp *op){
	belle_sip_request_t* req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans));
	belle_sip_header_expires_t* expires = belle_sip_message_get_header_by_type(req,belle_sip_header_expires_t);
	belle_sip_response_t* resp = belle_sip_response_create_from_request(req,200);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp),BELLE_SIP_HEADER(expires));
	belle_sip_server_transaction_send_response(op->pending_server_trans,resp);
	return 0;
}
int sal_subscribe_decline(SalOp *op){
	belle_sip_response_t*  resp = belle_sip_response_create_from_request(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans)),403);
	belle_sip_server_transaction_send_response(op->pending_server_trans,resp);
	return 0;
}
int sal_notify_presence(SalOp *op, SalPresenceStatus status, const char *status_message){
	belle_sip_request_t* notify=belle_sip_dialog_create_request(op->dialog,"NOTIFY");
/*	belle_sip_header_address_t* identity=sal_op_get_contact_address(op);
	if (identity==NULL) identity=sal_op_get_to_address(op);
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,identity);*/
	add_presence_info(BELLE_SIP_MESSAGE(notify),status); /*FIXME, what about expires ??*/
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
									,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,600)));
	return sal_op_send_request(op,notify);
}
int sal_notify_close(SalOp *op){
	belle_sip_request_t* notify=belle_sip_dialog_create_request(op->dialog,"NOTIFY");
	add_presence_info(BELLE_SIP_MESSAGE(notify),SalPresenceOffline); /*FIXME, what about expires ??*/
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
									,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,-1)));
	return sal_op_send_request(op,notify);
}
