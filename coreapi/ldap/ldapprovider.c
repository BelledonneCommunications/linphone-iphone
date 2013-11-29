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

#include "ldapprovider.h"
#include "linphonecore_utils.h"
#include "lpconfig.h"
#include <ldap.h>


#define MAX_RUNNING_REQUESTS 10
#define FILTER_MAX_SIZE      512
typedef struct {
	int                        msgid;
	LinphoneLDAPContactSearch* request;
} LDAPRequestEntry;

typedef enum {
	ANONYMOUS,
	PLAIN,
	SASL
} LDAPAuthMethod;

struct _LinphoneLDAPContactProvider
{
	LDAP*   ld;
	MSList* requests;
	uint    req_count;
	
	// bind transaction
	uint bind_msgid;

	// config
	int use_tls;
	LDAPAuthMethod auth_method;
	char* username;
	char* password;
	char* server;

	char*  base_object;
	char** attributes;
	char*  filter;
	int    timeout;
	int    deref_aliases;
	int    max_results;
};

struct _LinphoneLDAPContactSearch
{
	LDAP *ld;
	int   msgid;
	char* filter;
};


/* *************************
 * LinphoneLDAPContactSearch
 * *************************/

LinphoneLDAPContactSearch*linphone_ldap_contact_search_create(LinphoneLDAPContactProvider* cp, const char* predicate, ContactSearchCallback cb, void* cb_data)
{
	LinphoneLDAPContactSearch* search = belle_sip_object_new(LinphoneLDAPContactSearch);
	LinphoneContactSearch* base = LINPHONE_CONTACT_SEARCH(search);
	struct timeval timeout = { cp->timeout, 0 };

	linphone_contact_search_init(base, predicate, cb, cb_data);

	search->ld = cp->ld;

	search->filter = ms_malloc(FILTER_MAX_SIZE);
	snprintf(search->filter, FILTER_MAX_SIZE-1, cp->filter, predicate);
	search->filter[FILTER_MAX_SIZE-1] = 0;

	int ret = ldap_search_ext(search->ld,
					cp->base_object, // base from which to start
					LDAP_SCOPE_SUBTREE,
					search->filter, // search predicate
					cp->attributes, // which attributes to get
					0,              // 0 = get attrs AND value, 1 = get attrs only
					NULL,
					NULL,
					&timeout,       // server timeout for the search
					cp->max_results,// max result number
					&search->msgid );
	if( ret != LDAP_SUCCESS ){
		ms_error("Error ldap_search_ext returned %d (%s)", ret, ldap_err2string(ret));
		belle_sip_object_unref(search);
		return NULL;
	}
	return search;
}

static void linphone_ldap_contact_destroy( LinphoneLDAPContactSearch* obj )
{
	if( obj->filter ) ms_free(obj->filter);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneLDAPContactSearch);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneLDAPContactSearch,LinphoneContactSearch,
						   (belle_sip_object_destroy_t)linphone_ldap_contact_destroy,
						   NULL,
						   NULL,
						   TRUE
);


/* ***************************
 * LinphoneLDAPContactProvider
 * ***************************/
struct AuthMethodDescription{
	LDAPAuthMethod method;
	const char* description;
};

static struct AuthMethodDescription ldap_auth_method_description[] = {
	{ANONYMOUS, "anonymous"},
	{PLAIN,     "plain"},
	{SASL,      "sasl"},
	{0,         NULL}
};

static LDAPAuthMethod linphone_ldap_contact_provider_auth_method( const char* description )
{
	struct AuthMethodDescription* desc = ldap_auth_method_description;
	while( desc && desc->description ){
		if( strcmp(description, desc->description) == 0)
			return desc->method;
		desc++;
	}
	return ANONYMOUS;
}

static void linphone_ldap_contact_provider_conf_destroy(LinphoneLDAPContactProvider* obj )
{
	if(obj->username)    ms_free(obj->username);
	if(obj->password)    ms_free(obj->password);
	if(obj->base_object) ms_free(obj->base_object);

	if(obj->attributes){
		int i=0;
		for( ; obj->attributes[i]; i++){
			ms_free(obj->attributes[i]);
		}
		ms_free(obj->attributes);
	}
}

static void linphone_ldap_contact_provider_destroy( LinphoneLDAPContactProvider* obj )
{
	if (obj->ld) ldap_unbind_ext(obj->ld, NULL, NULL);
	obj->ld = NULL;
	linphone_ldap_contact_provider_conf_destroy(obj);
}

