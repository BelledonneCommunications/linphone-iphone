#include <linphone/core.h>
#include <linphone/lpconfig.h>
#include <libsoup/soup.h>

#define SERIALIZE_HTTPS 0

static bool_t buddy_lookup_init(void){
	return TRUE;
};

typedef struct _BLReq{
	BuddyLookupRequest base;
	SoupMessage *msg;
	SoupSession *session;
	ortp_thread_t th;
}BLReq;


void set_proxy(SoupSession *session, const char *proxy){
	SoupURI *uri=soup_uri_new(proxy);
	ms_message("Using http proxy %s",proxy);
	g_object_set(G_OBJECT(session),"proxy-uri",uri,NULL);
}

static void buddy_lookup_instance_init(SipSetupContext *ctx){
}

static void buddy_lookup_instance_uninit(SipSetupContext *ctx){
}

static SoupMessage * build_xmlrpc_request(const char *identity, const char *password, const char *key, const char *domain, const char *url, int max_results){
	SoupMessage * msg;

	msg=soup_xmlrpc_request_new(url,
				"fp.searchUsers",
				G_TYPE_STRING, identity,
				G_TYPE_STRING, password ? password : "",
				G_TYPE_STRING, key,
				G_TYPE_INT , max_results,
				G_TYPE_INT , 0,
				G_TYPE_STRING, domain,
				G_TYPE_INVALID);
	if (!msg){
		ms_error("Fail to create SoupMessage !");
	}else{
		SoupBuffer *sb=soup_message_body_flatten(msg->request_body);
		ms_message("This is the XML-RPC request we are going to send:\n%s\n",sb->data);
		soup_buffer_free(sb);
	}
	return msg;
}

static void got_headers(BLReq *blreq, SoupMessage*msg){
	ms_message("Got headers !");
	blreq->base.status=BuddyLookupConnected;
}

static void fill_item(GHashTable *ht , const char *name, char *dest, size_t dest_size){
	GValue *v=(GValue*)g_hash_table_lookup(ht,(gconstpointer)name);
	if (v) {
		const char *tmp=g_value_get_string(v);
		if (tmp){
			strncpy(dest,tmp,dest_size-1);
		}
	}else ms_warning("no field named '%s'", name);
}

static void fill_buddy_info(BLReq *blreq, BuddyInfo *bi, GHashTable *ht){
	char tmp[256];
	fill_item(ht,"first_name",bi->firstname,sizeof(bi->firstname));
	fill_item(ht,"last_name",bi->lastname,sizeof(bi->lastname));
	fill_item(ht,"display_name",bi->displayname,sizeof(bi->displayname));
	fill_item(ht,"sip",tmp,sizeof(tmp));
	if (strstr(tmp,"sip:")==0){
		snprintf(bi->sip_uri,sizeof(bi->sip_uri)-1,"sip:%s",tmp);
	}else{
		strncpy(bi->sip_uri,tmp,sizeof(bi->sip_uri)-1);
	}

	fill_item(ht,"street",bi->address.street,sizeof(bi->address.street));
	fill_item(ht,"zip",bi->address.zip,sizeof(bi->address.zip));
	fill_item(ht,"city",bi->address.town,sizeof(bi->address.town));
	fill_item(ht,"country",bi->address.country,sizeof(bi->address.country));
	fill_item(ht,"email",bi->email,sizeof(bi->email));
	tmp[0]='\0';
	fill_item(ht,"image",tmp,sizeof(tmp));
	if (tmp[0]!='\0'){
		SoupMessage *msg;
		guint status;
		ms_message("This buddy has an image, let's download it: %s",tmp);
		msg=soup_message_new("GET",tmp);
		if ((status=soup_session_send_message(blreq->session,msg))==200){
			SoupMessageBody *body=msg->response_body;
			ms_message("Received %i bytes",body->length);
			strncpy(bi->image_type,"png",sizeof(bi->image_type));
			bi->image_length=body->length;
			bi->image_data=ms_malloc(body->length+4); /*add padding bytes*/
			memcpy(bi->image_data,body->data,bi->image_length);
		}else{
			ms_error("Fail to fetch the image %i",status);
		}
	}

}

static bctbx_list_t * make_buddy_list(BLReq *blreq, GValue *retval){
	bctbx_list_t *ret=NULL;
	if (G_VALUE_TYPE(retval)==G_TYPE_VALUE_ARRAY){
		GValueArray *array=(GValueArray*)g_value_get_boxed(retval);
		GValue *gelem;
		int i;
		for(i=0;i<array->n_values;++i){
			gelem=g_value_array_get_nth(array,i);
			if (G_VALUE_TYPE(gelem)==G_TYPE_HASH_TABLE){
				GHashTable *ht=(GHashTable*)g_value_get_boxed(gelem);
				BuddyInfo *bi=buddy_info_new();
				fill_buddy_info(blreq,bi,ht);
				ret=bctbx_list_append(ret,bi);
			}else{
				ms_error("Element is not a hash table");
			}
		}
	}else ms_error("Return value is not an array");
	return ret;
}


