#ifdef IN_LINPHONE
#include "linphonecore.h"
#include "lpconfig.h"
#else
#include <linphone/linphonecore.h>
#include <linphone/lpconfig.h>
#endif
#include <libsoup/soup.h>

static bool_t buddy_lookup_init(void){
	return TRUE;
}

typedef struct _BuddyLookupState{
	SoupSession *session;
	BuddyLookupStatus status;
	SoupMessage *msg;
	ortp_thread_t th;
	MSList *results;
	bool_t processing;
}BuddyLookupState;

#define get_buddy_lookup_state(ctx)	((BuddyLookupState*)((ctx)->data))

static void set_proxy(SoupSession *session){
	SoupURI *uri=soup_uri_new("http://web-proxy.gre.hp.com:8080");
	g_object_set(G_OBJECT(session),"proxy-uri",uri,NULL);
}

static void buddy_lookup_instance_init(SipSetupContext *ctx){
	BuddyLookupState *s=ms_new0(BuddyLookupState,1);
	s->session=soup_session_sync_new();
	set_proxy(s->session);
	ctx->data=s;
}

static void buddy_lookup_instance_uninit(SipSetupContext *ctx){
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
	g_object_unref(G_OBJECT(s->session));
	ms_free(s);
}

static SoupMessage * build_xmlrpc_request(const char *identity, const char *password, const char *key, const char *domain, const char *url){
	SoupMessage * msg;

	msg=soup_xmlrpc_request_new(url,
				"fp.searchUsers",
				G_TYPE_STRING, identity,
				G_TYPE_STRING, password ? password : "",
				G_TYPE_STRING, key,
				G_TYPE_INT , 100,
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

static void got_headers(SipSetupContext *ctx, SoupMessage*msg){
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
	ms_message("Got headers !");
	s->status=BuddyLookupConnected;
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

static void fill_buddy_info(BuddyInfo *bi, GHashTable *ht){
	char tmp[128];
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
}

static MSList * make_buddy_list(GValue *retval){
	MSList *ret=NULL;
	if (G_VALUE_TYPE(retval)==G_TYPE_VALUE_ARRAY){
		GValueArray *array=(GValueArray*)g_value_get_boxed(retval);
		GValue *gelem;
		int i;
		for(i=0;i<array->n_values;++i){
			gelem=g_value_array_get_nth(array,i);
			if (G_VALUE_TYPE(gelem)==G_TYPE_HASH_TABLE){
				GHashTable *ht=(GHashTable*)g_value_get_boxed(gelem);
				BuddyInfo *bi=ms_new0(BuddyInfo,1);
				fill_buddy_info(bi,ht);
				ret=ms_list_append(ret,bi);
			}else{
				ms_error("Element is not a hash table");
			}
		}
	}else ms_error("Return value is not an array");
	return ret;
}


static int xml_rpc_parse_response(SipSetupContext *ctx, SoupMessage *sm){
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
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
		s->status=BuddyLookupFailure;
	}else{
		ms_message("Extracting values from return type...");
		s->results=make_buddy_list(&retval);
		g_value_unset(&retval);
		s->status=BuddyLookupDone;
	}
	soup_buffer_free(sb);
	return s->status==BuddyLookupDone ? 0 : -1;
}

static void * process_xml_rpc_request(void *up){
	SipSetupContext *ctx=(SipSetupContext*)up;
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
	SoupMessage *sm=s->msg;
	int code;
	g_signal_connect_swapped(G_OBJECT(sm),"got-headers",(GCallback)got_headers,ctx);
	s->status=BuddyLookupConnecting;
	code=soup_session_send_message(s->session,sm);
	if (code==200){
		ms_message("Got a response from server, yeah !");
		xml_rpc_parse_response(ctx,sm);
	}else{
		ms_error("request failed, error-code=%i (%s)",code,soup_status_get_phrase(code));
		s->status=BuddyLookupFailure;
	}
	s->processing=FALSE;
	return NULL;
}

static int lookup_buddy(SipSetupContext *ctx, const char *key){
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	LpConfig *config=linphone_core_get_config(lc);
	const char *identity=linphone_proxy_config_get_identity(cfg);
	const char *url=lp_config_get_string(config,"BuddyLookup","url",NULL);
	LinphoneAuthInfo *aa;
	SoupMessage *sm;

	if (url==NULL){
		ms_error("No url defined for BuddyLookup in config file, aborting search.");
		return -1;
	}
	if (s->th!=0){
		if (s->processing){
			ms_message("Canceling previous request...");
			soup_session_cancel_message(s->session,s->msg, SOUP_STATUS_CANCELLED);
		}
		ortp_thread_join(s->th,NULL);
		s->th=0;
		g_object_unref(G_OBJECT(s->msg));
	}

	osip_from_t *from;
	osip_from_init(&from);
	if (osip_from_parse(from,identity)!=0){
		osip_from_free(from);
		ms_error("Could not parse identity %s",identity);
		return -1;
	}
	aa=linphone_core_find_auth_info(lc,from->url->host,from->url->username);
	if (aa) ms_message("There is a password: %s",aa->passwd);
	else ms_message("No password for %s on %s",from->url->username,from->url->host);
	sm=build_xmlrpc_request(identity, aa ? aa->passwd : NULL, key, from->url->host, url);
	osip_from_free(from);
	s->msg=sm;
	s->processing=TRUE;
	ortp_thread_create(&s->th,NULL,process_xml_rpc_request,ctx);
	if (!sm) return -1;
	return 0;
}

static BuddyLookupStatus get_buddy_lookup_status(SipSetupContext *ctx){
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
	return s->status;
}

static int get_buddy_lookup_results(SipSetupContext *ctx, MSList **results){
	BuddyLookupState *s=get_buddy_lookup_state(ctx);
	if (s->results){
		*results=s->results;
		s->results=NULL;
	}
	return 0;
}

static void buddy_lookup_exit(void){
}

static SipSetup buddy_lookup_funcs={
	.name="BuddyLookup",
	.capabilities=SIP_SETUP_CAP_BUDDY_LOOKUP,
	.init=buddy_lookup_init,
	.init_instance=buddy_lookup_instance_init,
	.lookup_buddy=lookup_buddy,
	.get_buddy_lookup_status=get_buddy_lookup_status,
	.get_buddy_lookup_results=get_buddy_lookup_results,
	.uninit_instance=buddy_lookup_instance_uninit,
	.exit=buddy_lookup_exit
};

void libbuddylookup_init(){
	sip_setup_register(&buddy_lookup_funcs);
	ms_message("Buddylookup plugin registered.");
}