static LinphoneLDAPContactSearch*
linphone_ldap_begin_search(LinphoneLDAPContactProvider* obj,
						   const char* predicate,
						   ContactSearchCallback cb,
						   void* cb_data)
{
	LDAPRequestEntry* entry = ms_new0(LDAPRequestEntry,1);
	if( entry){
		LinphoneLDAPContactSearch* request = linphone_ldap_contact_search_create(obj, predicate, cb, cb_data);

		entry->msgid   = request->msgid;
		entry->request = request;

		obj->requests  = ms_list_append(obj->requests, entry);
		obj->req_count++;
		return request;
	} else {
		return NULL;
	}
}

static int linphone_ldap_request_entry_compare(const void*a, const void* b)
{
	const LDAPRequestEntry* ra = (const LDAPRequestEntry*)a;
	const LDAPRequestEntry* rb = (const LDAPRequestEntry*)b;
	return !(ra->msgid == rb->msgid);
}

static unsigned int linphone_ldap_cancel_search(LinphoneContactProvider* obj, LinphoneContactSearch *req)
{
	LinphoneLDAPContactSearch* ldap_req = LINPHONE_LDAP_CONTACT_SEARCH(req);
	LinphoneLDAPContactProvider* ldap_cp = LINPHONE_LDAP_CONTACT_PROVIDER(obj);
	LDAPRequestEntry dummy = { ldap_req->msgid, ldap_req };
	int ret = 1;

	MSList* list_entry = ms_list_find_custom(ldap_cp->requests, linphone_ldap_request_entry_compare, &dummy);
	if( list_entry ) {
		ldap_cp->requests = ms_list_remove(ldap_cp->requests, list_entry);
		ldap_cp->req_count--;
		ms_free(list_entry);
		ret = 0; // return OK if we found it in the monitored requests
	} else {
		ms_warning("Couldn't find ldap request %p (id %d) in monitoring.", ldap_req, ldap_req->msgid);
	}
	belle_sip_object_unref(req);
	return ret;
}

static int linphone_ldap_parse_bind_results( LinphoneContactProvider* obj, LDAPMessage* results )
{
	LinphoneLDAPContactProvider* cp = LINPHONE_LDAP_CONTACT_PROVIDER(obj);
	struct berval *servercreds;
	int ret = ldap_parse_sasl_bind_result(cp->ld, results, &servercreds, 1);
	if( ret != LDAP_SUCCESS ){
		ms_error("ldap_parse_sasl_bind_result failed")
	}
	return ret;
}

static bool_t linphone_ldap_contact_provider_iterate(void *data)
{
	LinphoneLDAPContactProvider* obj = LINPHONE_LDAP_CONTACT_PROVIDER(data);
	if( obj->ld && ((obj->req_count >= 0) || (obj->bind_msgid != 0) )){

		// never block
		struct timeval timeout = {0,0};
		LDAPMessage* results = NULL;

		int res = ldap_result(obj->ld, LDAP_RES_ANY, LDAP_MSG_ALL, &timeout, &results);

		switch( res ){
		case -1:
		{
			ms_warning("Error in ldap_result : returned -1");
			break;
		}
		case 0: break; // nothing to do
		case LDAP_RES_BIND: 
		{
			ms_message("iterate: LDAP_RES_BIND");
			if( ldap_msgid( results ) != obj->bind_msgid ) {
				ms_error("Bad msgid");
			} else {
				linphone_ldap_parse_bind_results( obj, results );
				obj->bind_msgid = 0;
			}
			break;
		}
		case LDAP_RES_SEARCH_ENTRY:
		case LDAP_RES_SEARCH_REFERENCE:
		case LDAP_RES_SEARCH_RESULT:
		case LDAP_RES_MODIFY:
		case LDAP_RES_ADD:
		case LDAP_RES_DELETE:
		case LDAP_RES_MODDN:
		case LDAP_RES_COMPARE:
		case LDAP_RES_EXTENDED:
		case LDAP_RES_INTERMEDIATE:
		{
			ms_message("Got LDAP result type %x", res);
			ldap_msgfree(results);
			break;
		}
		default:
			ms_message("Unhandled LDAP result %x", res);
			break;
		}
	}
	return TRUE;
}