static int xml_rpc_parse_response(BLReq *blreq, SoupMessage *sm){
	SoupBuffer *sb;
	GValue retval;
	GError *error=NULL;
	sb=soup_message_body_flatten(sm->response_body);
	ms_message("This the xml-rpc response:\n%s\n",sb->data);
	if (soup_xmlrpc_parse_method_response(sb->data,sb->length,&retval,&error)==FALSE){
		if (error!=NULL){
			ms_error("xmlrpc fault: %s",error->message);
			g_error_free(error);
		}else{
			ms_error("Could not parse xml-rpc response !");
		}
		blreq->base.status=BuddyLookupFailure;
	}else{
		ms_message("Extracting values from return type...");
		blreq->base.results=make_buddy_list(blreq,&retval);
		g_value_unset(&retval);
		blreq->base.status=BuddyLookupDone;
	}
	soup_buffer_free(sb);
	return blreq->base.status==BuddyLookupDone ? 0 : -1;
}

#if SERIALIZE_HTTPS
/*on windows libsoup support for threads with gnutls is not yet functionnal (only in git)
This will come in next release of libsoup, probably.
In the meantime, we are forced to serialize all soup https processing with a big
ugly global mutex...*/

static GStaticMutex big_mutex = G_STATIC_MUTEX_INIT;

#endif

static void * process_xml_rpc_request(void *up){
	BLReq *blreq=(BLReq*)up;
	SoupMessage *sm=blreq->msg;
	int code;
	g_signal_connect_swapped(G_OBJECT(sm),"got-headers",(GCallback)got_headers,blreq);
	blreq->base.status=BuddyLookupConnecting;
#if SERIALIZE_HTTPS
	g_static_mutex_lock(&big_mutex);
#endif
	code=soup_session_send_message(blreq->session,sm);
	if (code==200){
		ms_message("Got a response from server, yeah !");
		xml_rpc_parse_response(blreq,sm);
	}else{
		ms_error("request failed, error-code=%i (%s)",code,soup_status_get_phrase(code));
		blreq->base.status=BuddyLookupFailure;
	}
#if SERIALIZE_HTTPS
	g_static_mutex_unlock(&big_mutex);
#endif
	return NULL;
}

static int lookup_buddy(SipSetupContext *ctx, BLReq *req){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	LpConfig *config=linphone_core_get_config(lc);
	LinphoneAddress *from=linphone_proxy_config_get_identity_address(cfg);
	const char *url=lp_config_get_string(config,"BuddyLookup","url",NULL);
	const LinphoneAuthInfo *auth_info;
	SoupMessage *sm;
	char *identity;

	if (url==NULL){
		ms_error("No url defined for BuddyLookup in config file, aborting search.");
		return -1;
	}
	auth_info=linphone_core_find_auth_info(lc,linphone_address_get_domain(from),linphone_address_get_username(from));
	if (auth_info) {
		ms_message("There is a password: %s",auth_info->passwd);
	} else {
		ms_message("No password for %s on %s",linphone_address_get_username(from),linphone_address_get_domain(from));
	}

	identity=linphone_proxy_config_get_identity(cfg);
	sm=build_xmlrpc_request(identity, auth_info ? auth_info->passwd : NULL, req->base.key, linphone_address_get_domain(from), url, req->base.max_results);
	ms_free(identity);
	req->msg=sm;
	ortp_thread_create(&req->th,NULL,process_xml_rpc_request,req);
	if (!sm) return -1;
	return 0;
}

static BuddyLookupRequest * create_request(SipSetupContext *ctx){
	BLReq *req=ms_new0(BLReq,1);
	const char *proxy=NULL;
	req->session=soup_session_sync_new();
	proxy=getenv("http_proxy");
	if (proxy && strlen(proxy)>0) set_proxy(req->session,proxy);
	return (BuddyLookupRequest*)req;
}

static int submit_request(SipSetupContext *ctx, BuddyLookupRequest *req){
	BLReq *blreq=(BLReq*)req;
	return lookup_buddy(ctx,blreq);
}

static int free_request(SipSetupContext *ctx, BuddyLookupRequest *req){
	BLReq *blreq=(BLReq*)req;
	if (blreq->th!=0){
		soup_session_cancel_message(blreq->session,blreq->msg, SOUP_STATUS_CANCELLED);
		ortp_thread_join(blreq->th,NULL);
		blreq->th=0;
		g_object_unref(G_OBJECT(blreq->msg));
	}
	if (blreq->session)
		g_object_unref(G_OBJECT(blreq->session));
	buddy_lookup_request_free(req);
	return 0;
}

static void buddy_lookup_exit(void){
}

static BuddyLookupFuncs bl_funcs={
	.request_create=create_request,
	.request_submit=submit_request,
	.request_free=free_request
};



static SipSetup buddy_lookup_funcs={
	.name="BuddyLookup",
	.capabilities=SIP_SETUP_CAP_BUDDY_LOOKUP,
	.init=buddy_lookup_init,
	.init_instance=buddy_lookup_instance_init,
	.uninit_instance=buddy_lookup_instance_uninit,
	.exit=buddy_lookup_exit,
	.buddy_lookup_funcs=&bl_funcs,
};

void libbuddylookup_init(){
	sip_setup_register(&buddy_lookup_funcs);
	ms_message("Buddylookup plugin registered.");
}
