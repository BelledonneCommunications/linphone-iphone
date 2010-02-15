/*
linphone
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphonecore.h"
#include "lpconfig.h"
#include "private.h"
#include <eXosip2/eXosip.h>

/**
 * @addtogroup linphone_address
 * @{
**/

/**
 * Constructs a LinphoneAddress object by parsing the user supplied address,
 * given as a string.
**/
LinphoneAddress * linphone_address_new(const char *uri){
	osip_from_t *from;
	osip_from_init(&from);
	if (osip_from_parse(from,uri)!=0){
		osip_from_free(from);
		ms_error("Cannot create LinphoneAddress, bad uri [%s]",uri);
		return NULL;
	}
	return from;
}

/**
 * Clones a LinphoneAddress object.
**/
LinphoneAddress * linphone_address_clone(const LinphoneAddress *uri){
	osip_from_t *ret=NULL;
	osip_from_clone(uri,&ret);
	return ret;
}

#define null_if_empty(s) (((s)!=NULL && (s)[0]!='\0') ? (s) : NULL )

/**
 * Returns the address scheme, normally "sip".
**/
const char *linphone_address_get_scheme(const LinphoneAddress *u){
	return null_if_empty(u->url->scheme);
}

/**
 * Returns the display name.
**/
const char *linphone_address_get_display_name(const LinphoneAddress* u){
	return null_if_empty(u->displayname);
}

/**
 * Returns the username.
**/
const char *linphone_address_get_username(const LinphoneAddress *u){
	return null_if_empty(u->url->username);
}

/**
 * Returns the domain name.
**/
const char *linphone_address_get_domain(const LinphoneAddress *u){
	return null_if_empty(u->url->host);
}

/**
 * Sets the display name.
**/
void linphone_address_set_display_name(LinphoneAddress *u, const char *display_name){
	if (u->displayname!=NULL){
		osip_free(u->displayname);
		u->displayname=NULL;
	}
	if (display_name!=NULL)
		u->displayname=osip_strdup(display_name);
}

/**
 * Sets the username.
**/
void linphone_address_set_username(LinphoneAddress *uri, const char *username){
	if (uri->url->username!=NULL){
		osip_free(uri->url->username);
		uri->url->username=NULL;
	}
	if (username)
		uri->url->username=osip_strdup(username);
}

/**
 * Sets the domain.
**/
void linphone_address_set_domain(LinphoneAddress *uri, const char *host){
	if (uri->url->host!=NULL){
		osip_free(uri->url->host);
		uri->url->host=NULL;
	}
	if (host)
		uri->url->host=osip_strdup(host);
}

/**
 * Sets the port number.
**/
void linphone_address_set_port(LinphoneAddress *uri, const char *port){
	if (uri->url->port!=NULL){
		osip_free(uri->url->port);
		uri->url->port=NULL;
	}
	if (port)
		uri->url->port=osip_strdup(port);
}

/**
 * Sets the port number.
**/
void linphone_address_set_port_int(LinphoneAddress *uri, int port){
	char tmp[12];
	if (port==5060){
		/*this is the default, special case to leave the port field blank*/
		linphone_address_set_port(uri,NULL);
		return;
	}
	snprintf(tmp,sizeof(tmp),"%i",port);
	linphone_address_set_port(uri,tmp);
}

/**
 * Removes address's tags and uri headers so that it is displayable to the user.
**/
void linphone_address_clean(LinphoneAddress *uri){
	osip_generic_param_freelist(&uri->gen_params);
}

/**
 * Returns the address as a string.
 * The returned char * must be freed by the application. Use ms_free().
**/
char *linphone_address_as_string(const LinphoneAddress *u){
	char *tmp,*ret;
	osip_from_to_str(u,&tmp);
	ret=ms_strdup(tmp);
	osip_free(tmp);
	return ret;
}

/**
 * Returns the SIP uri only as a string, that is display name is removed.
 * The returned char * must be freed by the application. Use ms_free().
**/
char *linphone_address_as_string_uri_only(const LinphoneAddress *u){
	char *tmp=NULL,*ret;
	osip_uri_to_str(u->url,&tmp);
	ret=ms_strdup(tmp);
	osip_free(tmp);
	return ret;
}

/**
 * Destroys a LinphoneAddress object.
**/
void linphone_address_destroy(LinphoneAddress *u){
	osip_from_free(u);
}


/** @} */
