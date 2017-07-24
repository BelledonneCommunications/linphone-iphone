/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphone/ldapprovider.h"
#include "private.h"
#include "linphone/lpconfig.h"
#include "contact_providers_priv.h"
#include "mediastreamer2/mscommon.h"
#include <belle-sip/dict.h>

#ifdef BUILD_LDAP
#include <ldap.h>
#include <sasl/sasl.h>


#define MAX_RUNNING_REQUESTS 10
#define FILTER_MAX_SIZE      512

struct LDAPFriendData {
	char* name;
	char* sip;
};

struct _LinphoneLDAPContactProvider
{
	LinphoneContactProvider base;
	LinphoneDictionary* config;

	LDAP*   ld;
	bctbx_list_t* requests;
	unsigned int    req_count;

	// bind transaction
	bool_t connected;
	ms_thread_t bind_thread;

	// config
	int use_tls;
	const char*  auth_method;
	const char*  username;
	const char*  password;
	const char*  server;
	const char*  bind_dn;

	const char*  sasl_authname;
	const char*  sasl_realm;

	const char*  base_object;
	const char*  sip_attr;
	const char*  name_attr;
	const char*  filter;

	char**       attributes;

	int    timeout;
	int    deref_aliases;
	int    max_results;

};

struct _LinphoneLDAPContactSearch
{
	LinphoneContactSearch base;
	LDAP*   ld;
	int     msgid;
	char*   filter;
	bool_t  complete;
	bctbx_list_t* found_entries;
	unsigned int found_count;
};


/* *************************
 * LinphoneLDAPContactSearch
 * *************************/

LinphoneLDAPContactSearch* linphone_ldap_contact_search_create(LinphoneLDAPContactProvider* cp, const char* predicate, ContactSearchCallback cb, void* cb_data)
{
	LinphoneLDAPContactSearch* search = belle_sip_object_new(LinphoneLDAPContactSearch);
	LinphoneContactSearch* base = LINPHONE_CONTACT_SEARCH(search);

	linphone_contact_search_init(base, predicate, cb, cb_data);

	search->ld = cp->ld;

	search->filter = ms_malloc(FILTER_MAX_SIZE);
	snprintf(search->filter, FILTER_MAX_SIZE-1, cp->filter, predicate);
	search->filter[FILTER_MAX_SIZE-1] = 0;

	return search;
}

void linphone_ldap_contact_search_destroy_friend( void* entry )
{
	linphone_friend_destroy((LinphoneFriend*)entry);
}

unsigned int linphone_ldap_contact_search_result_count(LinphoneLDAPContactSearch* obj)
{
	return obj->found_count;
}

static void linphone_ldap_contact_search_destroy( LinphoneLDAPContactSearch* obj )
{
	//ms_message("~LinphoneLDAPContactSearch(%p)", obj);
	bctbx_list_for_each(obj->found_entries, linphone_ldap_contact_search_destroy_friend);
	obj->found_entries = bctbx_list_free(obj->found_entries);
	if( obj->filter ) ms_free(obj->filter);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneLDAPContactSearch);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneLDAPContactSearch,LinphoneContactSearch,
						   (belle_sip_object_destroy_t)linphone_ldap_contact_search_destroy,
						   NULL,
						   NULL,
						   TRUE
);


/* ***************************
 * LinphoneLDAPContactProvider
 * ***************************/

static inline LinphoneLDAPContactSearch* linphone_ldap_contact_provider_request_search( LinphoneLDAPContactProvider* obj, int msgid );
static unsigned int linphone_ldap_contact_provider_cancel_search(LinphoneContactProvider* obj, LinphoneContactSearch *req);
static void linphone_ldap_contact_provider_conf_destroy(LinphoneLDAPContactProvider* obj );
static bool_t linphone_ldap_contact_provider_iterate(void *data);
static int linphone_ldap_contact_provider_bind_interact(LDAP *ld, unsigned flags, void *defaults, void *sasl_interact);
static int linphone_ldap_contact_provider_perform_search( LinphoneLDAPContactProvider* obj, LinphoneLDAPContactSearch* req);

static void linphone_ldap_contact_provider_destroy_request_cb(void *req)
{
	belle_sip_object_unref(req);
}

