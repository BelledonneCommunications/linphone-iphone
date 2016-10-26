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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "sal_impl.h"
/**/
/* Address manipulation API*/
SalAddress * sal_address_new(const char *uri){
	belle_sip_header_address_t*  result;
	if (uri) {
		result=belle_sip_header_address_parse (uri);
		/*may return NULL*/
	} else {
		result = belle_sip_header_address_new();
		belle_sip_header_address_set_uri(result,belle_sip_uri_new());
	}
	if (result) belle_sip_object_ref(result);
	return (SalAddress *)result;
}

SalAddress * sal_address_clone(const SalAddress *addr){
	return (SalAddress *) belle_sip_object_ref(belle_sip_object_clone(BELLE_SIP_OBJECT(addr)));
}

const char *sal_address_get_scheme(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	belle_generic_uri_t* generic_uri = belle_sip_header_address_get_absolute_uri(header_addr);
	if (uri) {
		if (belle_sip_uri_is_secure(uri)) return "sips";
		else return "sip";
	} else if (generic_uri)
		return belle_generic_uri_get_scheme(generic_uri);
	else
		return NULL;
}

void sal_address_set_secure(SalAddress *addr, bool_t enabled){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) belle_sip_uri_set_secure(uri,enabled);
}

bool_t sal_address_is_secure(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) return belle_sip_uri_is_secure(uri);
	return FALSE;
}

const char *sal_address_get_display_name(const SalAddress* addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	return belle_sip_header_address_get_displayname(header_addr);

}
const char *sal_address_get_display_name_unquoted(const SalAddress *addr){
	return sal_address_get_display_name(addr);
}
#define SAL_ADDRESS_GET(addr,param) \
{belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);\
belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);\
if (uri) {\
	return belle_sip_uri_get_##param(uri);\
} else\
	return NULL;}

#define SAL_ADDRESS_SET(addr,param,value) {\
belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);\
belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);\
belle_sip_uri_set_##param(uri,value);}

const char *sal_address_get_username(const SalAddress *addr){
	SAL_ADDRESS_GET(addr,user)
}
const char *sal_address_get_domain(const SalAddress *addr){
	SAL_ADDRESS_GET(addr,host)
}
int sal_address_get_port(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		return belle_sip_uri_get_port(uri);
	} else
		return -1;
}
SalTransport sal_address_get_transport(const SalAddress* addr){
	const char *transport=sal_address_get_transport_name(addr);
	if (transport)
		return sal_transport_parse(transport);
	else
		return SalTransportUDP;
};

const char* sal_address_get_transport_name(const SalAddress* addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		return belle_sip_uri_get_transport_param(uri);
	}
	return NULL;
}

const char *sal_address_get_method_param(const SalAddress *addr) {
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		return belle_sip_uri_get_method_param(uri);
	}
	return NULL;
}

void sal_address_set_display_name(SalAddress *addr, const char *display_name){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_header_address_set_displayname(header_addr,display_name);
}

void sal_address_set_username(SalAddress *addr, const char *username){
	SAL_ADDRESS_SET(addr,user,username);
}

void sal_address_set_password(SalAddress *addr, const char *passwd){
	SAL_ADDRESS_SET(addr,user_password,passwd);
}

const char* sal_address_get_password(const SalAddress *addr){
	SAL_ADDRESS_GET(addr,user_password);
}

void sal_address_set_domain(SalAddress *addr, const char *host){
	SAL_ADDRESS_SET(addr,host,host);
}

void sal_address_set_port(SalAddress *addr, int port){
	SAL_ADDRESS_SET(addr,port,port);
}

void sal_address_clean(SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri=belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		belle_sip_parameters_clean(BELLE_SIP_PARAMETERS(uri));
		belle_sip_uri_headers_clean(uri);
	}
	belle_sip_parameters_clean(BELLE_SIP_PARAMETERS(header_addr));
	return ;
}

char *sal_address_as_string(const SalAddress *addr){
	char tmp[1024]={0};
	size_t off=0;
	belle_sip_object_marshal((belle_sip_object_t*)addr,tmp,sizeof(tmp),&off);
	return ms_strdup(tmp);
}

bool_t sal_address_is_sip(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	return belle_sip_header_address_get_uri(header_addr) != NULL;
}

char *sal_address_as_string_uri_only(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* sip_uri = belle_sip_header_address_get_uri(header_addr);
	belle_generic_uri_t* absolute_uri = belle_sip_header_address_get_absolute_uri(header_addr);
	char tmp[1024]={0};
	size_t off=0;
	belle_sip_object_t* uri;

	if (sip_uri) {
		uri=(belle_sip_object_t*)sip_uri;
	} else if (absolute_uri) {
		uri=(belle_sip_object_t*)absolute_uri;
	} else {
		ms_error("Cannot generate string for addr [%p] with null uri",addr);
		return NULL;
	}
	belle_sip_object_marshal(uri,tmp,sizeof(tmp),&off);
	return ms_strdup(tmp);
}

void sal_address_set_param(SalAddress *addr,const char* name,const char* value){
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(addr);
	belle_sip_parameters_set_parameter(parameters,name,value);
	return ;
}

bool_t sal_address_has_param(const SalAddress *addr, const char *name){
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(addr);
	return belle_sip_parameters_has_parameter(parameters, name);
}

const char * sal_address_get_param(const SalAddress *addr, const char *name) {
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(addr);
	return belle_sip_parameters_get_parameter(parameters, name);
}

void sal_address_set_params(SalAddress *addr, const char *params){
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(addr);
	belle_sip_parameters_set(parameters,params);
}

void sal_address_set_uri_param(SalAddress *addr, const char *name, const char *value) {
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(addr)));
	belle_sip_parameters_set_parameter(parameters, name, value);
}

void sal_address_set_uri_params(SalAddress *addr, const char *params){
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(addr)));
	belle_sip_parameters_set(parameters,params);
}

bool_t sal_address_has_uri_param(const SalAddress *addr, const char *name){
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(addr)));
	return belle_sip_parameters_has_parameter(parameters, name);
}

const char * sal_address_get_uri_param(const SalAddress *addr, const char *name) {
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(addr)));
	return belle_sip_parameters_get_parameter(parameters, name);
}

void sal_address_set_header(SalAddress *addr, const char *header_name, const char *header_value){
	belle_sip_uri_set_header(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(addr)),header_name, header_value);
}

void sal_address_set_transport(SalAddress* addr,SalTransport transport){
	if (!sal_address_is_secure(addr)){
		SAL_ADDRESS_SET(addr,transport_param,sal_transport_to_string(transport));
	}
}

void sal_address_set_transport_name(SalAddress* addr,const char *transport){
	SAL_ADDRESS_SET(addr,transport_param,transport);
}

void sal_address_set_method_param(SalAddress *addr, const char *method) {
	SAL_ADDRESS_SET(addr, method_param, method);
}

SalAddress *sal_address_ref(SalAddress *addr){
	return (SalAddress*)belle_sip_object_ref(BELLE_SIP_HEADER_ADDRESS(addr));
}

void sal_address_unref(SalAddress *addr){
	belle_sip_object_unref(BELLE_SIP_HEADER_ADDRESS(addr));
}

bool_t sal_address_is_ipv6(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri){
		const char *host=belle_sip_uri_get_host(uri);
		if (host && strchr(host,':')!=NULL)
			return TRUE;
	}
	return FALSE;
}

void sal_address_destroy(SalAddress *addr){
	sal_address_unref(addr);
}