static void linphone_ldap_contact_provider_loadconfig(LinphoneLDAPContactProvider* obj, LpConfig* config)
{
	const char* section="ldap";
	char* attributes_list, *saveptr, *attr;
	unsigned int attr_count = 0, attr_idx = 0, i;
	obj->use_tls       = lp_config_get_int(config, section, "use_tls", 0);
	obj->timeout       = lp_config_get_int(config, section, "timeout", 10);
	obj->deref_aliases = lp_config_get_int(config, section, "deref_aliases", 0);
	obj->max_results   = lp_config_get_int(config, section, "max_results", 50);
	obj->auth_method   = linphone_ldap_contact_provider_auth_method( lp_config_get_string(config, section, "auth_method", "anonymous"));


	// free any pre-existing char* conf values
	linphone_ldap_contact_provider_conf_destroy(obj);


	/*
	 * parse the attributes list
	 */
	attributes_list = ms_strdup(lp_config_get_string(config, section,
													 "attributes",
													 "telephoneNumber,givenName,sn"));

	// count attributes:
	for( i=0; attributes_list[i]; i++)
	{
		if( attributes_list[i] == ',') attr_count++;
	}
	obj->attributes = ms_malloc0((attr_count+2) * sizeof(char*));
	// 1 more for the first attr without ',', the other for the null-finished list

	attr = strtok_r( attributes_list, ",", &saveptr );
	while( attr != NULL ){
		obj->attributes[attr_idx] = ms_strdup(attr);
		attr_idx++;
		attr = strtok_r(NULL, ",", &saveptr);
	}
	if( attr_idx != attr_count+1) ms_error("Invalid attribute number!!! %d expected, got %d", attr_count+1, attr_idx);

	ms_free(attributes_list);

	obj->username    = ms_strdup(lp_config_get_string(config, section, "username", ""));
	obj->password    = ms_strdup(lp_config_get_string(config, section, "password", ""));
	obj->base_object = ms_strdup(lp_config_get_string(config, section, "base_object", ""));
	obj->server      = ms_strdup(lp_config_get_string(config, section, "server", "ldap://localhost:10389"));


}

static int linphone_ldap_contact_provider_bind( LinphoneLDAPContactProvider* obj )
{
	struct berval password = { strlen( obj->password), obj->password };
	int ret;
	int bind_msgid = 0;
	
	switch( obj->auth_method ){
	case ANONYMOUS:
	default:
	{
		char *auth = NULL;
		ret = ldap_sasl_bind( obj->ld, obj->base_object, auth, &password, NULL, NULL, &bind_msgid);
		if( ret == LDAP_SUCCESS ) {
			obj->bind_msgid = bind_msgid;
		} else {
			int err;
			ldap_get_option(obj->ld, LDAP_OPT_RESULT_CODE, &err);
			ms_error("ldap_sasl_bind error %d (%s)", err, ldap_err2string(err) );
		}
		break;
	}
	case SASL:
	{
		break;
	}
	}
	return 0;
}

LinphoneLDAPContactProvider*linphone_ldap_contact_provider_create(LinphoneCore* lc)
{
	LinphoneLDAPContactProvider* obj = belle_sip_object_new(LinphoneLDAPContactProvider);
	linphone_contact_provider_init(LINPHONE_CONTACT_PROVIDER(obj), lc);
	memset(obj->requests, MAX_RUNNING_REQUESTS, sizeof(LDAPRequestEntry));

	int proto_version = LDAP_VERSION3;
	ms_message( "Constructed Contact provider '%s'", BELLE_SIP_OBJECT_VPTR(obj,LinphoneContactProvider)->name);

	linphone_ldap_contact_provider_loadconfig(obj, linphone_core_get_config(lc));

	int ret = ldap_initialize(&(obj->ld),obj->server);

	if( ret != LDAP_SUCCESS ){
		ms_error( "Problem initializing ldap on url '%s': %s", obj->server, ldap_err2string(ret));
		belle_sip_object_unref(obj);
		return NULL;
	} else if( (ret = ldap_set_option(obj->ld, LDAP_OPT_PROTOCOL_VERSION, &proto_version)) != LDAP_SUCCESS ){
		ms_error( "Problem setting protocol version %d: %s", proto_version, ldap_err2string(ret));
		belle_sip_object_unref(obj);
		return NULL;
	} else {
		// register our hook into iterate so that LDAP can do its magic asynchronously.
		linphone_ldap_contact_provider_bind(obj);
		linphone_core_add_iterate_hook(lc, linphone_ldap_contact_provider_iterate, obj);
	}
	return obj;
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneLDAPContactProvider);

BELLE_SIP_INSTANCIATE_CUSTOM_VPTR(LinphoneLDAPContactProvider)=
{
	{
		{
			BELLE_SIP_VPTR_INIT(LinphoneLDAPContactProvider,LinphoneContactProvider,TRUE),
			(belle_sip_object_destroy_t)linphone_ldap_contact_provider_destroy,
			NULL,
			NULL
		},
		"LDAP",
		(LinphoneContactProviderStartSearchMethod)linphone_ldap_begin_search,
		(LinphoneContactProviderCancelSearchMethod)linphone_ldap_cancel_search
	}
};