static void linphone_ldap_contact_provider_destroy( LinphoneLDAPContactProvider* obj )
{
	//ms_message("linphone_ldap_contact_provider_destroy");
	linphone_core_remove_iterate_hook(LINPHONE_CONTACT_PROVIDER(obj)->lc, linphone_ldap_contact_provider_iterate,obj);

	// clean pending requests
	bctbx_list_for_each(obj->requests, linphone_ldap_contact_provider_destroy_request_cb);

	if (obj->ld) ldap_unbind_ext(obj->ld, NULL, NULL);
	obj->ld = NULL;

	if( obj->config ) linphone_dictionary_unref(obj->config);

	linphone_ldap_contact_provider_conf_destroy(obj);
}

static int linphone_ldap_contact_provider_complete_contact( LinphoneLDAPContactProvider* obj, struct LDAPFriendData* lf, const char* attr_name, const char* attr_value)
{
	if( strcmp(attr_name, obj->name_attr ) == 0 ){
		lf->name = ms_strdup(attr_value);
	} else if( strcmp(attr_name, obj->sip_attr) == 0 ) {
		lf->sip = ms_strdup(attr_value);
	}

	// return 1 if the structure has enough data to create a linphone friend
	if( lf->name && lf->sip )
		return 1;
	else
		return 0;

}

static void linphone_ldap_contact_provider_handle_search_result( LinphoneLDAPContactProvider* obj, LinphoneLDAPContactSearch* req, LDAPMessage* message )
{
	int msgtype = ldap_msgtype(message);

	switch(msgtype){

	case LDAP_RES_SEARCH_ENTRY:
	case LDAP_RES_EXTENDED:
	{
		LDAPMessage *entry = ldap_first_entry(obj->ld, message);
		LinphoneCore*   lc = LINPHONE_CONTACT_PROVIDER(obj)->lc;

		while( entry != NULL ){

			struct LDAPFriendData ldap_data = {0};
			bool_t contact_complete = FALSE;
			BerElement*  ber = NULL;
			char*       attr = ldap_first_attribute(obj->ld, entry, &ber);

			while( attr ){
				struct berval** values = ldap_get_values_len(obj->ld, entry, attr);
				struct berval**     it = values;

				while( values && *it && (*it)->bv_val && (*it)->bv_len )
				{
					contact_complete = linphone_ldap_contact_provider_complete_contact(obj, &ldap_data, attr, (*it)->bv_val);
					if( contact_complete ) break;

					it++;
				}

				if( values ) ldap_value_free_len(values);
				ldap_memfree(attr);

				if( contact_complete ) break;

				attr = ldap_next_attribute(obj->ld, entry, ber);
			}

			if( contact_complete ) {
				LinphoneAddress* la = linphone_core_interpret_url(lc, ldap_data.sip);
				if( la ){
					LinphoneFriend* lf = linphone_core_create_friend(lc);
					linphone_friend_set_address(lf, la);
					linphone_friend_set_name(lf, ldap_data.name);
					req->found_entries = bctbx_list_append(req->found_entries, lf);
					req->found_count++;
					//ms_message("Added friend %s / %s", ldap_data.name, ldap_data.sip);
					ms_free(ldap_data.sip);
					ms_free(ldap_data.name);
					linphone_address_unref(la);
				}
			}

			if( ber ) ber_free(ber, 0);

			entry = ldap_next_entry(obj->ld, entry);
		}
	}
	break;

	case LDAP_RES_SEARCH_RESULT:
	{
		// this one is received when a request is finished
		req->complete = TRUE;
		linphone_contact_search_invoke_cb(LINPHONE_CONTACT_SEARCH(req), req->found_entries);
	}
	break;


	default: ms_message("[LDAP] Unhandled message type %x", msgtype); break;
	}
}

static bool_t linphone_ldap_contact_provider_iterate(void *data)
{
	LinphoneLDAPContactProvider* obj = LINPHONE_LDAP_CONTACT_PROVIDER(data);
	if( obj->ld && obj->connected && (obj->req_count > 0) ){

		// never block
		struct timeval timeout = {0,0};
		LDAPMessage* results = NULL;

		int ret = ldap_result(obj->ld, LDAP_RES_ANY, LDAP_MSG_ONE, &timeout, &results);

		switch( ret ){
		case -1:
		{
			ms_warning("Error in ldap_result : returned -1 (req_count %d): %s", obj->req_count, ldap_err2string(errno));
			break;
		}
		case 0: break; // nothing to do

		case LDAP_RES_BIND:
		{
			ms_error("iterate: unexpected LDAP_RES_BIND");
			break;
		}
		case LDAP_RES_EXTENDED:
		case LDAP_RES_SEARCH_ENTRY:
		case LDAP_RES_SEARCH_REFERENCE:
		case LDAP_RES_INTERMEDIATE:
		case LDAP_RES_SEARCH_RESULT:
		{
			LDAPMessage* message = ldap_first_message(obj->ld, results);
			LinphoneLDAPContactSearch* req = linphone_ldap_contact_provider_request_search(obj, ldap_msgid(message));
			while( message != NULL ){
				linphone_ldap_contact_provider_handle_search_result(obj, req, message );
				message = ldap_next_message(obj->ld, message);
			}
			if( req && ret == LDAP_RES_SEARCH_RESULT)
				linphone_ldap_contact_provider_cancel_search(
							LINPHONE_CONTACT_PROVIDER(obj),
							LINPHONE_CONTACT_SEARCH(req));
			break;
		}
		case LDAP_RES_MODIFY:
		case LDAP_RES_ADD:
		case LDAP_RES_DELETE:
		case LDAP_RES_MODDN:
		case LDAP_RES_COMPARE:
		default:
			ms_message("Unhandled LDAP result %x", ret);
			break;
		}

		if( results )
			ldap_msgfree(results);
	}

	if( obj->ld && obj->connected ){
		// check for pending searches
		unsigned int i;

		for( i=0; i<obj->req_count; i++){
			LinphoneLDAPContactSearch* search = (LinphoneLDAPContactSearch*)bctbx_list_nth_data( obj->requests, i );
			if( search && search->msgid == 0){
				int ret;
				ms_message("Found pending search %p (for %s), launching...", search, search->filter);
				ret = linphone_ldap_contact_provider_perform_search(obj, search);
				if( ret != LDAP_SUCCESS ){
					linphone_ldap_contact_provider_cancel_search(
								LINPHONE_CONTACT_PROVIDER(obj),
								LINPHONE_CONTACT_SEARCH(search));
				}
			}
		}
	}

	return TRUE;
}

static void linphone_ldap_contact_provider_conf_destroy(LinphoneLDAPContactProvider* obj )
{
	if(obj->attributes){
		int i=0;
		for( ; obj->attributes[i]; i++){
			ms_free(obj->attributes[i]);
		}
		ms_free(obj->attributes);
	}
}

static char* required_config_keys[] = {
	// connection
	"server",
	"use_tls",
	"auth_method",
	"username",
	"password",
	"bind_dn",
	"sasl_authname",
	"sasl_realm",

	// search
	"base_object",
	"filter",
	"name_attribute",
	"sip_attribute",
	"attributes",

	// misc
	"timeout",
	"max_results",
	"deref_aliases",
	NULL
};

static bool_t linphone_ldap_contact_provider_valid_config(const LinphoneDictionary* dict)
{
	char** config_name = required_config_keys;

	bool_t valid = TRUE;
	bool_t has_key;

	while(*config_name ){
		has_key = linphone_dictionary_haskey(dict, *config_name);
		if( !has_key ) ms_error("Missing LDAP config value for '%s'", *config_name);
		valid &= has_key;
		config_name++;
	}
	return valid;
}

static void linphone_ldap_contact_provider_loadconfig(LinphoneLDAPContactProvider* obj, const LinphoneDictionary* dict)
{
	char* attributes_list, *saveptr, *attr;
	unsigned int attr_count = 0, attr_idx = 0, i;

	if( !linphone_ldap_contact_provider_valid_config(dict) ) return;

	// free any pre-existing attributes values
	linphone_ldap_contact_provider_conf_destroy(obj);
	if( obj->config ) linphone_dictionary_unref(obj->config);

	// clone new config into the dictionary
	obj->config = linphone_dictionary_ref(linphone_dictionary_clone(dict));

#if 0 // until sasl auth is set up, force anonymous auth.
	linphone_dictionary_set_string(obj->config, "auth_method", "ANONYMOUS");
#endif

	obj->use_tls       = linphone_dictionary_get_int(obj->config, "use_tls",       0);
	obj->timeout       = linphone_dictionary_get_int(obj->config, "timeout",       10);
	obj->deref_aliases = linphone_dictionary_get_int(obj->config, "deref_aliases", 0);
	obj->max_results   = linphone_dictionary_get_int(obj->config, "max_results",   50);
	obj->auth_method   = linphone_dictionary_get_string(obj->config, "auth_method",    "ANONYMOUS");
	obj->username      = linphone_dictionary_get_string(obj->config, "username",       "");
	obj->password      = linphone_dictionary_get_string(obj->config, "password",       "");
	obj->bind_dn       = linphone_dictionary_get_string(obj->config, "bind_dn",        "");
	obj->base_object   = linphone_dictionary_get_string(obj->config, "base_object",    "dc=example,dc=com");
	obj->server        = linphone_dictionary_get_string(obj->config, "server",         "ldap://localhost");
	obj->filter        = linphone_dictionary_get_string(obj->config, "filter",         "uid=*%s*");
	obj->name_attr     = linphone_dictionary_get_string(obj->config, "name_attribute", "givenName");
	obj->sip_attr      = linphone_dictionary_get_string(obj->config, "sip_attribute",  "mobile");
	obj->sasl_authname = linphone_dictionary_get_string(obj->config, "sasl_authname",  "");
	obj->sasl_realm    = linphone_dictionary_get_string(obj->config, "sasl_realm",  "");

	/*
	 * parse the attributes list
	 */
	attributes_list = ms_strdup(
				linphone_dictionary_get_string(obj->config,
											   "attributes",
											   "telephoneNumber,givenName,sn,mobile,homePhone")
				);

	// count attributes:
	for( i=0; attributes_list[i]; i++) {
		if( attributes_list[i] == ',') attr_count++;
	}

	// 1 more for the first attr without ',', the other for the null-finished list
	obj->attributes = ms_malloc0((attr_count+2) * sizeof(char*));

	attr = strtok_r( attributes_list, ",", &saveptr );
	while( attr != NULL ){
		obj->attributes[attr_idx] = ms_strdup(attr);
		attr_idx++;
		attr = strtok_r(NULL, ",", &saveptr);
	}
	if( attr_idx != attr_count+1) ms_error("Invalid attribute number!!! %d expected, got %d", attr_count+1, attr_idx);

	ms_free(attributes_list);
}

static int linphone_ldap_contact_provider_bind_interact(LDAP *ld,
														unsigned flags,
														void *defaults,
														void *sasl_interact)
{
	sasl_interact_t *interact = (sasl_interact_t*)sasl_interact;
	LinphoneLDAPContactProvider* obj = LINPHONE_LDAP_CONTACT_PROVIDER(defaults);
	ms_message("bind_interact called: ld %p, flags %x, default %p, interact %p",
			   ld, flags, defaults, sasl_interact);

	if( ld == NULL ) return LDAP_PARAM_ERROR;

	while( interact->id != SASL_CB_LIST_END ) {

		const char *dflt = interact->defresult;

		switch( interact->id ) {
		case SASL_CB_GETREALM:
			ms_message("* SASL_CB_GETREALM -> %s", obj->sasl_realm);
			dflt = obj->sasl_realm;
		break;
		case SASL_CB_AUTHNAME:
			ms_message("* SASL_CB_AUTHNAME -> %s", obj->sasl_authname);
			dflt = obj->sasl_authname;
		break;
		case SASL_CB_USER:
			ms_message("* SASL_CB_USER -> %s", obj->username);
			dflt = obj->username;
		break;
		case SASL_CB_PASS:
			ms_message("* SASL_CB_PASS (hidden)");
			dflt = obj->password;
		break;
		default:
			ms_message("SASL interact asked for unknown id %lx\n",interact->id);
		}
		interact->result = (dflt && *dflt) ? dflt : (const char*)"";
		interact->len = strlen( (const char*)interact->result );

		interact++;
	}
	return LDAP_SUCCESS;
}

static void* ldap_bind_thread_func( void*arg)
{
	LinphoneLDAPContactProvider* obj = linphone_ldap_contact_provider_ref(arg);
	const char* auth_mechanism = obj->auth_method;
	int ret;

	if( (strcmp(auth_mechanism, "ANONYMOUS") == 0) || (strcmp(auth_mechanism, "SIMPLE") == 0) )
	{
		struct berval passwd = { strlen(obj->password), ms_strdup(obj->password)};
		auth_mechanism = LDAP_SASL_SIMPLE;
		ret = ldap_sasl_bind_s(obj->ld,
							   obj->bind_dn,
							   auth_mechanism,
							   &passwd,
							   NULL,
							   NULL,
							   NULL);

		ms_free(passwd.bv_val);
	}
	else
	{

		ms_message("LDAP interactive bind");
		ret = ldap_sasl_interactive_bind_s(obj->ld,
										   obj->bind_dn,
										   auth_mechanism,
										   NULL,NULL,
										   LDAP_SASL_QUIET,
										   linphone_ldap_contact_provider_bind_interact,
										   obj);
	}

	if( ret == LDAP_SUCCESS ) {
		ms_message("LDAP bind OK");
		obj->connected = 1;
	} else {
		int err;
		ldap_get_option(obj->ld, LDAP_OPT_RESULT_CODE, &err);
		ms_error("ldap_sasl_bind error returned %x, err %x (%s), auth_method: %s",
				 ret, err, ldap_err2string(err), auth_mechanism );
	}

	obj->bind_thread = 0;
	linphone_ldap_contact_provider_unref(obj);
	return (void*)0;
}

static int linphone_ldap_contact_provider_bind( LinphoneLDAPContactProvider* obj )
{
	// perform the bind in an alternate thread, so that we don't stall the main loop
	ms_thread_create(&obj->bind_thread, NULL, ldap_bind_thread_func, obj);
	return 0;
}

unsigned int linphone_ldap_contact_provider_get_max_result(const LinphoneLDAPContactProvider* obj)
{
	return obj->max_results;
}

static void linphone_ldap_contact_provider_config_dump_cb(const char*key, void* value, void* userdata)
{
	ms_message("- %s -> %s", key, (const char* )value);
}

LinphoneLDAPContactProvider*linphone_ldap_contact_provider_create(LinphoneCore* lc, const LinphoneDictionary* config)
{
	LinphoneLDAPContactProvider* obj = belle_sip_object_new(LinphoneLDAPContactProvider);
	int proto_version = LDAP_VERSION3;

	linphone_contact_provider_init((LinphoneContactProvider*)obj, lc);

	ms_message( "Constructed Contact provider '%s'", BELLE_SIP_OBJECT_VPTR(obj,LinphoneContactProvider)->name);

	if( !linphone_ldap_contact_provider_valid_config(config) ) {
		ms_error( "Invalid configuration for LDAP, aborting creation");
		belle_sip_object_unref(obj);
		obj = NULL;
	} else {
		int ret;
		linphone_dictionary_foreach( config, linphone_ldap_contact_provider_config_dump_cb, 0 );
		linphone_ldap_contact_provider_loadconfig(obj, config);

		ret = ldap_initialize(&(obj->ld),obj->server);

		if( ret != LDAP_SUCCESS ){
			ms_error( "Problem initializing ldap on url '%s': %s", obj->server, ldap_err2string(ret));
			belle_sip_object_unref(obj);
			obj = NULL;
		} else if( (ret = ldap_set_option(obj->ld, LDAP_OPT_PROTOCOL_VERSION, &proto_version)) != LDAP_SUCCESS ){
			ms_error( "Problem setting protocol version %d: %s", proto_version, ldap_err2string(ret));
			belle_sip_object_unref(obj);
			obj = NULL;
		} else {
			// prevents blocking calls to bind() when the server is invalid, but this is not working for now..
			// see bug https://bugzilla.mozilla.org/show_bug.cgi?id=79509
			//ldap_set_option( obj->ld, LDAP_OPT_CONNECT_ASYNC, LDAP_OPT_ON);

			// register our hook into iterate so that LDAP can do its magic asynchronously.
			linphone_core_add_iterate_hook(lc, linphone_ldap_contact_provider_iterate, obj);
		}
	}
	return obj;
}

/**
 * Search an LDAP request in the list of current LDAP requests to serve, only taking
 * the msgid as a key to search.
 */
static int linphone_ldap_request_entry_compare_weak(const void*a, const void* b)
{
	const LinphoneLDAPContactSearch* ra = (const LinphoneLDAPContactSearch*)a;
	const LinphoneLDAPContactSearch* rb = (const LinphoneLDAPContactSearch*)b;
	return !(ra->msgid == rb->msgid); // 0 if equal
}

/**
 * Search an LDAP request in the list of current LDAP requests to serve, with strong search
 * comparing both msgid and request pointer
 */
static int linphone_ldap_request_entry_compare_strong(const void*a, const void* b)
{
	const LinphoneLDAPContactSearch* ra = (const LinphoneLDAPContactSearch*)a;
	const LinphoneLDAPContactSearch* rb = (const LinphoneLDAPContactSearch*)b;
	return !(ra->msgid == rb->msgid) && !(ra == rb);
}

static inline LinphoneLDAPContactSearch* linphone_ldap_contact_provider_request_search( LinphoneLDAPContactProvider* obj, int msgid )
{
	LinphoneLDAPContactSearch dummy = {};
	bctbx_list_t* list_entry;
	dummy.msgid = msgid;

	list_entry = bctbx_list_find_custom(obj->requests, linphone_ldap_request_entry_compare_weak, &dummy);
	if( list_entry ) return list_entry->data;
	else return NULL;
}

static unsigned int linphone_ldap_contact_provider_cancel_search(LinphoneContactProvider* obj, LinphoneContactSearch *req)
{
	LinphoneLDAPContactSearch*  ldap_req = LINPHONE_LDAP_CONTACT_SEARCH(req);
	LinphoneLDAPContactProvider* ldap_cp = LINPHONE_LDAP_CONTACT_PROVIDER(obj);
	int ret = 1;

	bctbx_list_t* list_entry = bctbx_list_find_custom(ldap_cp->requests, linphone_ldap_request_entry_compare_strong, req);
	if( list_entry ) {
		ms_message("Delete search %p", req);
		ldap_cp->requests = bctbx_list_erase_link(ldap_cp->requests, list_entry);
		ldap_cp->req_count--;
		ret = 0; // return OK if we found it in the monitored requests
	} else {
		ms_warning("Couldn't find ldap request %p (id %d) in monitoring.", ldap_req, ldap_req->msgid);
	}
	belle_sip_object_unref(req); // unref request even if not found
	return ret;
}

static int linphone_ldap_contact_provider_perform_search( LinphoneLDAPContactProvider* obj, LinphoneLDAPContactSearch* req)
{
	int ret = -1;
	struct timeval timeout = { obj->timeout, 0 };

	if( req->msgid == 0 ){
		ms_message ( "Calling ldap_search_ext with predicate '%s' on base '%s', ld %p, attrs '%s', maxres = %d", req->filter, obj->base_object, obj->ld, obj->attributes[0], obj->max_results );
		ret = ldap_search_ext(obj->ld,
						obj->base_object,// base from which to start
						LDAP_SCOPE_SUBTREE,
						req->filter,     // search predicate
						obj->attributes, // which attributes to get
						0,               // 0 = get attrs AND value, 1 = get attrs only
						NULL,
						NULL,
						&timeout,        // server timeout for the search
						obj->max_results,// max result number
						&req->msgid );

		if( ret != LDAP_SUCCESS ){
			ms_error("Error ldap_search_ext returned %d (%s)", ret, ldap_err2string(ret));
		} else {
			ms_message("LinphoneLDAPContactSearch created @%p : msgid %d", req, req->msgid);
		}

	} else {
		ms_warning( "LDAP Search already performed for %s, msgid %d", req->filter, req->msgid);
	}
	return ret;
}

static LinphoneLDAPContactSearch* linphone_ldap_contact_provider_begin_search ( LinphoneLDAPContactProvider* obj,
		const char* predicate,
		ContactSearchCallback cb,
		void* cb_data )
{
	bool_t connected = obj->connected;
	LinphoneLDAPContactSearch* request;

	// if we're not yet connected, bind
	if( !connected ) {
		if( !obj->bind_thread ) linphone_ldap_contact_provider_bind(obj);
	}

	request = linphone_ldap_contact_search_create( obj, predicate, cb, cb_data );

	if( connected ){
		int ret = linphone_ldap_contact_provider_perform_search(obj, request);
		ms_message ( "Created search %d for '%s', msgid %d, @%p", obj->req_count, predicate, request->msgid, request );
		if( ret != LDAP_SUCCESS ){
			belle_sip_object_unref(request);
			request = NULL;
		}
	} else {
		ms_message("Delayed search, wait for connection");
	}

	if( request != NULL ) {
		obj->requests = bctbx_list_append ( obj->requests, request );
		obj->req_count++;
	}

	return request;
}


static int linphone_ldap_contact_provider_marshal(LinphoneLDAPContactProvider* obj, char* buff, size_t buff_size, size_t *offset)
{
	belle_sip_error_code error = BELLE_SIP_OK;
	char **attr;

	error = belle_sip_snprintf(buff, buff_size, offset, "ld:%p,\n", obj->ld);
	if(error!= BELLE_SIP_OK) return error;

	error = belle_sip_snprintf(buff, buff_size, offset, "req_count:%d,\n", obj->req_count);
	if(error!= BELLE_SIP_OK) return error;

	error = belle_sip_snprintf(buff, buff_size, offset,
							   "CONFIG:\n"
							   "tls: %d \n"
							   "auth: %s \n"
							   "user: %s \n"
							   "pass: %s \n"
							   "server: %s \n"
							   "base: %s \n"
							   "filter: %s \n"
							   "timeout: %d \n"
							   "deref: %d \n"
							   "max_res: %d \n"
							   "sip_attr:%s \n"
							   "name_attr:%s \n"
							   "attrs:\n",
							   obj->use_tls, obj->auth_method,
							   obj->username, obj->password, obj->server,
							   obj->base_object, obj->filter,
							   obj->timeout, obj->deref_aliases,
							   obj->max_results,
							   obj->sip_attr, obj->name_attr);
	if(error!= BELLE_SIP_OK) return error;

	attr = obj->attributes;
	while( *attr ){
		error = belle_sip_snprintf(buff, buff_size, offset, "- %s\n", *attr);
		if(error!= BELLE_SIP_OK) return error;
		else attr++;
	}

	return error;

}

LinphoneLDAPContactProvider*linphone_ldap_contact_provider_ref(void* obj)
{
	return linphone_ldap_contact_provider_cast(belle_sip_object_ref(obj));
}


void linphone_ldap_contact_provider_unref(void* obj)
{
	belle_sip_object_unref(obj);
}

inline LinphoneLDAPContactSearch*linphone_ldap_contact_search_cast(void* obj)
{
	return BELLE_SIP_CAST(obj, LinphoneLDAPContactSearch);
}


LinphoneLDAPContactProvider* linphone_ldap_contact_provider_cast(void* obj)
{
	return BELLE_SIP_CAST(obj, LinphoneLDAPContactProvider);
}

int linphone_ldap_contact_provider_available()
{
	return 1;
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneLDAPContactProvider);

BELLE_SIP_INSTANCIATE_CUSTOM_VPTR_BEGIN(LinphoneLDAPContactProvider)
	{
		{
			BELLE_SIP_VPTR_INIT(LinphoneLDAPContactProvider,LinphoneContactProvider,TRUE),
			(belle_sip_object_destroy_t)linphone_ldap_contact_provider_destroy,
			NULL,
			(belle_sip_object_marshal_t)linphone_ldap_contact_provider_marshal
		},
		"LDAP",
		(LinphoneContactProviderStartSearchMethod)linphone_ldap_contact_provider_begin_search,
		(LinphoneContactProviderCancelSearchMethod)linphone_ldap_contact_provider_cancel_search
	}
BELLE_SIP_INSTANCIATE_CUSTOM_VPTR_END

#else

/* Stubbed implementation */

LinphoneLDAPContactSearch* linphone_ldap_contact_search_create(LinphoneLDAPContactProvider* ld,
															   const char* predicate,
															   ContactSearchCallback cb,
															   void* cb_data)
{
	return NULL;
}

unsigned int linphone_ldap_contact_search_result_count(LinphoneLDAPContactSearch* obj){ return 0; }
LinphoneLDAPContactSearch* linphone_ldap_contact_search_cast( void* obj ){ return NULL; }


/* LinphoneLDAPContactProvider */

LinphoneLDAPContactProvider* linphone_ldap_contact_provider_create(LinphoneCore* lc, const LinphoneDictionary* config){ return NULL; }
unsigned int                 linphone_ldap_contact_provider_get_max_result(const LinphoneLDAPContactProvider* obj){ return 0; }
LinphoneLDAPContactProvider* linphone_ldap_contact_provider_ref( void* obj ){ return NULL; }
void                         linphone_ldap_contact_provider_unref( void* obj ){  }
LinphoneLDAPContactProvider* linphone_ldap_contact_provider_cast( void* obj ){ return NULL; }

int linphone_ldap_contact_provider_available(){	return 0; }


#endif /* BUILD_LDAP */

